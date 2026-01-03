// AIShipController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIShipController.generated.h"

class UBehaviorTree;
class UBlackboardData;
class UAICommandSubsystem;

UCLASS()
class GALACTICARMADA_API AAIShipController : public AAIController
{
	GENERATED_BODY()

public:
	AAIShipController();

	// AAIController already implements IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBlackboardData> BlackboardAsset = nullptr;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAICommandSubsystem> AICommandSubsystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APawn> ControlledPawn = nullptr;

	FGenericTeamId TeamId = FGenericTeamId::NoTeam;

	// Only used to avoid calling BT startup repeatedly during the same possession.
	bool bStartedBehavior = false;

private:
	void EnsureSubsystemsCached();

	void PropagateTeamToPawn();

	void RegisterPawnAsAgent();
	void UnregisterPawnAsAgent();

	FGenericTeamId ResolveTeamFromActor(const AActor& Actor) const;

	void StartBrainIfNeeded();
	void StopBrain();
};
