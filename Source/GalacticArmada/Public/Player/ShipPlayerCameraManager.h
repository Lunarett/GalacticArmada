// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ShipPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class GALACTICARMADA_API AShipPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

protected:
	virtual void UpdateCamera(float DeltaTime) override;
};
