// ShipSpawnSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ShipSpawnSubsystem.generated.h"

class AShipPawn;

USTRUCT()
struct FShipTeamBucket
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AShipPawn>> Ships;
};

UCLASS()
class GALACTICARMADA_API UShipSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	AShipPawn* SpawnShip(
		TSubclassOf<AShipPawn> ShipClass,
		const FTransform& Transform,
		uint8 TeamId,
		bool bSpawnController
	);

	void DespawnShip(AShipPawn* Ship);

	const TArray<TObjectPtr<AShipPawn>>& GetAllShips() const { return AllShips; }
	const TArray<TObjectPtr<AShipPawn>>& GetShipsForTeam(uint8 TeamId) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<AShipPawn>> AllShips;

	UPROPERTY()
	TMap<uint8, FShipTeamBucket> ShipsByTeam;

private:
	void RegisterShip(AShipPawn* Ship, uint8 TeamId);
	void UnregisterShip(AShipPawn* Ship);

	static void ApplyTeamToPawnAndController(AShipPawn* Ship, uint8 TeamId);
};