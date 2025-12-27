#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AICommandSubsystem.generated.h"

UCLASS()
class GALACTICARMADA_API UAICommandSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickableInEditor() const override { return false; }

public:
	void RegisterAgent(APawn* Pawn, uint8 TeamId);
	void UnregisterAgent(APawn* Pawn);

	AActor* GetAssignedTarget(APawn* Pawn) const;
	void ClearAssignedTarget(APawn* Pawn);

	AActor* GetOrAssignTarget(APawn* Pawn, float MaxTargetDistance, bool bRequireLineOfSight);

public:
	void SetPlayerChaseBudgetForTeam(uint8 TeamId, int32 Budget);
	int32 GetPlayerChaseBudgetForTeam(uint8 TeamId) const;

private:
	struct FAgentInfo
	{
		TWeakObjectPtr<APawn> Pawn;
		uint8 TeamId = 0;

		bool bIsPlayerControlled = false;

		TWeakObjectPtr<AActor> AssignedTarget;
		bool bAssignedTargetIsPlayer = false;
	};

private:
	void CleanupInvalid();

	bool IsValidCombatPawn(const APawn* Pawn) const;
	bool IsValidTargetActor(const AActor* Target) const;

	void ReleasePrimaryClaimIfOwnedBy(APawn* Pawn);
	void SetPrimaryClaim(APawn* Pawn, AActor* Target);

	void AddToPlayerList(APawn* Pawn, uint8 TeamId);
	void RemoveFromPlayerList(APawn* Pawn, uint8 TeamId);

	void SetAssignedTarget(FAgentInfo& SelfInfo, APawn* SelfPawn, AActor* NewTarget, bool bNewTargetIsPlayer);

	void AddAttackerToTarget(AActor* Target, APawn* Attacker);
	void RemoveAttackerFromTarget(AActor* Target, APawn* Attacker);

	void ClearAllAttackersOfTarget(AActor* Target);

	int32 GetCurrentPlayerChasersForTeam(uint8 TeamId) const;
	void IncrementPlayerChasers(uint8 TeamId);
	void DecrementPlayerChasers(uint8 TeamId);

	APawn* PickClosestEnemyPlayer(APawn* SelfPawn, uint8 SelfTeamId) const;
	APawn* PickClosestEnemyAI(APawn* SelfPawn, uint8 SelfTeamId) const;

private:
	TMap<TObjectKey<APawn>, FAgentInfo> Agents;

	TMap<TObjectKey<AActor>, TObjectKey<APawn>> PrimaryClaimerByTarget;

	TMap<uint8, TArray<TWeakObjectPtr<APawn>>> PlayerPawnsByTeam;

	TMap<uint8, int32> CurrentPlayerChasersByTeam;

	UPROPERTY()
	TMap<uint8, int32> PlayerChaseBudgetByTeam;

	TMap<TObjectKey<AActor>, TArray<TWeakObjectPtr<APawn>>> AttackersByTarget;

	float CleanupInterval = 0.5f;
	float CleanupAccum = 0.0f;
};