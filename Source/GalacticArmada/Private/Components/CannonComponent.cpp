#include "Components/CannonComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "Actors/ProjectileBase.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

UCannonComponent::UCannonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	if (GetOwner())
	{
		ProjectileSpawnParams.Owner = GetOwner();
		PawnOwner = Cast<APawn>(GetOwner());
		if (PawnOwner)
		{
			ProjectileSpawnParams.Instigator = PawnOwner;
		}
	}
}

void UCannonComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get Skeletal Mesh
	if (const AActor* Owner = GetOwner())
	{
		OwnerSkeletalMeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
	
	// Initialize Sequential Cannon Mode
	SequentialCannonIndices.SetNum(CannonFirePropertiesArray.Num());
	for (int32 i = 0; i < SequentialCannonIndices.Num(); ++i)
	{
		SequentialCannonIndices[i] = 0;
	}

	// Initialize Automatic Fire Timers
	AutomaticFireTimers.SetNum(CannonFirePropertiesArray.Num());
}

void UCannonComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Clear All Timers
	for (FTimerHandle& TimerHandle : AutomaticFireTimers)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
}

void UCannonComponent::BeginCannonFire(int32 CannonIndex)
{
	if (!CannonFirePropertiesArray.IsValidIndex(CannonIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("CannonComponent: Failed to fire, Invalid CannonIndex: %d"), CannonIndex);
		return;
	}

	const FCannonFireProperties& CannonFireProps = CannonFirePropertiesArray[CannonIndex];
	if (!CannonFireProps.Enabled || !OwnerSkeletalMeshComponent)
	{
		return;
	}

	if (CannonFireProps.IsAutomaticFire)
	{
		StartAutomaticFire(CannonIndex);
	}
	else
	{
		FireCannon(CannonIndex);
	}
}

void UCannonComponent::EndCannonFire(int32 CannonIndex)
{
	if (CannonFirePropertiesArray.IsValidIndex(CannonIndex))
	{
		StopAutomaticFire(CannonIndex);
	}
}

void UCannonComponent::FireCannon(int32 CannonIndex)
{
	if (!CannonFirePropertiesArray.IsValidIndex(CannonIndex)) return;

	const FCannonFireProperties& CannonFireProps = CannonFirePropertiesArray[CannonIndex];

	switch (CannonFireProps.CannonFireMode)
	{
	case ECannonFireMode::All:
		FireAllCannons(CannonFireProps);
		break;
	case ECannonFireMode::Sequential:
		FireSequentialCannon(CannonFireProps, SequentialCannonIndices[CannonIndex]);
		break;
	}

	// Play Fire Camera Shake
	if (FireCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(this, FireCameraShake, GetOwner()->GetActorLocation(), 0.0f, 1000.0f);
	}

	// Broadcast Fire Event
	OnCannonFired.Broadcast(CannonIndex);
}

void UCannonComponent::FireAllCannons(const FCannonFireProperties& CannonFireProps) const
{
	for (const FName& SocketName : CannonFireProps.FireLocationSocketNames)
	{
		const USkeletalMeshSocket* Socket = OwnerSkeletalMeshComponent->GetSocketByName(SocketName);
		if (!Socket || !CannonFireProps.ProjectileClass) continue;

		FTransform SocketTransform = Socket->GetSocketTransform(OwnerSkeletalMeshComponent);
		if (UWorld* World = GetWorld())
		{
			// Spawn Projectile
			World->SpawnActor<AProjectileBase>(CannonFireProps.ProjectileClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), ProjectileSpawnParams);

			// Spawn Muzzle Effect
			if (CannonFireProps.MuzzleParticleEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(CannonFireProps.MuzzleParticleEffect, OwnerSkeletalMeshComponent, SocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
			}
		}
	}
}

void UCannonComponent::FireSequentialCannon(const FCannonFireProperties& CannonFireProps, int32& CurrentCannonIndex) const
{
	if (CannonFireProps.FireLocationSocketNames.Num() == 0) return;

	const FName& SocketName = CannonFireProps.FireLocationSocketNames[CurrentCannonIndex];
	const USkeletalMeshSocket* Socket = OwnerSkeletalMeshComponent->GetSocketByName(SocketName);

	if (Socket && CannonFireProps.ProjectileClass)
	{
		const FTransform SocketTransform = Socket->GetSocketTransform(OwnerSkeletalMeshComponent);
		if (UWorld* World = GetWorld())
		{
			// Spawn Projectile
			World->SpawnActor<AProjectileBase>(CannonFireProps.ProjectileClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), ProjectileSpawnParams);

			// Spawn Muzzle Effect
			if (CannonFireProps.MuzzleParticleEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(CannonFireProps.MuzzleParticleEffect, OwnerSkeletalMeshComponent, SocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
			}
		}
	}

	// Increment Index
	CurrentCannonIndex = (CurrentCannonIndex + 1) % CannonFireProps.FireLocationSocketNames.Num();
}

void UCannonComponent::StartAutomaticFire(int32 CannonIndex)
{
	if (AutomaticFireTimers[CannonIndex].IsValid()) return;

	const FCannonFireProperties& CannonFireProps = CannonFirePropertiesArray[CannonIndex];
	const float TimeBetweenShots = 60 / CannonFireProps.FireRate;
	const float FirstDelay = FMath::Max(GetWorld()->GetTimeSeconds() + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorld()->GetTimerManager().SetTimer(
		AutomaticFireTimers[CannonIndex],
		FTimerDelegate::CreateUObject(this, &UCannonComponent::FireCannon, CannonIndex),
		TimeBetweenShots,
		true,
		FirstDelay
	);
}

void UCannonComponent::StopAutomaticFire(int32 CannonIndex)
{
	if (AutomaticFireTimers[CannonIndex].IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutomaticFireTimers[CannonIndex]);
	}
}