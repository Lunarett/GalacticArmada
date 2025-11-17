// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShipPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GALACTICARMADA_API AShipPlayerState : public APlayerState
{
	GENERATED_BODY()

private:
	UPROPERTY()
	int32 Kills;

	UPROPERTY()
	int32 Deaths;


	UPROPERTY()
	int32 TeamID;

public:
	void AddKill();
	void AddDeath();
	void SetTeamID(const int32 InTeamID);

	FORCEINLINE int32 GetKills() const { return Kills; }
	FORCEINLINE int32 GetDeaths() const { return Deaths; }
	FORCEINLINE int32 GetTeamID() const { return TeamID; }
};
