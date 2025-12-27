#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIShipController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;
class UBlackboardData;
class AShipPlayerState;

UCLASS()
class GALACTICARMADA_API AAIShipController : public AAIController
{
	GENERATED_BODY()

public:
	AAIShipController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// Ensures we have a PlayerState and the correct TeamID on it.
	void EnsurePlayerStateTeamFromPawn(APawn* InPawn);

	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBlackboardData> BlackboardAsset;

public:
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
};