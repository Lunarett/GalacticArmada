// ActorPoolSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> InactiveActors;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActiveActors;
};

UCLASS()
class GALACTICARMADA_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void InitializePool(UClass* ActorClass, int32 Count);

	AActor* ActivateActor(UClass* ActorClass, const FTransform& SpawnTransform);
	void DeactivateActor(AActor* Actor);

private:
	UPROPERTY()
	TMap<TObjectPtr<UClass>, FActorPool> Pools;

	void SetActorActive(AActor* Actor, const FTransform& Transform);
	void SetActorInactive(AActor* Actor);
};