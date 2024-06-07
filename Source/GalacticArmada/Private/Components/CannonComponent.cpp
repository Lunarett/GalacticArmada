#include "Components/CannonComponent.h"
#include "Actors/ProjectileBase.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UCannonComponent::UCannonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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

	// Clear all timers on end play
	for (FTimerHandle& TimerHandle : AutomaticFireTimers)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
}

void UCannonComponent::BeginCannonFire(int32 CannonIndex)
{
	if (!CannonFirePropertiesArray.IsValidIndex(CannonIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid CannonIndex: %d"), CannonIndex);
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
		switch (CannonFireProps.CannonFireMode)
		{
		case ECannonFireMode::All:
			FireAllCannons(CannonFireProps);
			break;
		case ECannonFireMode::Sequential:
			FireSequentialCannon(CannonFireProps, SequentialCannonIndices[CannonIndex]);
			break;
		}
	}
}

void UCannonComponent::EndCannonFire(int32 CannonIndex)
{
	if (CannonFirePropertiesArray.IsValidIndex(CannonIndex))
	{
		StopAutomaticFire(CannonIndex);
	}
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
			World->SpawnActor<AProjectileBase>(CannonFireProps.ProjectileClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator());
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
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = Cast<APawn>(GetOwner());
			World->SpawnActor<AProjectileBase>(CannonFireProps.ProjectileClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
		}
	}

	// Increment and loop the index
	CurrentCannonIndex = (CurrentCannonIndex + 1) % CannonFireProps.FireLocationSocketNames.Num();
}

void UCannonComponent::StartAutomaticFire(int32 CannonIndex)
{
	if (AutomaticFireTimers[CannonIndex].IsValid()) return;

	const FCannonFireProperties& CannonFireProps = CannonFirePropertiesArray[CannonIndex];
	GetWorld()->GetTimerManager().SetTimer(
		AutomaticFireTimers[CannonIndex],
		FTimerDelegate::CreateUObject(this, &UCannonComponent::AutomaticFire, CannonIndex),
		CannonFireProps.FireRate,
		true
	);
}

void UCannonComponent::StopAutomaticFire(int32 CannonIndex)
{
	if (AutomaticFireTimers[CannonIndex].IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutomaticFireTimers[CannonIndex]);
	}
}

void UCannonComponent::AutomaticFire(int32 CannonIndex)
{
	if (!CannonFirePropertiesArray.IsValidIndex(CannonIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid CannonIndex for automatic fire: %d"), CannonIndex);
		return;
	}

	const FCannonFireProperties& CannonFireProps = CannonFirePropertiesArray[CannonIndex];
	if (!CannonFireProps.Enabled || !OwnerSkeletalMeshComponent)
	{
		StopAutomaticFire(CannonIndex);
		return;
	}

	switch (CannonFireProps.CannonFireMode)
	{
	case ECannonFireMode::All:
		FireAllCannons(CannonFireProps);
		break;
	case ECannonFireMode::Sequential:
		FireSequentialCannon(CannonFireProps, SequentialCannonIndices[CannonIndex]);
		break;
	}
}