// AITargetSelectionSubsystem.cpp

#include "Systems/AITargetSelectionSubsystem.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogAITargetSelection, Log, All);

void UAITargetSelectionSubsystem::Deinitialize()
{
	// Unbind everything we bound
	for (const TWeakObjectPtr<AActor>& WeakTarget : BoundDestroyedTargets)
	{
		if (AActor* Target = WeakTarget.Get())
		{
			Target->OnDestroyed.RemoveDynamic(this, &UAITargetSelectionSubsystem::HandleTargetDestroyed);
		}
	}

	BoundDestroyedTargets.Empty();
	ClaimerByTeamAndTarget.Empty();
	TargetByClaimer.Empty();
	AgentsByTeam.Empty();
	PlayerChaseBudgetByTeam.Empty();

	Super::Deinitialize();
}

void UAITargetSelectionSubsystem::SetPlayerChaseBudgetForTeam(uint8 TeamId, int32 Budget)
{
	PlayerChaseBudgetByTeam.FindOrAdd(TeamId) = Budget;
}

void UAITargetSelectionSubsystem::RegisterAgent(APawn* AgentPawn, uint8 TeamId)
{
	if (!IsValid(AgentPawn))
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>& Bucket = AgentsByTeam.FindOrAdd(TeamId);

	for (const TWeakObjectPtr<APawn>& Existing : Bucket)
	{
		if (Existing.Get() == AgentPawn)
		{
			return;
		}
	}

	Bucket.Add(AgentPawn);
}

void UAITargetSelectionSubsystem::UnregisterAgent(APawn* AgentPawn)
{
	if (!IsValid(AgentPawn))
	{
		return;
	}

	ReleaseTarget(AgentPawn);

	for (auto& Pair : AgentsByTeam)
	{
		TArray<TWeakObjectPtr<APawn>>& Bucket = Pair.Value;
		for (int32 i = Bucket.Num() - 1; i >= 0; --i)
		{
			if (!Bucket[i].IsValid() || Bucket[i].Get() == AgentPawn)
			{
				Bucket.RemoveAtSwap(i, 1, EAllowShrinking::No);
			}
		}
	}

	for (auto It = AgentsByTeam.CreateIterator(); It; ++It)
	{
		if (It.Value().Num() == 0)
		{
			It.RemoveCurrent();
		}
	}
}

AActor* UAITargetSelectionSubsystem::GetOrAssignTarget(APawn* ClaimerPawn)
{
	if (!IsValid(ClaimerPawn))
	{
		return nullptr;
	}

	CleanupDeadWeakEntries();

	if (TWeakObjectPtr<AActor>* ExistingTargetPtr = TargetByClaimer.Find(ClaimerPawn))
	{
		AActor* ExistingTarget = ExistingTargetPtr->Get();
		if (IsValidTarget(ExistingTarget))
		{
			return ExistingTarget;
		}

		ReleaseTarget(ClaimerPawn);
	}

	const uint8 ClaimerTeamId = ResolveTeamIdFromActor(*ClaimerPawn);

	AActor* NewTarget = PickTargetFor(ClaimerPawn, ClaimerTeamId);
	if (!IsValidTarget(NewTarget))
	{
		return nullptr;
	}

	TargetByClaimer.Add(ClaimerPawn, NewTarget);
	ClaimerByTeamAndTarget.Add(FClaimKey{ClaimerTeamId, NewTarget}, ClaimerPawn);

	BindDestroyedOnce(NewTarget);

	return NewTarget;
}

void UAITargetSelectionSubsystem::ReleaseTarget(APawn* ClaimerPawn)
{
	if (!IsValid(ClaimerPawn))
	{
		return;
	}

	TWeakObjectPtr<AActor>* TargetPtr = TargetByClaimer.Find(ClaimerPawn);
	if (!TargetPtr)
	{
		return;
	}

	AActor* TargetActor = TargetPtr->Get();
	const uint8 TeamId = ResolveTeamIdFromActor(*ClaimerPawn);

	TargetByClaimer.Remove(ClaimerPawn);
	ClaimerByTeamAndTarget.Remove(FClaimKey{TeamId, TargetActor});

	// We keep OnDestroyed bound until the target actually dies (cheap and avoids ref counting).
}

bool UAITargetSelectionSubsystem::IsValidTarget(const AActor* Target) const
{
	return IsValid(Target) && !Target->IsActorBeingDestroyed();
}

uint8 UAITargetSelectionSubsystem::ResolveTeamIdFromActor(const AActor& Actor) const
{
	if (const IGenericTeamAgentInterface* Direct = Cast<IGenericTeamAgentInterface>(&Actor))
	{
		return Direct->GetGenericTeamId().GetId();
	}

	if (const APawn* Pawn = Cast<APawn>(&Actor))
	{
		const AController* Ctrl = Pawn->GetController();
		if (Ctrl)
		{
			if (const IGenericTeamAgentInterface* CtrlTeam = Cast<IGenericTeamAgentInterface>(Ctrl))
			{
				return CtrlTeam->GetGenericTeamId().GetId();
			}
		}
	}

	return FGenericTeamId::NoTeam.GetId();
}

AActor* UAITargetSelectionSubsystem::PickTargetFor(APawn* ClaimerPawn, uint8 ClaimerTeamId) const
{
	APawn* BestUnclaimedPlayer = nullptr;
	APawn* BestUnclaimedAI = nullptr;

	APawn* FallbackPlayer = nullptr;
	APawn* FallbackAI = nullptr;

	for (const auto& TeamPair : AgentsByTeam)
	{
		const uint8 OtherTeamId = TeamPair.Key;
		if (OtherTeamId == ClaimerTeamId)
		{
			continue;
		}

		const TArray<TWeakObjectPtr<APawn>>& Candidates = TeamPair.Value;

		for (const TWeakObjectPtr<APawn>& WeakPawn : Candidates)
		{
			APawn* CandidatePawn = WeakPawn.Get();
			if (!IsValid(CandidatePawn) || CandidatePawn == ClaimerPawn)
			{
				continue;
			}

			const bool bIsPlayer = CandidatePawn->IsPlayerControlled();
			const bool bClaimed = ClaimerByTeamAndTarget.Contains(FClaimKey{ClaimerTeamId, CandidatePawn});

			if (!bClaimed)
			{
				if (bIsPlayer)
				{
					if (!BestUnclaimedPlayer)
					{
						BestUnclaimedPlayer = CandidatePawn;
					}
				}
				else
				{
					if (!BestUnclaimedAI)
					{
						BestUnclaimedAI = CandidatePawn;
					}
				}
			}

			if (bIsPlayer)
			{
				if (!FallbackPlayer)
				{
					FallbackPlayer = CandidatePawn;
				}
			}
			else
			{
				if (!FallbackAI)
				{
					FallbackAI = CandidatePawn;
				}
			}

			if (BestUnclaimedPlayer)
			{
				break;
			}
		}

		if (BestUnclaimedPlayer)
		{
			break;
		}
	}

	if (BestUnclaimedPlayer)
	{
		return BestUnclaimedPlayer;
	}
	if (BestUnclaimedAI)
	{
		return BestUnclaimedAI;
	}
	if (FallbackPlayer)
	{
		return FallbackPlayer;
	}
	return FallbackAI;
}

void UAITargetSelectionSubsystem::BindDestroyedOnce(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> Key(TargetActor);
	if (BoundDestroyedTargets.Contains(Key))
	{
		return;
	}

	TargetActor->OnDestroyed.AddDynamic(this, &UAITargetSelectionSubsystem::HandleTargetDestroyed);
	BoundDestroyedTargets.Add(Key);
}

void UAITargetSelectionSubsystem::UnbindDestroyed(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> Key(TargetActor);
	if (!BoundDestroyedTargets.Contains(Key))
	{
		return;
	}

	TargetActor->OnDestroyed.RemoveDynamic(this, &UAITargetSelectionSubsystem::HandleTargetDestroyed);
	BoundDestroyedTargets.Remove(Key);
}

void UAITargetSelectionSubsystem::HandleTargetDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor)
	{
		return;
	}

	// Remove claimer -> target entries pointing at this
	for (auto It = TargetByClaimer.CreateIterator(); It; ++It)
	{
		if (It.Value().Get() == DestroyedActor)
		{
			It.RemoveCurrent();
		}
	}

	// Remove reservations pointing at this
	for (auto It = ClaimerByTeamAndTarget.CreateIterator(); It; ++It)
	{
		if (It.Key().Target.Get() == DestroyedActor)
		{
			It.RemoveCurrent();
		}
	}

	UnbindDestroyed(DestroyedActor);
}

void UAITargetSelectionSubsystem::CleanupDeadWeakEntries()
{
	for (auto& Pair : AgentsByTeam)
	{
		TArray<TWeakObjectPtr<APawn>>& Bucket = Pair.Value;
		for (int32 i = Bucket.Num() - 1; i >= 0; --i)
		{
			if (!Bucket[i].IsValid())
			{
				Bucket.RemoveAtSwap(i, 1, EAllowShrinking::No);
			}
		}
	}

	for (auto It = TargetByClaimer.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !IsValidTarget(It.Value().Get()))
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = ClaimerByTeamAndTarget.CreateIterator(); It; ++It)
	{
		if (!It.Key().Target.IsValid() || !It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = BoundDestroyedTargets.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}