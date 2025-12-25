// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PooledActorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UPooledActorInterface : public UInterface
{
	GENERATED_BODY()
};

class GALACTICARMADA_API IPooledActorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category="Pooling")
	void OnPooledActorActivated();

	UFUNCTION(BlueprintNativeEvent, Category="Pooling")
	void OnPooledActorDeactivated();
public:
};
