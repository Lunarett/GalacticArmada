#include "Pawns/ShipPawn.h"
#include "ChaosVehicleMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipPawn, Log, All)

AShipPawn::AShipPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Initialize Detection Sphere Collision
	DetectionSphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionCollision"));
	DetectionSphereCollision->SetupAttachment(RootComponent);
	DetectionSphereCollision->SetSphereRadius(40000.0f);
	DetectionSphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AShipPawn::OnDetectionOverlapBegin);
	DetectionSphereCollision->OnComponentEndOverlap.AddDynamic(this, &AShipPawn::OnDetectionOverlapEnd);
	
	// Initialize Spring Arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 2000.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.0f;
	SpringArm->CameraRotationLagSpeed = 10.0f;

	// Initialize Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// Initialize Cannon
	Cannon = CreateDefaultSubobject<UCannonComponent>(TEXT("Cannon"));

	// Initialize Health
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->SetDefaultHealth(100.0f);
	Health->OnDeath.AddDynamic(this, &AShipPawn::AShipPawn::OnPawnDied);
}

void AShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(ThrustInputAction, ETriggerEvent::Triggered, this, &AShipPawn::HandleThrustInput);
		EnhancedInputComponent->BindAction(ThrustInputAction, ETriggerEvent::Completed, this, &AShipPawn::HandleThrustInput);
		
		EnhancedInputComponent->BindAction(RollInputAction, ETriggerEvent::Triggered, this, &AShipPawn::HandleRollInput);
		EnhancedInputComponent->BindAction(RollInputAction, ETriggerEvent::Completed, this, &AShipPawn::HandleRollInput);
		
		EnhancedInputComponent->BindAction(PitchInputAction, ETriggerEvent::Triggered, this, &AShipPawn::HandlePitchInput);
		EnhancedInputComponent->BindAction(PitchInputAction, ETriggerEvent::Completed, this, &AShipPawn::HandlePitchInput);

		EnhancedInputComponent->BindAction(YawInputAction, ETriggerEvent::Triggered, this, &AShipPawn::HandleYawInput);
		EnhancedInputComponent->BindAction(YawInputAction, ETriggerEvent::Completed, this, &AShipPawn::HandleYawInput);

		EnhancedInputComponent->BindAction(PrimaryFireInputAction, ETriggerEvent::Triggered, this, &AShipPawn::BeginPrimaryFire);
		EnhancedInputComponent->BindAction(PrimaryFireInputAction, ETriggerEvent::Canceled, this, &AShipPawn::EndPrimaryFire);
		EnhancedInputComponent->BindAction(PrimaryFireInputAction, ETriggerEvent::Completed, this, &AShipPawn::EndPrimaryFire);

		EnhancedInputComponent->BindAction(SecondaryFireInputAction, ETriggerEvent::Triggered, this, &AShipPawn::BeginSecondaryFire);
		EnhancedInputComponent->BindAction(SecondaryFireInputAction, ETriggerEvent::Canceled, this, &AShipPawn::EndSecondaryFire);
		EnhancedInputComponent->BindAction(SecondaryFireInputAction, ETriggerEvent::Completed, this, &AShipPawn::EndSecondaryFire);
	}
}

void AShipPawn::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Input Map
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ShipMappingContext, 0);
		}
	}

	// Initialize Chaos Vehicle Movement Component
	ChaosVehicleComp = Cast<UChaosVehicleMovementComponent>(GetVehicleMovementComponent());
	if (!ChaosVehicleComp) { UE_LOG(LogShipPawn, Error, TEXT("Error ShipPawn.cpp: Failed to cast VehicleMovementComponent to ChaosVehicleMovementComponent")) }
}

void AShipPawn::HandleThrustInput(const FInputActionValue& Value)
{
	const float ThrottleValue = Value.Get<float>();
	if (ChaosVehicleComp)
	{
		ChaosVehicleComp->SetThrottleInput(ThrottleValue);
	}
}

void AShipPawn::HandleRollInput(const FInputActionValue& Value)
{
	const float RollValue = Value.Get<float>();
	if (ChaosVehicleComp)
	{
		ChaosVehicleComp->SetRollInput(RollValue);
	}
}

void AShipPawn::HandlePitchInput(const FInputActionValue& Value)
{
	const float PitchValue = Value.Get<float>();
	if (ChaosVehicleComp)
	{
		ChaosVehicleComp->SetPitchInput(PitchValue);
	}
}

void AShipPawn::HandleYawInput(const FInputActionValue& Value)
{
	const float YawValue = Value.Get<float>();
	if (ChaosVehicleComp)
	{
		ChaosVehicleComp->SetYawInput(YawValue);
	}
}

void AShipPawn::BeginPrimaryFire()
{
	if (Cannon)
	{
		Cannon->BeginCannonFire(0);
	}
}

void AShipPawn::EndPrimaryFire()
{
	if (Cannon)
	{
		Cannon->EndCannonFire(0);
	}
}

void AShipPawn::BeginSecondaryFire()
{
	if (Cannon)
	{
		Cannon->BeginCannonFire(1);
	}
}

void AShipPawn::EndSecondaryFire()
{
	if (Cannon)
	{
		Cannon->EndCannonFire(1);
	}
}

void AShipPawn::OnPawnDied(AController* InstigatedBy, AActor* DamageCauser)
{
	if (ExplosionParticleEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionParticleEffect, GetActorLocation());
	}

	Destroy();
}

void AShipPawn::OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherComp)
	{
		DetectedActors.AddUnique(OtherActor);
	}
}

void AShipPawn::OnDetectionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	DetectedActors.Remove(OtherActor);
}

AActor* AShipPawn::GetNearestActor()
{
	AActor* NearestActor = nullptr;
	float MinDistance = FLT_MAX;

	for (AActor* Actor : DetectedActors)
	{
		const float Distance = FVector::Dist(Actor->GetActorLocation(), GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestActor = Actor;
		}
	}
	
	return NearestActor;
}

FVector AShipPawn::GetClosestCollisionLocation() const
{
	FVector ClosestCollisionLocation = FVector::ZeroVector;
	float MinDistance = FLT_MAX;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	
	for (AActor* Actor : DetectedActors)
	{
		FHitResult HitResult;
		if (GetWorld()->LineTraceSingleByChannel(HitResult, GetActorLocation(), Actor->GetActorLocation(), ECC_Visibility))
		{
			const float Distance = FVector::Dist(HitResult.ImpactPoint, GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestCollisionLocation = HitResult.ImpactPoint;
			}
		}
	}

	return ClosestCollisionLocation;
}
