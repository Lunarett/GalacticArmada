// ShipPlayerController.cpp

#include "Player/ShipPlayerController.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameModeBase.h"

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
	
	if (const ULocalPlayer* ShipLocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ShipLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (ShipInputMappingContext)
			{
				Subsystem->AddMappingContext(ShipInputMappingContext, 0);
			}
		}
	}
}

void AShipPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ShipPawn = Cast<AShipPawn>(InPawn);

	UE_LOG(LogShipPC, Log, TEXT("OnPossess ShipPawn=%s"), *GetNameSafe(ShipPawn));

	SetViewTarget(ShipPawn ? ShipPawn : InPawn);
}

void AShipPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	ShipPawn = nullptr;
}

void AShipPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
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
			EIC->BindAction(PrimaryFireInputAction, ETriggerEvent::Started,   this, &AShipPlayerController::HandleBeginPrimaryFireInput);
			EIC->BindAction(PrimaryFireInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandleEndPrimaryFireInput);
			EIC->BindAction(PrimaryFireInputAction, ETriggerEvent::Canceled,  this, &AShipPlayerController::HandleEndPrimaryFireInput);
		}

		if (SecondaryFireInputAction)
		{
			EIC->BindAction(SecondaryFireInputAction, ETriggerEvent::Started,   this, &AShipPlayerController::HandleBeginSecondaryFireInput);
			EIC->BindAction(SecondaryFireInputAction, ETriggerEvent::Completed, this, &AShipPlayerController::HandleEndSecondaryFireInput);
			EIC->BindAction(SecondaryFireInputAction, ETriggerEvent::Canceled,  this, &AShipPlayerController::HandleEndSecondaryFireInput);
		}
	}
}

void AShipPlayerController::HandleThrottleInput(const FInputActionValue& Value)
{
	if (ShipPawn)
	{
		ShipPawn->SetThrottle(Value.Get<float>());
	}
}

void AShipPlayerController::HandleRollInput(const FInputActionValue& Value)
{
	if (ShipPawn)
	{
		ShipPawn->SetRoll(Value.Get<float>());
	}
}

void AShipPlayerController::HandlePitchInput(const FInputActionValue& Value)
{
	if (ShipPawn)
	{
		ShipPawn->SetPitch(Value.Get<float>());
	}
}

void AShipPlayerController::HandleYawInput(const FInputActionValue& Value)
{
	if (ShipPawn)
	{
		ShipPawn->SetYaw(Value.Get<float>());
	}
}

void AShipPlayerController::HandleBeginPrimaryFireInput()
{
	if (ShipPawn)
	{
		ShipPawn->BeginPrimaryFire();
	}
}

void AShipPlayerController::HandleEndPrimaryFireInput()
{
	if (ShipPawn)
	{
		ShipPawn->EndPrimaryFire();
	}
}

void AShipPlayerController::HandleBeginSecondaryFireInput()
{
	if (ShipPawn)
	{
		ShipPawn->BeginSecondaryFire();
	}
}

void AShipPlayerController::HandleEndSecondaryFireInput()
{
	if (ShipPawn)
	{
		ShipPawn->EndSecondaryFire();
	}
}