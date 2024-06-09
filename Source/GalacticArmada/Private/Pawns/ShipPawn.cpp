#include "Pawns/ShipPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Components/ShipMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Controllers/ShipAIController.h"
#include "NiagaraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipPawn, Log, All)

AShipPawn::AShipPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set AI Controller
	AIControllerClass = AShipAIController::StaticClass();

	// Initialize Ship Mesh
	ShipMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;
	ShipMesh->SetSimulatePhysics(true);
	ShipMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ShipMesh->SetCollisionObjectType(ECC_Vehicle);
	ShipMesh->OnComponentHit.AddDynamic(this, &AShipPawn::OnShipCollision);
	
	// Initialize Detection Sphere Collision
	DetectionSphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionCollision"));
	DetectionSphereCollision->SetupAttachment(ShipMesh);
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

	// Initialize Ship Movement
	ShipMovementComponent = CreateDefaultSubobject<UShipMovementComponent>(TEXT("ShipMovement"));

	// Initialize Cannon
	CannonComponent = CreateDefaultSubobject<UCannonComponent>(TEXT("Cannon"));

	// Initialize Health
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	HealthComponent->SetDefaultHealth(100.0f);
	HealthComponent->OnDeath.AddDynamic(this, &AShipPawn::OnPawnDied);
}

void AShipPawn::BeginPlay()
{
	Super::BeginPlay();

	// Setup player input if controlled by player
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ShipMappingContext, 0);
		}
	}

	InitializeThrusterEffects();
}

void AShipPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateThrusterEffects();
}

void AShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
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
		EnhancedInputComponent->BindAction(PrimaryFireInputAction, ETriggerEvent::Completed, this, &AShipPawn::EndPrimaryFire);
		
		EnhancedInputComponent->BindAction(SecondaryFireInputAction, ETriggerEvent::Triggered, this, &AShipPawn::BeginSecondaryFire);
		EnhancedInputComponent->BindAction(SecondaryFireInputAction, ETriggerEvent::Completed, this, &AShipPawn::EndSecondaryFire);
	}
}

void AShipPawn::InitializeThrusterEffects()
{
	for (const FThrusterEffect& ThrusterEffect : ThrusterEffects)
	{
		if (ThrusterEffect.ThrusterParticleEffect)
		{
			UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				ThrusterEffect.ThrusterParticleEffect,
				ShipMesh,
				ThrusterEffect.SocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				false);
			ThrusterParticleEffects.Add(NiagaraComponent);
		}
	}
}

void AShipPawn::UpdateThrusterEffects()
{
	for (int32 i = 0; i < ThrusterParticleEffects.Num(); ++i)
	{
		if (ThrusterParticleEffects[i])
		{
			const float ThrustScale = FMath::Lerp(
				ThrusterEffects[i].MinScale,
				ThrusterEffects[i].MaxScale,
				ShipMovementComponent->GetCurrentSpeed() / ShipMovementComponent->GetMaxSpeed());
			
			ThrusterParticleEffects[i]->SetFloatParameter(TEXT("User.Scale"), ThrustScale);
		}
	}
}

void AShipPawn::HandleThrustInput(const FInputActionValue& Value)
{
	if (!IsValid(ShipMovementComponent)) return;
	const float ThrustValue = Value.Get<float>();
	ShipMovementComponent->SetThrustInput(ThrustValue);
}

void AShipPawn::HandleRollInput(const FInputActionValue& Value)
{
	if (!IsValid(ShipMovementComponent)) return;
	const float RollValue = Value.Get<float>();
	ShipMovementComponent->SetRollInput(RollValue);
}

void AShipPawn::HandlePitchInput(const FInputActionValue& Value)
{
	if (!IsValid(ShipMovementComponent)) return;
	const float PitchValue = Value.Get<float>();
	ShipMovementComponent->SetPitchInput(PitchValue);
}

void AShipPawn::HandleYawInput(const FInputActionValue& Value)
{
	if (!IsValid(ShipMovementComponent)) return;
	const float YawValue = Value.Get<float>();
	ShipMovementComponent->SetYawInput(YawValue);
}

void AShipPawn::BeginPrimaryFire()
{
	if (!IsValid(CannonComponent)) return;
	CannonComponent->BeginCannonFire(0);
}

void AShipPawn::EndPrimaryFire()
{
	if (!IsValid(CannonComponent)) return;
	CannonComponent->EndCannonFire(0);
}

void AShipPawn::BeginSecondaryFire()
{
	if (!IsValid(CannonComponent)) return;
	CannonComponent->BeginCannonFire(1);
}

void AShipPawn::EndSecondaryFire()
{
	if (!IsValid(CannonComponent)) return;
	CannonComponent->EndCannonFire(1);
}

void AShipPawn::ClearCollisionCooldown()
{
	bIsCollisionCooldown = false;
}

void AShipPawn::OnShipCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsCollisionCooldown) return;
	if (IsValid(OtherActor) && OtherActor != this)
	{
		float Speed = FVector::DotProduct(GetVelocity(), GetActorForwardVector());
		float Damage = FMath::GetMappedRangeValueClamped(FVector2D(ShipMovementComponent->GetMinSpeed(), ShipMovementComponent->GetMaxSpeed()), FVector2D(MinCollisionDamage, MaxCollisionDamage), Speed);

		// Apply Damage To Self
		if (Damage > 0)
		{
			UGameplayStatics::ApplyPointDamage(this, Damage, GetActorLocation(), Hit, GetController(), this, nullptr);
		}

		// Bounce Off Collision
		if (bBounceOffOnCollision)
		{
			const FVector IncomingVector = GetActorForwardVector();
			const FVector ReflectedVector = FMath::GetReflectionVector(IncomingVector, Hit.Normal);
			const FRotator NewRotation = ReflectedVector.Rotation();
			FHitResult HitResult;
			AddActorLocalRotation(NewRotation, true, &HitResult, ETeleportType::TeleportPhysics);
		}

		// Spawn Impact Particle Effects
		if (CollisionImpactParticleEffect)
		{
			if (UNiagaraComponent* SpawnedImpactEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CollisionImpactParticleEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation()))
			{
				SpawnedImpactEffect->SetAutoDestroy(true);
			}
		}

		// Play Camera Shake
		if (ImpactCameraShake)
		{
			 UGameplayStatics::PlayWorldCameraShake(this, ImpactCameraShake, Hit.ImpactPoint, 0.0f, 5000.0f);
		}

		// Begin Cooldown
		bIsCollisionCooldown = true;
		GetWorld()->GetTimerManager().SetTimer(CollisionCooldownTimerHandle, this, &AShipPawn::ClearCollisionCooldown, CollisionCooldownDuration, false);
	}
}

void AShipPawn::OnPawnDied(AController* InstigatedBy, AActor* DamageCauser)
{
	if (ExplosionParticleEffect)
	{
		if (UNiagaraComponent* SpawnedExplosionEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionParticleEffect, GetActorLocation(), GetActorRotation()))
		{
			SpawnedExplosionEffect->SetAutoDestroy(true);
		}
	}

	Destroy();
}

void AShipPawn::OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		DetectedActors.AddUnique(OtherActor);
	}
}

void AShipPawn::OnDetectionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValid(OtherActor) && OtherActor != this)
	{
		DetectedActors.RemoveSingle(OtherActor);
	}
}

FVector AShipPawn::GetClosestCollisionLocation() const
{
	FVector ClosestCollisionLocation = FVector::ZeroVector;
	float MinDistance = FLT_MAX;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	for (AActor* Actor : DetectedActors)
	{
		if (!IsValid(Actor))
		{
			UE_LOG(LogShipPawn, Warning, TEXT("ShipPawn: DetectedActors contains an invalid actor."));
			continue;
		}

		FHitResult HitResult;
		if (GetWorld()->LineTraceSingleByChannel(HitResult, GetActorLocation(), Actor->GetActorLocation(), ECC_Visibility, CollisionParams))
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
