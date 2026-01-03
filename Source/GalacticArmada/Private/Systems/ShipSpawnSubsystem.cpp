#include "Systems/ShipSpawnSubsystem.h"
#include "Components/HealthComponent.h"
#include "Engine/World.h"
#include "Optimization/ActorPool/ActorPoolSubsystem.h"
#include "Player/ShipPawn.h"

AShipPawn* UShipSpawnSubsystem::SpawnShip(TSubclassOf<AShipPawn> ShipClass, const FTransform& Transform, int32 TeamId, bool bSpawnController)
{
	if (!GetWorld() || !ShipClass)
	{
		return nullptr;
	}

	UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	if (!Pool)
	{
		return nullptr;
	}

	AActor* Activated = Pool->ActivateActor(*ShipClass, Transform);
	AShipPawn* Ship = Cast<AShipPawn>(Activated);

	if (!Ship)
	{
		if (Activated)
		{
			Pool->DeactivateActor(Activated);
		}
		return nullptr;
	}

	if (UHealthComponent* Health = Ship->FindComponentByClass<UHealthComponent>())
	{
		//Health->SetTeamId(TeamId);
	}

	RegisterShip(Ship, TeamId);

	if (bSpawnController && Ship->GetController() == nullptr)
	{
		Ship->SpawnDefaultController();
	}

	return Ship;
}

void UShipSpawnSubsystem::DespawnShip(AShipPawn* Ship)
{
	if (!GetWorld() || !Ship)
	{
		return;
	}

	UnregisterShip(Ship);

	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		Pool->DeactivateActor(Ship);
	}
	else
	{
		Ship->Destroy();
	}
}

const TArray<TObjectPtr<AShipPawn>>& UShipSpawnSubsystem::GetShipsForTeam(int32 TeamId) const
{
	if (const FShipTeamBucket* Bucket = ShipsByTeam.Find(TeamId))
	{
		return Bucket->Ships;
	}

	static const TArray<TObjectPtr<AShipPawn>> Empty;
	return Empty;
}

void UShipSpawnSubsystem::GetEnemyShips(int32 TeamId, TArray<AShipPawn*>& OutEnemies) const
{
	OutEnemies.Reset();

	for (const auto& Pair : ShipsByTeam)
	{
		if (Pair.Key == TeamId)
		{
			continue;
		}

		for (AShipPawn* Ship : Pair.Value.Ships)
		{
			if (IsValid(Ship))
			{
				OutEnemies.Add(Ship);
			}
		}
	}
}

void UShipSpawnSubsystem::RegisterShip(AShipPawn* Ship, int32 TeamId)
{
	if (!Ship)
	{
		return;
	}

	AllShips.Add(Ship);

	FShipTeamBucket& Bucket = ShipsByTeam.FindOrAdd(TeamId);
	const int32 NewIndex = Bucket.Ships.Add(Ship);

	ShipToTeamId.Add(Ship, TeamId);
	ShipToTeamIndex.Add(Ship, NewIndex);
}

void UShipSpawnSubsystem::UnregisterShip(AShipPawn* Ship)
{
	if (!Ship)
	{
		return;
	}

	AllShips.RemoveSwap(Ship);

	const int32* TeamIdPtr = ShipToTeamId.Find(Ship);
	const int32* IndexPtr = ShipToTeamIndex.Find(Ship);

	if (!TeamIdPtr || !IndexPtr)
	{
		// Fallback: safe slow removal if maps got desynced
		for (auto& Pair : ShipsByTeam)
		{
			Pair.Value.Ships.RemoveSwap(Ship);
		}

		ShipToTeamId.Remove(Ship);
		ShipToTeamIndex.Remove(Ship);
		return;
	}

	const int32 TeamId = *TeamIdPtr;
	const int32 Index = *IndexPtr;

	FShipTeamBucket* BucketPtr = ShipsByTeam.Find(TeamId);
	if (!BucketPtr)
	{
		ShipToTeamId.Remove(Ship);
		ShipToTeamIndex.Remove(Ship);
		return;
	}

	TArray<TObjectPtr<AShipPawn>>& TeamShips = BucketPtr->Ships;

	if (!TeamShips.IsValidIndex(Index) || TeamShips[Index] != Ship)
	{
		// Safety fallback: index mismatch
		TeamShips.RemoveSwap(Ship);
	}
	else
	{
		const int32 LastIndex = TeamShips.Num() - 1;

		if (Index != LastIndex)
		{
			AShipPawn* SwappedShip = TeamShips[LastIndex];
			TeamShips[Index] = SwappedShip;

			ShipToTeamIndex.FindOrAdd(SwappedShip) = Index;
		}

		TeamShips.Pop(false);
	}

	ShipToTeamId.Remove(Ship);
	ShipToTeamIndex.Remove(Ship);

	if (TeamShips.Num() == 0)
	{
		ShipsByTeam.Remove(TeamId);
	}
}