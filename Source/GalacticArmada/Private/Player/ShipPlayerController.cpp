#include "Player/ShipPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerCameraManager.h"
#include "Player/ShipPlayerState.h"
#include "Systems/AICommandSubsystem.h"
#include "Components/HealthComponent.h"

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

	const ULocalPlayer* ShipLocalPlayer = GetLocalPlayer();
	if (!ShipLocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ShipLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	if (!ShipInputMappingContext)
	{
		return;
	}

	Subsystem->AddMappingContext(ShipInputMappingContext, 0);
}

void AShipPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ShipPawn = Cast<AShipPawn>(InPawn);
	if (!ShipPawn)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UAICommandSubsystem* Cmd = World->GetSubsystem<UAICommandSubsystem>();
	if (!Cmd)
	{
		return;
	}

	int32 Team = 0;

	const UHealthComponent* Health = ShipPawn->FindComponentByClass<UHealthComponent>();
	if (Health)
	{
		Team = Health->GetTeamId();
	}

	const AShipPlayerState* PS = GetPlayerState<AShipPlayerState>();
	if (PS)
	{
		Team = PS->GetTeamID();
	}

	const uint8 TeamId = (uint8)FMath::Clamp(Team, 0, 255);
	Cmd->RegisterAgent(InPawn, TeamId);
}

void AShipPlayerController::OnUnPossess()
{
	if (!ShipPawn)
	{
		Super::OnUnPossess();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		Super::OnUnPossess();
		ShipPawn = nullptr;
		return;
	}

	UAICommandSubsystem* Cmd = World->GetSubsystem<UAICommandSubsystem>();
	if (Cmd)
	{
		Cmd->UnregisterAgent(ShipPawn);
	}

	Super::OnUnPossess();
	ShipPawn = nullptr;
}

void AShipPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
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