#include "Systems/AICommandSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"

void UAICommandSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Agents.Reset();
	PrimaryClaimerByTarget.Reset();
	PlayerPawnsByTeam.Reset();
	CurrentPlayerChasersByTeam.Reset();
	PlayerChaseBudgetByTeam.Reset();
	AttackersByTarget.Reset();

	CleanupAccum = 0.0f;
}

void UAICommandSubsystem::Deinitialize()
{
	Agents.Reset();
	PrimaryClaimerByTarget.Reset();
	PlayerPawnsByTeam.Reset();
	CurrentPlayerChasersByTeam.Reset();
	PlayerChaseBudgetByTeam.Reset();
	AttackersByTarget.Reset();

	Super::Deinitialize();
}

TStatId UAICommandSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAICommandSubsystem, STATGROUP_Tickables);
}

void UAICommandSubsystem::Tick(float DeltaTime)
{
	CleanupAccum += DeltaTime;

	if (CleanupAccum < CleanupInterval)
	{
		return;
	}

	CleanupAccum = 0.0f;
	CleanupInvalid();
}

bool UAICommandSubsystem::IsValidCombatPawn(const APawn* Pawn) const
{
	if (!IsValid(Pawn))
	{
		return false;
	}

	if (Pawn->IsActorBeingDestroyed())
	{
		return false;
	}

	return true;
}

bool UAICommandSubsystem::IsValidTargetActor(const AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	if (Target->IsActorBeingDestroyed())
	{
		return false;
	}

	return true;
}

void UAICommandSubsystem::SetPlayerChaseBudgetForTeam(const uint8 TeamId, const int32 Budget)
{
	const int32 Clamped = FMath::Max(0, Budget);
	PlayerChaseBudgetByTeam.FindOrAdd(TeamId) = Clamped;
}

int32 UAICommandSubsystem::GetPlayerChaseBudgetForTeam(const uint8 TeamId) const
{
	const int32* Found = PlayerChaseBudgetByTeam.Find(TeamId);
	if (!Found)
	{
		return 0;
	}

	return FMath::Max(0, *Found);
}

int32 UAICommandSubsystem::GetCurrentPlayerChasersForTeam(const uint8 TeamId) const
{
	const int32* Found = CurrentPlayerChasersByTeam.Find(TeamId);
	if (!Found)
	{
		return 0;
	}

	return FMath::Max(0, *Found);
}

void UAICommandSubsystem::IncrementPlayerChasers(const uint8 TeamId)
{
	int32& Count = CurrentPlayerChasersByTeam.FindOrAdd(TeamId);
	Count = FMath::Max(0, Count + 1);
}

void UAICommandSubsystem::DecrementPlayerChasers(const uint8 TeamId)
{
	int32* Found = CurrentPlayerChasersByTeam.Find(TeamId);
	if (!Found)
	{
		return;
	}

	*Found = FMath::Max(0, *Found - 1);
}

void UAICommandSubsystem::AddToPlayerList(APawn* Pawn, const uint8 TeamId)
{
	if (!Pawn)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>& List = PlayerPawnsByTeam.FindOrAdd(TeamId);

	for (const TWeakObjectPtr<APawn>& Existing : List)
	{
		if (Existing.Get() == Pawn)
		{
			return;
		}
	}

	List.Add(Pawn);
}

void UAICommandSubsystem::RemoveFromPlayerList(APawn* Pawn, const uint8 TeamId)
{
	if (!Pawn)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>* ListPtr = PlayerPawnsByTeam.Find(TeamId);
	if (!ListPtr)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>& List = *ListPtr;

	List.RemoveAll([Pawn](const TWeakObjectPtr<APawn>& P)
	{
		return P.Get() == Pawn;
	});

	if (List.Num() == 0)
	{
		PlayerPawnsByTeam.Remove(TeamId);
	}
}

void UAICommandSubsystem::AddAttackerToTarget(AActor* Target, APawn* Attacker)
{
	if (!Target)
	{
		return;
	}

	if (!Attacker)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>& List = AttackersByTarget.FindOrAdd(TObjectKey<AActor>(Target));

	for (const TWeakObjectPtr<APawn>& Existing : List)
	{
		if (Existing.Get() == Attacker)
		{
			return;
		}
	}

	List.Add(Attacker);
}

void UAICommandSubsystem::RemoveAttackerFromTarget(AActor* Target, APawn* Attacker)
{
	if (!Target)
	{
		return;
	}

	if (!Attacker)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>* ListPtr = AttackersByTarget.Find(TObjectKey<AActor>(Target));
	if (!ListPtr)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>& List = *ListPtr;

	List.RemoveAll([Attacker](const TWeakObjectPtr<APawn>& P)
	{
		return P.Get() == Attacker;
	});

	if (List.Num() == 0)
	{
		AttackersByTarget.Remove(TObjectKey<AActor>(Target));
	}
}

void UAICommandSubsystem::ClearAllAttackersOfTarget(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>>* ListPtr = AttackersByTarget.Find(TObjectKey<AActor>(Target));
	if (!ListPtr)
	{
		return;
	}

	TArray<TWeakObjectPtr<APawn>> AttackersCopy = *ListPtr;

	for (const TWeakObjectPtr<APawn>& WeakAttacker : AttackersCopy)
	{
		APawn* AttackerPawn = WeakAttacker.Get();
		if (!IsValidCombatPawn(AttackerPawn))
		{
			continue;
		}

		FAgentInfo* Info = Agents.Find(TObjectKey<APawn>(AttackerPawn));
		if (!Info)
		{
			continue;
		}

		AActor* Current = Info->AssignedTarget.Get();
		if (Current != Target)
		{
			continue;
		}

		SetAssignedTarget(*Info, AttackerPawn, nullptr, false);
	}
}

void UAICommandSubsystem::RegisterAgent(APawn* Pawn, const uint8 TeamId)
{
	if (!IsValidCombatPawn(Pawn))
	{
		return;
	}

	const TObjectKey<APawn> Key(Pawn);

	FAgentInfo* ExistingPtr = Agents.Find(Key);
	const uint8 OldTeam = ExistingPtr ? ExistingPtr->TeamId : TeamId;
	const bool bWasPlayer = ExistingPtr ? ExistingPtr->bIsPlayerControlled : false;

	FAgentInfo& Info = Agents.FindOrAdd(Key);
	Info.Pawn = Pawn;
	Info.TeamId = TeamId;
	Info.bIsPlayerControlled = Pawn->IsPlayerControlled();

	const bool bTeamChanged = (OldTeam != TeamId);

	if (bWasPlayer && (bTeamChanged || !Info.bIsPlayerControlled))
	{
		RemoveFromPlayerList(Pawn, OldTeam);
	}

	if (Info.bIsPlayerControlled)
	{
		AddToPlayerList(Pawn, TeamId);
	}

	AActor* Target = Info.AssignedTarget.Get();
	if (!IsValidTargetActor(Target))
	{
		if (Target)
		{
			SetAssignedTarget(Info, Pawn, nullptr, false);
		}
	}
}

void UAICommandSubsystem::UnregisterAgent(APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	const TObjectKey<APawn> Key(Pawn);

	FAgentInfo* InfoPtr = Agents.Find(Key);
	if (!InfoPtr)
	{
		return;
	}

	FAgentInfo& Info = *InfoPtr;

	if (Info.bIsPlayerControlled)
	{
		RemoveFromPlayerList(Pawn, Info.TeamId);
	}

	AActor* OldTarget = Info.AssignedTarget.Get();
	if (OldTarget)
	{
		RemoveAttackerFromTarget(OldTarget, Pawn);
	}

	SetAssignedTarget(Info, Pawn, nullptr, false);
	ReleasePrimaryClaimIfOwnedBy(Pawn);

	Agents.Remove(Key);

	ClearAllAttackersOfTarget(Pawn);
	PrimaryClaimerByTarget.Remove(TObjectKey<AActor>(Pawn));
}

AActor* UAICommandSubsystem::GetAssignedTarget(APawn* Pawn) const
{
	if (!Pawn)
	{
		return nullptr;
	}

	const FAgentInfo* Info = Agents.Find(TObjectKey<APawn>(Pawn));
	if (!Info)
	{
		return nullptr;
	}

	AActor* Target = Info->AssignedTarget.Get();
	if (!IsValidTargetActor(Target))
	{
		return nullptr;
	}

	return Target;
}

void UAICommandSubsystem::ClearAssignedTarget(APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	FAgentInfo* Info = Agents.Find(TObjectKey<APawn>(Pawn));
	if (!Info)
	{
		return;
	}

	AActor* OldTarget = Info->AssignedTarget.Get();
	if (OldTarget)
	{
		RemoveAttackerFromTarget(OldTarget, Pawn);
	}

	SetAssignedTarget(*Info, Pawn, nullptr, false);
	ReleasePrimaryClaimIfOwnedBy(Pawn);
}

void UAICommandSubsystem::ReleasePrimaryClaimIfOwnedBy(APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	const TObjectKey<APawn> PawnKey(Pawn);

	for (auto It = PrimaryClaimerByTarget.CreateIterator(); It; ++It)
	{
		if (It.Value() == PawnKey)
		{
			It.RemoveCurrent();
		}
	}
}

void UAICommandSubsystem::SetPrimaryClaim(APawn* Pawn, AActor* Target)
{
	if (!Pawn)
	{
		return;
	}

	if (!Target)
	{
		return;
	}

	PrimaryClaimerByTarget.FindOrAdd(TObjectKey<AActor>(Target)) = TObjectKey<APawn>(Pawn);
}

void UAICommandSubsystem::SetAssignedTarget(FAgentInfo& SelfInfo, APawn* SelfPawn, AActor* NewTarget, const bool bNewTargetIsPlayer)
{
	AActor* OldTarget = SelfInfo.AssignedTarget.Get();

	const bool bHadPlayerTarget = (OldTarget != nullptr) && SelfInfo.bAssignedTargetIsPlayer;
	if (bHadPlayerTarget)
	{
		DecrementPlayerChasers(SelfInfo.TeamId);
	}

	if (OldTarget && SelfPawn)
	{
		RemoveAttackerFromTarget(OldTarget, SelfPawn);
	}

	SelfInfo.AssignedTarget = NewTarget;
	SelfInfo.bAssignedTargetIsPlayer = bNewTargetIsPlayer;

	const bool bHasPlayerTargetNow = (NewTarget != nullptr) && bNewTargetIsPlayer;
	if (bHasPlayerTargetNow)
	{
		IncrementPlayerChasers(SelfInfo.TeamId);
	}

	if (NewTarget && SelfPawn)
	{
		AddAttackerToTarget(NewTarget, SelfPawn);
	}

	if (NewTarget != nullptr)
	{
		return;
	}

	if (!SelfPawn)
	{
		return;
	}

	ReleasePrimaryClaimIfOwnedBy(SelfPawn);
}

APawn* UAICommandSubsystem::PickClosestEnemyPlayer(APawn* SelfPawn, const uint8 SelfTeamId) const
{
	if (!SelfPawn)
	{
		return nullptr;
	}

	const FVector MyLoc = SelfPawn->GetActorLocation();

	APawn* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (const auto& Pair : PlayerPawnsByTeam)
	{
		const uint8 EnemyTeam = Pair.Key;
		if (EnemyTeam == SelfTeamId)
		{
			continue;
		}

		const TArray<TWeakObjectPtr<APawn>>& Players = Pair.Value;

		for (const TWeakObjectPtr<APawn>& WeakP : Players)
		{
			APawn* P = WeakP.Get();
			if (!IsValidCombatPawn(P))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(MyLoc, P->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = P;
			}
		}
	}

	return Best;
}

APawn* UAICommandSubsystem::PickClosestEnemyAI(APawn* SelfPawn, const uint8 SelfTeamId) const
{
	if (!SelfPawn)
	{
		return nullptr;
	}

	const FVector MyLoc = SelfPawn->GetActorLocation();

	APawn* BestFree = nullptr;
	float BestFreeDistSq = TNumericLimits<float>::Max();

	APawn* BestAny = nullptr;
	float BestAnyDistSq = TNumericLimits<float>::Max();

	for (const auto& Pair : Agents)
	{
		const FAgentInfo& Other = Pair.Value;

		APawn* EnemyPawn = Other.Pawn.Get();
		if (!IsValidCombatPawn(EnemyPawn))
		{
			continue;
		}

		if (EnemyPawn == SelfPawn)
		{
			continue;
		}

		if (Other.TeamId == SelfTeamId)
		{
			continue;
		}

		if (Other.bIsPlayerControlled)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLoc, EnemyPawn->GetActorLocation());

		if (DistSq < BestAnyDistSq)
		{
			BestAnyDistSq = DistSq;
			BestAny = EnemyPawn;
		}

		const TObjectKey<AActor> TargetKey(EnemyPawn);
		const TObjectKey<APawn>* Claimer = PrimaryClaimerByTarget.Find(TargetKey);

		const bool bIsFree = (!Claimer) || (Claimer->ResolveObjectPtr() == SelfPawn);
		if (bIsFree && DistSq < BestFreeDistSq)
		{
			BestFreeDistSq = DistSq;
			BestFree = EnemyPawn;
		}
	}

	return BestFree ? BestFree : BestAny;
}

AActor* UAICommandSubsystem::GetOrAssignTarget(APawn* Pawn, const float MaxTargetDistance, const bool bRequireLineOfSight)
{
	if (!IsValidCombatPawn(Pawn))
	{
		return nullptr;
	}

	FAgentInfo* SelfInfo = Agents.Find(TObjectKey<APawn>(Pawn));
	if (!SelfInfo)
	{
		return nullptr;
	}

	AActor* Existing = SelfInfo->AssignedTarget.Get();
	if (IsValidTargetActor(Existing))
	{
		return Existing;
	}

	if (Existing)
	{
		SetAssignedTarget(*SelfInfo, Pawn, nullptr, false);
	}

	const uint8 SelfTeamId = SelfInfo->TeamId;

	const int32 Budget = GetPlayerChaseBudgetForTeam(SelfTeamId);
	const int32 Current = GetCurrentPlayerChasersForTeam(SelfTeamId);

	if (Current < Budget)
	{
		APawn* PlayerTarget = PickClosestEnemyPlayer(Pawn, SelfTeamId);
		if (PlayerTarget)
		{
			SetAssignedTarget(*SelfInfo, Pawn, PlayerTarget, true);
			return PlayerTarget;
		}
	}

	APawn* EnemyAI = PickClosestEnemyAI(Pawn, SelfTeamId);
	if (!EnemyAI)
	{
		return nullptr;
	}

	SetAssignedTarget(*SelfInfo, Pawn, EnemyAI, false);
	SetPrimaryClaim(Pawn, EnemyAI);

	return EnemyAI;
}

void UAICommandSubsystem::CleanupInvalid()
{
	for (auto It = Agents.CreateIterator(); It; ++It)
	{
		APawn* Pawn = It.Value().Pawn.Get();
		if (IsValidCombatPawn(Pawn))
		{
			continue;
		}

		if (Pawn)
		{
			if (It.Value().bIsPlayerControlled)
			{
				RemoveFromPlayerList(Pawn, It.Value().TeamId);
			}

			AActor* OldTarget = It.Value().AssignedTarget.Get();
			if (OldTarget)
			{
				RemoveAttackerFromTarget(OldTarget, Pawn);
			}

			SetAssignedTarget(It.Value(), Pawn, nullptr, false);
			ReleasePrimaryClaimIfOwnedBy(Pawn);
		}

		It.RemoveCurrent();
	}

	for (auto It = PrimaryClaimerByTarget.CreateIterator(); It; ++It)
	{
		AActor* Target = It.Key().ResolveObjectPtr();
		if (IsValidTargetActor(Target))
		{
			continue;
		}

		It.RemoveCurrent();
	}

	for (auto It = PlayerPawnsByTeam.CreateIterator(); It; ++It)
	{
		TArray<TWeakObjectPtr<APawn>>& List = It.Value();

		List.RemoveAll([this](const TWeakObjectPtr<APawn>& P)
		{
			const APawn* Pawn = P.Get();
			return !IsValidCombatPawn(Pawn);
		});

		if (List.Num() == 0)
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = AttackersByTarget.CreateIterator(); It; ++It)
	{
		AActor* Target = It.Key().ResolveObjectPtr();
		if (IsValidTargetActor(Target))
		{
			continue;
		}

		It.RemoveCurrent();
	}
}