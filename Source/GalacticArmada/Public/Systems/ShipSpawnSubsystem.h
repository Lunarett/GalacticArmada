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
	AShipPawn* SpawnShip(TSubclassOf<AShipPawn> ShipClass, const FTransform& Transform, int32 TeamId, bool bSpawnController);
	void DespawnShip(AShipPawn* Ship);

	const TArray<TObjectPtr<AShipPawn>>& GetAllShips() const { return AllShips; }

	const TArray<TObjectPtr<AShipPawn>>& GetShipsForTeam(int32 TeamId) const;
	void GetEnemyShips(int32 TeamId, TArray<AShipPawn*>& OutEnemies) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<AShipPawn>> AllShips;

	UPROPERTY()
	TMap<int32, FShipTeamBucket> ShipsByTeam;

	UPROPERTY()
	TMap<TObjectPtr<AShipPawn>, int32> ShipToTeamId;

	UPROPERTY()
	TMap<TObjectPtr<AShipPawn>, int32> ShipToTeamIndex;

	void RegisterShip(AShipPawn* Ship, int32 TeamId);
	void UnregisterShip(AShipPawn* Ship);
};