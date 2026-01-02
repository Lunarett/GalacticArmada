#include "Player/ShipPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Engine/World.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerCameraManager.h"
#include "Systems/AICommandSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipPC, Log, All);

AShipPlayerController::AShipPlayerController()
{
	PlayerCameraManagerClass = AShipPlayerCameraManager::StaticClass();
	bAutoManageActiveCameraTarget = true;
}

void AShipPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;

	InitializeInputMapping();
	EnsureSubsystemsCached();
}

void AShipPlayerController::EnsureSubsystemsCached()
{
	if (AICommandSubsystem)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	AICommandSubsystem = World->GetSubsystem<UAICommandSubsystem>();
}

void AShipPlayerController::InitializeInputMapping()
{
	const ULocalPlayer* const LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* const InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!InputSubsystem)
	{
		return;
	}

	if (!ShipInputMappingContext)
	{
		return;
	}

	InputSubsystem->AddMappingContext(ShipInputMappingContext, 0);
}

void AShipPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ShipPawn = Cast<AShipPawn>(InPawn);
	RegisterPawnAsAgent();
}

void AShipPlayerController::OnUnPossess()
{
	UnregisterPawnAsAgent();

	Super::OnUnPossess();
	ShipPawn = nullptr;
}

void AShipPlayerController::RegisterPawnAsAgent()
{
	if (!ShipPawn)
	{
		return;
	}

	EnsureSubsystemsCached();

	if (!AICommandSubsystem)
	{
		return;
	}

	const uint8 TeamValue = TeamId.GetId();
	AICommandSubsystem->RegisterAgent(ShipPawn, TeamValue);
}

void AShipPlayerController::UnregisterPawnAsAgent()
{
	if (!ShipPawn)
	{
		return;
	}

	EnsureSubsystemsCached();

	if (!AICommandSubsystem)
	{
		return;
	}

	AICommandSubsystem->UnregisterAgent(ShipPawn);
}

// --------------------------
// IGenericTeamAgentInterface
// --------------------------

FGenericTeamId AShipPlayerController::GetGenericTeamId() const
{
	return TeamId;
}

void AShipPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	if (TeamId == NewTeamId)
	{
		return;
	}

	TeamId = NewTeamId;

	// If we are currently possessing a pawn, update its registration.
	UnregisterPawnAsAgent();
	RegisterPawnAsAgent();
}

FGenericTeamId AShipPlayerController::ResolveTeamFromActor(const AActor& Actor) const
{
	const IGenericTeamAgentInterface* const DirectTeamAgent = Cast<IGenericTeamAgentInterface>(&Actor);
	if (DirectTeamAgent)
	{
		return DirectTeamAgent->GetGenericTeamId();
	}

	const APawn* const OtherPawn = Cast<APawn>(&Actor);
	if (!OtherPawn)
	{
		return FGenericTeamId::NoTeam;
	}

	const AController* const OtherController = OtherPawn->GetController();
	if (!OtherController)
	{
		return FGenericTeamId::NoTeam;
	}

	const IGenericTeamAgentInterface* const ControllerTeamAgent = Cast<IGenericTeamAgentInterface>(OtherController);
	if (!ControllerTeamAgent)
	{
		return FGenericTeamId::NoTeam;
	}

	return ControllerTeamAgent->GetGenericTeamId();
}

ETeamAttitude::Type AShipPlayerController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const FGenericTeamId OtherTeam = ResolveTeamFromActor(Other);

	if (TeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	if (OtherTeam == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	if (TeamId == OtherTeam)
	{
		return ETeamAttitude::Friendly;
	}

	return ETeamAttitude::Hostile;
}

// --------------------------
// Input bindings
// --------------------------

void AShipPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* const EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		return;
	}

	if (ThrottleInputAction)
	{
		EIC->BindAction(ThrottleInputAction, ETriggerEvent::Triggered, this, &AShipPlayerController::HandleThrottleInput);
		EIC->BindAction(ThrottleInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandleThrottleInput);
	}

	if (RollInputAction)
	{
		EIC->BindAction(RollInputAction, ETriggerEvent::Triggered, this, &AShipPlayerController::HandleRollInput);
		EIC->BindAction(RollInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandleRollInput);
	}

	if (PitchInputAction)
	{
		EIC->BindAction(PitchInputAction, ETriggerEvent::Triggered, this, &AShipPlayerController::HandlePitchInput);
		EIC->BindAction(PitchInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandlePitchInput);
	}

	if (YawInputAction)
	{
		EIC->BindAction(YawInputAction, ETriggerEvent::Triggered, this, &AShipPlayerController::HandleYawInput);
		EIC->BindAction(YawInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandleYawInput);
	}

	if (PrimaryFireInputAction)
	{
		EIC->BindAction(PrimaryFireInputAction, ETriggerEvent::Started, this, &AShipPlayerController::HandleBeginPrimaryFireInput);
		EIC->BindAction(PrimaryFireInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandleEndPrimaryFireInput);
		EIC->BindAction(PrimaryFireInputAction, ETriggerEvent::Canceled, this, &AShipPlayerController::HandleEndPrimaryFireInput);
	}

	if (SecondaryFireInputAction)
	{
		EIC->BindAction(SecondaryFireInputAction, ETriggerEvent::Started, this, &AShipPlayerController::HandleBeginSecondaryFireInput);
		EIC->BindAction(SecondaryFireInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandleEndSecondaryFireInput);
		EIC->BindAction(SecondaryFireInputAction, ETriggerEvent::Canceled, this, &AShipPlayerController::HandleEndSecondaryFireInput);
	}
}

void AShipPlayerController::HandleThrottleInput(const FInputActionValue& Value)
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->SetThrottle(Value.Get<float>());
}

void AShipPlayerController::HandleRollInput(const FInputActionValue& Value)
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->SetRoll(Value.Get<float>());
}

void AShipPlayerController::HandlePitchInput(const FInputActionValue& Value)
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->SetPitch(Value.Get<float>());
}

void AShipPlayerController::HandleYawInput(const FInputActionValue& Value)
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->SetYaw(Value.Get<float>());
}

void AShipPlayerController::HandleBeginPrimaryFireInput()
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->BeginPrimaryFire();
}

void AShipPlayerController::HandleEndPrimaryFireInput()
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->EndPrimaryFire();
}

void AShipPlayerController::HandleBeginSecondaryFireInput()
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->BeginSecondaryFire();
}

void AShipPlayerController::HandleEndSecondaryFireInput()
{
	if (!ShipPawn)
	{
		return;
	}

	ShipPawn->EndSecondaryFire();
}