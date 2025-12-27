#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_SelectTarget.generated.h"

class UAICommandSubsystem;
class AAIController;
class APawn;
class AActor;
class UHealthComponent;

UCLASS()
class GALACTICARMADA_API UBTService_SelectTarget : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_SelectTarget();

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bLog = false;

private:
	UAICommandSubsystem* GetCommandSubsystem(const UBehaviorTreeComponent& OwnerComp) const;

	bool ResolveContext(UBehaviorTreeComponent& OwnerComp, AAIController*& OutAI, APawn*& OutPawn) const;

	uint8 ResolveTeamId(const APawn* Pawn, const AAIController* AI) const;
	uint8 ResolveTeamIdFromHealth(const APawn* Pawn) const;
	uint8 ResolveTeamIdFromPlayerState(const AAIController* AI) const;

	void RegisterIfNeeded(UAICommandSubsystem* Subsys, APawn* Pawn, uint8 TeamId);

private:
	uint8 CachedRegisteredTeamId = 255;
	bool bHasRegistered = false;

	TWeakObjectPtr<AActor> LastLoggedTarget;
};