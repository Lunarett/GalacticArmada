#include "Player/ShipPawn.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "AI/AIShipController.h"
#include "Camera/CameraComponent.h"
#include "Components/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ShipMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipPawn, Log, All)

AShipPawn::AShipPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set AI Controller
	AIControllerClass = AAIShipController::StaticClass();

	// Initialize Ship Mesh
	ShipMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetSimulatePhysics(false);
	ShipMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ShipMesh->SetCollisionObjectType(ECC_Vehicle);
	ShipMesh->OnComponentHit.AddDynamic(this, &AShipPawn::OnShipCollision);
	SetRootComponent(ShipMesh);

	// Initialize Spring Arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 2000.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.0f;
	SpringArm->CameraRotationLagSpeed = 10.0f;

	// Initialize Camera Component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArm);

	// Initialize Ship Movement
	ShipMovementComponent = CreateDefaultSubobject<UShipMovementComponent>(TEXT("ShipMovement"));

	// Initialize Cannon
	CannonComponent = CreateDefaultSubobject<UCannonComponent>(TEXT("Cannon"));

	// Initialize Health
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	HealthComponent->SetMaxHealth(100.0f);
	HealthComponent->OnDeath.AddDynamic(this, &AShipPawn::OnPawnDied);
}

FGenericTeamId AShipPawn::GetGenericTeamId() const
{
	return TeamId;
}

void AShipPawn::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId;
}

ETeamAttitude::Type AShipPawn::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* const OtherTeamAgent = Cast<IGenericTeamAgentInterface>(&Other);
	if (!OtherTeamAgent)
	{
		return ETeamAttitude::Neutral;
	}

	const FGenericTeamId OtherTeam = OtherTeamAgent->GetGenericTeamId();

	if (TeamId == FGenericTeamId::NoTeam || OtherTeam == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	return (TeamId == OtherTeam) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void AShipPawn::SetThrottle(const float InThrottle)
{
	if (ShipMovementComponent)
	{
		ShipMovementComponent->SetThrottleInput(InThrottle);
	}
}

void AShipPawn::SetRoll(const float InRoll)
{
	if (ShipMovementComponent)
	{
		ShipMovementComponent->SetRollInput(InRoll);
	}
}

void AShipPawn::SetPitch(const float InPitch)
{
	if (ShipMovementComponent)
	{
		ShipMovementComponent->SetPitchInput(InPitch);
	}
}

void AShipPawn::SetYaw(const float InYaw)
{
	if (ShipMovementComponent)
	{
		ShipMovementComponent->SetYawInput(InYaw);
	}
}

void AShipPawn::BeginPrimaryFire()
{
	if (CannonComponent)
	{
		CannonComponent->BeginCannonFire(0);
	}
}

void AShipPawn::BeginSecondaryFire()
{
	if (CannonComponent)
	{
		CannonComponent->BeginCannonFire(1);
	}
}

void AShipPawn::EndPrimaryFire()
{
	if (CannonComponent)
	{
		CannonComponent->EndCannonFire(0);
	}
}

void AShipPawn::EndSecondaryFire()
{
	if (CannonComponent)
	{
		CannonComponent->EndCannonFire(1);
	}
}

void AShipPawn::ClearCollisionCooldown()
{
	bIsCollisionCooldown = false;
	CollisionCooldownTimerHandle.Invalidate();
}

void AShipPawn::OnShipCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsCollisionCooldown)
	{
		return;
	}

	if (IsValid(OtherActor) && OtherActor != this)
	{
		const float Speed = FVector::DotProduct(GetVelocity(), GetActorForwardVector());
		// const float Damage = FMath::GetMappedRangeValueClamped(
		// 	FVector2D(ShipMovementComponent->GetShipMinSpeed(), ShipMovementComponent->GetMaxSpeed()),
		// 	FVector2D((HealthComponent->GetMaxHealth() / 2.0f), HealthComponent->GetMaxHealth()), Speed);
		//
		// // Apply Damage To Self
		// if (Damage > 0)
		// {
		// 	UGameplayStatics::ApplyPointDamage(this, Damage, GetActorLocation(), Hit, GetController(), this, nullptr);
		// }

		// Spawn Impact Particle Effects
		if (CollisionImpactParticleEffect)
		{
			if (UNiagaraComponent* SpawnedImpactEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, CollisionImpactParticleEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation()))
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
		GetWorld()->GetTimerManager().SetTimer(CollisionCooldownTimerHandle, this, &AShipPawn::ClearCollisionCooldown,
		                                       0.3f, false);
	}
}

void AShipPawn::OnPawnDied(AActor* DeadActor, AController* KillerController, AActor* DamageCauser)
{
	if (ExplosionParticleEffect)
	{
		if (UNiagaraComponent* SpawnedExplosionEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, ExplosionParticleEffect, GetActorLocation(), GetActorRotation()))
		{
			SpawnedExplosionEffect->SetAutoDestroy(true);
		}
	}

	Destroy();
}
