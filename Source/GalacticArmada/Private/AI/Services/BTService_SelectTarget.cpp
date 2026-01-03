#include "AI/Services/BTService_SelectTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"

#include "Systems/AITargetSelectionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogBTServiceSelectTarget, Log, All);

UBTService_SelectTarget::UBTService_SelectTarget()
{
	NodeName = TEXT("Select Target");

	bNotifyBecomeRelevant = true;
	bNotifyTick = true;

	// This service should be cheap: it only does work when the current target is invalid.
	// Keep the interval relatively low-frequency.
	Interval = 1.0f;
	RandomDeviation = 0.0f;
}

void UBTService_SelectTarget::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	AAIController* AI = nullptr;
	APawn* Pawn = nullptr;
	if (!ResolveContext(OwnerComp, AI, Pawn))
	{
		return;
	}

	if (IsCurrentTargetValid(OwnerComp))
	{
		return;
	}

	RefreshTarget(OwnerComp, Pawn);
}

void UBTService_SelectTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// Only re-query if the current target is invalid.
	if (IsCurrentTargetValid(OwnerComp))
	{
		return;
	}

	AAIController* AI = nullptr;
	APawn* Pawn = nullptr;
	if (!ResolveContext(OwnerComp, AI, Pawn))
	{
		return;
	}

	RefreshTarget(OwnerComp, Pawn);
}

bool UBTService_SelectTarget::ResolveContext(UBehaviorTreeComponent& OwnerComp, AAIController*& OutAI, APawn*& OutPawn)
{
	OutAI = OwnerComp.GetAIOwner();
	if (!OutAI)
	{
		return false;
	}

	OutPawn = OutAI->GetPawn();
	if (!OutPawn)
	{
		return false;
	}

	return true;
}

UAITargetSelectionSubsystem* UBTService_SelectTarget::GetTargetSubsystem(const UBehaviorTreeComponent& OwnerComp)
{
	UWorld* const World = OwnerComp.GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<UAITargetSelectionSubsystem>();
}

bool UBTService_SelectTarget::IsCurrentTargetValid(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return false;
	}

	UObject* const Obj = BB->GetValueAsObject(BlackboardKey.SelectedKeyName);
	AActor* const Target = Cast<AActor>(Obj);

	return IsValid(Target);
}

void UBTService_SelectTarget::RefreshTarget(UBehaviorTreeComponent& OwnerComp, APawn* ClaimerPawn)
{
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	UAITargetSelectionSubsystem* const Subsys = GetTargetSubsystem(OwnerComp);
	if (!Subsys)
	{
		return;
	}

	AActor* const Assigned = Subsys->GetOrAssignTarget(ClaimerPawn);
	if (!IsValid(Assigned))
	{
		BB->ClearValue(BlackboardKey.SelectedKeyName);
		return;
	}

	UObject* const CurrentObj = BB->GetValueAsObject(BlackboardKey.SelectedKeyName);
	AActor* const Current = Cast<AActor>(CurrentObj);

	if (Current == Assigned)
	{
		return;
	}

	BB->SetValueAsObject(BlackboardKey.SelectedKeyName, Assigned);

	if (bLog && LastLoggedTarget.Get() != Assigned)
	{
		LastLoggedTarget = Assigned;

		UE_LOG(LogBTServiceSelectTarget, Log,
			TEXT("SelectTarget: Claimer=%s -> Target=%s"),
			ClaimerPawn ? *ClaimerPawn->GetName() : TEXT("None"),
			Assigned ? *Assigned->GetName() : TEXT("None"));
	}
}