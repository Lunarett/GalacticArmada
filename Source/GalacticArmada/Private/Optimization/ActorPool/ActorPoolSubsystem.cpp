// ActorPoolSubsystem.cpp

#include "Optimization/ActorPool/ActorPoolSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Optimization/ActorPool/PooledActorInterface.h"

void UActorPoolSubsystem::InitializePool(UClass* ActorClass, int32 Count)
{
	if (!ActorClass || !GetWorld() || Count <= 0)
	{
		return;
	}

	FActorPool& Pool = Pools.FindOrAdd(ActorClass);

	for (int32 i = 0; i < Count; ++i)
	{
		AActor* Actor = GetWorld()->SpawnActor<AActor>(ActorClass, FTransform::Identity);
		if (!Actor)
		{
			continue;
		}

		SetActorInactive(Actor);
		Pool.InactiveActors.Add(Actor);
	}
}

AActor* UActorPoolSubsystem::ActivateActor(UClass* ActorClass, const FTransform& SpawnTransform)
{
	if (!ActorClass || !GetWorld())
	{
		return nullptr;
	}

	FActorPool& Pool = Pools.FindOrAdd(ActorClass);

	AActor* Actor = nullptr;

	if (Pool.InactiveActors.Num() > 0)
	{
		Actor = Pool.InactiveActors.Pop(EAllowShrinking::No);
	}
	else
	{
		Actor = GetWorld()->SpawnActor<AActor>(ActorClass, SpawnTransform);
	}

	if (!Actor)
	{
		return nullptr;
	}

	Pool.ActiveActors.Add(Actor);

	SetActorActive(Actor, SpawnTransform);

	if (Actor->Implements<UPooledActorInterface>())
	{
		IPooledActorInterface::Execute_OnPooledActorActivated(Actor);
	}

	return Actor;
}

void UActorPoolSubsystem::DeactivateActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	UClass* ActorClass = Actor->GetClass();
	FActorPool* Pool = Pools.Find(ActorClass);

	if (!Pool)
	{
		Actor->Destroy();
		return;
	}

	Pool->ActiveActors.RemoveSwap(Actor, EAllowShrinking::No);

	if (Actor->Implements<UPooledActorInterface>())
	{
		IPooledActorInterface::Execute_OnPooledActorDeactivated(Actor);
	}

	SetActorInactive(Actor);
	Pool->InactiveActors.Add(Actor);
}

void UActorPoolSubsystem::SetActorActive(AActor* Actor, const FTransform& Transform)
{
	Actor->SetActorTransform(Transform);
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);
}

void UActorPoolSubsystem::SetActorInactive(AActor* Actor)
{
	Actor->SetActorTickEnabled(false);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorHiddenInGame(true);
}