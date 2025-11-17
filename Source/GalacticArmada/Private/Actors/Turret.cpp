#include "Actors/Turret.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Actors/ProjectileBase.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/HealthComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/ShipPlayerState.h"

ATurret::ATurret()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);

	YawMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YawMesh"));
	YawMesh->SetupAttachment(BaseMesh);

	PitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PitchMesh"));
	PitchMesh->SetupAttachment(YawMesh);

	FirePoint = CreateDefaultSubobject<USceneComponent>(TEXT("FirePoint"));
	FirePoint->SetupAttachment(PitchMesh);

	TargetDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TargetDetectionSphere"));
	TargetDetectionSphere->SetupAttachment(Root);
	TargetDetectionSphere->SetSphereRadius(TargetingRange);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void ATurret::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent != nullptr)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ATurret::HandleDeath);
	}

	GetWorld()->GetTimerManager().SetTimer(FireTimerHandle, this, &ATurret::Fire, FireRate, true);
}

void ATurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SearchForTarget();
	RotateTowardsTarget(DeltaTime);
}

void ATurret::SearchForTarget()
{
	TArray<AActor*> OverlappingActors;
	TargetDetectionSphere->GetOverlappingActors(OverlappingActors);

	float ClosestDistance = TargetingRange;
	AActor* ClosestEnemy = nullptr;

	for (AActor* Actor : OverlappingActors)
	{
		if (IsEnemy(Actor))
		{
			const float Distance = FVector::Dist(Actor->GetActorLocation(), GetActorLocation());
			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestEnemy = Actor;
			}
		}
	}

	CurrentTarget = ClosestEnemy;
}

void ATurret::RotateTowardsTarget(float DeltaTime)
{
	if (CurrentTarget == nullptr)
	{
		return;
	}

	const FVector YawBaseLocation = YawMesh->GetComponentLocation();
	const FVector PitchBaseLocation = PitchMesh->GetComponentLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();

	// Calculate ideal rotations
	const FRotator DesiredYawRotation = UKismetMathLibrary::FindLookAtRotation(YawBaseLocation, TargetLocation);
	const FRotator DesiredPitchRotation = UKismetMathLibrary::FindLookAtRotation(PitchBaseLocation, TargetLocation);

	// Apply yaw with offset correction
	const float CurrentYaw = YawMesh->GetComponentRotation().Yaw;
	const float TargetYaw = DesiredYawRotation.Yaw + YawRotationOffset;
	const float NewYaw = FMath::FInterpConstantTo(CurrentYaw, TargetYaw, DeltaTime, YawSpeed);

	YawMesh->SetWorldRotation(FRotator(0.0f, NewYaw, 0.0f));

	// Apply pitch (roll of PitchMesh)
	const float CurrentRoll = PitchMesh->GetComponentRotation().Roll;
	const float TargetRoll = -ClampPitch(DesiredPitchRotation.Pitch); // Negative because Roll is used
	const float NewRoll = FMath::FInterpConstantTo(CurrentRoll, TargetRoll, DeltaTime, PitchSpeed);

	PitchMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, NewRoll));
}

bool ATurret::IsTargetInLineOfFire() const
{
	if (CurrentTarget == nullptr)
	{
		return false;
	}

	const FVector MuzzleLocation = FirePoint->GetComponentLocation();
	const FVector Direction = FirePoint->GetForwardVector();
	const FVector ToTarget = (CurrentTarget->GetActorLocation() - MuzzleLocation).GetSafeNormal();

	const float AimAngle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Direction, ToTarget)));
	return AimAngle <= FireAlignmentTolerance;
}

void ATurret::Fire()
{
	if (CurrentTarget == nullptr)
	{
		return;
	}

	if (!IsTargetInLineOfFire())
	{
		return;
	}

	if (ProjectileClass != nullptr)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = GetInstigator();

		GetWorld()->SpawnActor<AProjectileBase>(ProjectileClass, FirePoint->GetComponentLocation(), FirePoint->GetComponentRotation(), Params);
	}

	if (MuzzleEffect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleEffect, FirePoint->GetComponentLocation(), FirePoint->GetComponentRotation());
	}
}

void ATurret::HandleDeath(AActor* DeadActor, AController* InstigatedBy, AActor* DamageCauser)
{
	if (DeathEffect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathEffect, GetActorLocation(), GetActorRotation());
	}

	Destroy();
}

bool ATurret::IsEnemy(const AActor* OtherActor) const
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return false;
	}

	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr)
	{
		return false;
	}

	const AShipPlayerState* ShipPlayerState = Cast<AShipPlayerState>(OtherPawn->GetPlayerState());
	if (ShipPlayerState == nullptr)
	{
		return false;
	}

	return ShipPlayerState->GetTeamID() != TeamID;
}

float ATurret::ClampPitch(float DesiredPitch) const
{
	return FMath::Clamp(DesiredPitch, MinPitch, MaxPitch);
}

float ATurret::CalculateShortestYaw(float Current, float Target) const
{
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(Current, Target);
	return Current + DeltaYaw;
}