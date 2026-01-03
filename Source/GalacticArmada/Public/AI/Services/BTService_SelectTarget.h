#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_SelectTarget.generated.h"

class UAITargetSelectionSubsystem;
class AAIController;
class APawn;
class AActor;

UCLASS()
class GALACTICARMADA_API UBTService_SelectTarget : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_SelectTarget();

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bLog = false;

private:
	static bool ResolveContext(UBehaviorTreeComponent& OwnerComp, AAIController*& OutAI, APawn*& OutPawn);
	static UAITargetSelectionSubsystem* GetTargetSubsystem(const UBehaviorTreeComponent& OwnerComp);

	bool IsCurrentTargetValid(const UBehaviorTreeComponent& OwnerComp) const;
	void RefreshTarget(UBehaviorTreeComponent& OwnerComp, APawn* ClaimerPawn);

private:
	TWeakObjectPtr<AActor> LastLoggedTarget;
};