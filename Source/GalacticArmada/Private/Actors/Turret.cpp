// Turret.cpp

#include "Actors/Turret.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/HealthComponent.h"
#include "Actors/ProjectileBase.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"

ATurret::ATurret()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

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
	TargetDetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TargetDetectionSphere->SetGenerateOverlapEvents(true);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void ATurret::BeginPlay()
{
	Super::BeginPlay();

	TargetDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATurret::OnTargetEnter);
	TargetDetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ATurret::OnTargetExit);

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ATurret::HandleDeath);
	}

	GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&ATurret::Fire,
		FireRate,
		true
	);
}

void ATurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CurrentTarget)
	{
		SelectBestTarget();
	}

	RotateTowardsTarget(DeltaTime);
}

// --------------------------
// Team logic
// --------------------------

ETeamAttitude::Type ATurret::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(&Other);
	if (!OtherTeamAgent)
	{
		return ETeamAttitude::Neutral;
	}

	if (TeamId == OtherTeamAgent->GetGenericTeamId())
	{
		return ETeamAttitude::Friendly;
	}

	return ETeamAttitude::Hostile;
}

bool ATurret::IsEnemy(const AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == this)
	{
		return false;
	}

	return GetTeamAttitudeTowards(*OtherActor) == ETeamAttitude::Hostile;
}

// --------------------------
// Overlaps
// --------------------------

void ATurret::OnTargetEnter(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	int32,
	bool,
	const FHitResult&
)
{
	if (IsEnemy(OtherActor))
	{
		EnemyTargets.Add(OtherActor);
	}
}

void ATurret::OnTargetExit(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	int32
)
{
	EnemyTargets.Remove(OtherActor);

	if (CurrentTarget == OtherActor)
	{
		CurrentTarget = nullptr;
	}
}

// --------------------------
// Targeting / firing
// --------------------------

void ATurret::SelectBestTarget()
{
	float ClosestSq = TNumericLimits<float>::Max();
	AActor* Best = nullptr;

	const FVector MyLocation = GetActorLocation();

	for (AActor* Target : EnemyTargets)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLocation, Target->GetActorLocation());
		if (DistSq < ClosestSq)
		{
			ClosestSq = DistSq;
			Best = Target;
		}
	}

	CurrentTarget = Best;
}

void ATurret::RotateTowardsTarget(float DeltaTime)
{
	if (!CurrentTarget)
	{
		return;
	}

	const FVector TargetLocation = CurrentTarget->GetActorLocation();

	const FRotator DesiredYaw =
		UKismetMathLibrary::FindLookAtRotation(YawMesh->GetComponentLocation(), TargetLocation);

	const float NewYaw = FMath::FInterpConstantTo(
		YawMesh->GetComponentRotation().Yaw,
		DesiredYaw.Yaw + YawRotationOffset,
		DeltaTime,
		YawSpeed
	);

	YawMesh->SetWorldRotation(FRotator(0.f, NewYaw, 0.f));

	const FRotator DesiredPitch =
		UKismetMathLibrary::FindLookAtRotation(PitchMesh->GetComponentLocation(), TargetLocation);

	const float NewPitch = FMath::FInterpConstantTo(
		PitchMesh->GetComponentRotation().Roll,
		- ClampPitch(DesiredPitch.Pitch),
		DeltaTime,
		PitchSpeed
	);

	PitchMesh->SetRelativeRotation(FRotator(0.f, 0.f, NewPitch));
}

bool ATurret::IsTargetInLineOfFire() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	const FVector ToTarget =
		(CurrentTarget->GetActorLocation() - FirePoint->GetComponentLocation()).GetSafeNormal();

	const float Dot = FVector::DotProduct(FirePoint->GetForwardVector(), ToTarget);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	return AngleDeg <= FireAlignmentTolerance;
}

void ATurret::Fire()
{
	if (!CurrentTarget || !IsTargetInLineOfFire())
	{
		return;
	}

	if (ProjectileClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = nullptr;

		GetWorld()->SpawnActor<AProjectileBase>(
			ProjectileClass,
			FirePoint->GetComponentLocation(),
			FirePoint->GetComponentRotation(),
			Params
		);
	}

	if (MuzzleEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			MuzzleEffect,
			FirePoint->GetComponentLocation(),
			FirePoint->GetComponentRotation()
		);
	}
}

void ATurret::HandleDeath(AActor*, AController*, AActor*)
{
	if (DeathEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DeathEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	Destroy();
}

float ATurret::ClampPitch(float DesiredPitch) const
{
	return FMath::Clamp(DesiredPitch, MinPitch, MaxPitch);
}