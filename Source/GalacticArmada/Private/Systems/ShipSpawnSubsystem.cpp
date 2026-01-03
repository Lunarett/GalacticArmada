// ShipSpawnSubsystem.cpp

#include "Systems/ShipSpawnSubsystem.h"

#include "AIController.h"
#include "Engine/World.h"
#include "GenericTeamAgentInterface.h"
#include "Optimization/ActorPool/ActorPoolSubsystem.h"
#include "Player/ShipPawn.h"

AShipPawn* UShipSpawnSubsystem::SpawnShip(
	TSubclassOf<AShipPawn> ShipClass,
	const FTransform& Transform,
	uint8 TeamId,
	bool bSpawnController
)
{
	if (!ShipClass || !GetWorld())
	{
		return nullptr;
	}

	UActorPoolSubsystem* const Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	if (!Pool)
	{
		return nullptr;
	}

	AActor* const ActivatedActor = Pool->ActivateActor(*ShipClass, Transform);
	AShipPawn* const Ship = Cast<AShipPawn>(ActivatedActor);

	if (!Ship)
	{
		if (ActivatedActor)
		{
			Pool->DeactivateActor(ActivatedActor);
		}
		return nullptr;
	}

	RegisterShip(Ship, TeamId);

	if (bSpawnController && Ship->GetController() == nullptr)
	{
		Ship->SpawnDefaultController();
	}

	ApplyTeamToPawnAndController(Ship, TeamId);

	return Ship;
}

void UShipSpawnSubsystem::DespawnShip(AShipPawn* Ship)
{
	if (!Ship || !GetWorld())
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

const TArray<TObjectPtr<AShipPawn>>& UShipSpawnSubsystem::GetShipsForTeam(uint8 TeamId) const
{
	if (const FShipTeamBucket* Bucket = ShipsByTeam.Find(TeamId))
	{
		return Bucket->Ships;
	}

	static const TArray<TObjectPtr<AShipPawn>> Empty;
	return Empty;
}

void UShipSpawnSubsystem::RegisterShip(AShipPawn* Ship, uint8 TeamId)
{
	if (!Ship)
	{
		return;
	}

	AllShips.Add(Ship);
	ShipsByTeam.FindOrAdd(TeamId).Ships.Add(Ship);
}

void UShipSpawnSubsystem::UnregisterShip(AShipPawn* Ship)
{
	if (!Ship)
	{
		return;
	}

	AllShips.RemoveSwap(Ship);

	for (auto It = ShipsByTeam.CreateIterator(); It; ++It)
	{
		It.Value().Ships.RemoveSwap(Ship);

		if (It.Value().Ships.Num() == 0)
		{
			It.RemoveCurrent();
		}
	}
}

void UShipSpawnSubsystem::ApplyTeamToPawnAndController(AShipPawn* Ship, uint8 TeamId)
{
	if (!Ship)
	{
		return;
	}

	const FGenericTeamId NewTeam(TeamId);

	if (AController* Controller = Ship->GetController())
	{
		if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Controller))
		{
			TeamAgent->SetGenericTeamId(NewTeam);
		}
	}

	if (IGenericTeamAgentInterface* PawnTeamAgent = Cast<IGenericTeamAgentInterface>(Ship))
	{
		PawnTeamAgent->SetGenericTeamId(NewTeam);
	}
}