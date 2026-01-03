// AITargetSelectionSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GenericTeamAgentInterface.h"
#include "AITargetSelectionSubsystem.generated.h"

UCLASS()
class GALACTICARMADA_API UAITargetSelectionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterAgent(APawn* AgentPawn, uint8 TeamId);
	void UnregisterAgent(APawn* AgentPawn);

	AActor* GetOrAssignTarget(APawn* ClaimerPawn);
	void ReleaseTarget(APawn* ClaimerPawn);

	// TEMP: compile compatibility with existing code (BaseMatchGameMode).
	// We’ll replace this with the new quota logic later.
	void SetPlayerChaseBudgetForTeam(uint8 TeamId, int32 Budget);

protected:
	virtual void Deinitialize() override;

private:
	struct FClaimKey
	{
		uint8 TeamId = 255;
		TWeakObjectPtr<AActor> Target;

		friend bool operator==(const FClaimKey& A, const FClaimKey& B)
		{
			return A.TeamId == B.TeamId && A.Target == B.Target;
		}

		friend uint32 GetTypeHash(const FClaimKey& Key)
		{
			return HashCombine(::GetTypeHash(Key.TeamId), ::GetTypeHash(Key.Target));
		}
	};

private:
	TMap<uint8, TArray<TWeakObjectPtr<APawn>>> AgentsByTeam;

	TMap<TWeakObjectPtr<APawn>, TWeakObjectPtr<AActor>> TargetByClaimer;

	TMap<FClaimKey, TWeakObjectPtr<APawn>> ClaimerByTeamAndTarget;

	// Dynamic delegate -> no FDelegateHandle. We track if we bound.
	TSet<TWeakObjectPtr<AActor>> BoundDestroyedTargets;

	// TEMP: compile compatibility
	TMap<uint8, int32> PlayerChaseBudgetByTeam;

private:
	bool IsValidTarget(const AActor* Target) const;
	uint8 ResolveTeamIdFromActor(const AActor& Actor) const;

	AActor* PickTargetFor(APawn* ClaimerPawn, uint8 ClaimerTeamId) const;

	void BindDestroyedOnce(AActor* TargetActor);
	void UnbindDestroyed(AActor* TargetActor);

	UFUNCTION()
	void HandleTargetDestroyed(AActor* DestroyedActor);

	void CleanupDeadWeakEntries();
};