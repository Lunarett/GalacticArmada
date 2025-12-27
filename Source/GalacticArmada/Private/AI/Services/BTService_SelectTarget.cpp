#include "AI/Services/BTService_SelectTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#include "Components/HealthComponent.h"
#include "Player/ShipPlayerState.h"
#include "Systems/AICommandSubsystem.h"

UBTService_SelectTarget::UBTService_SelectTarget()
{
	NodeName = TEXT("Select Target (AICommandSubsystem)");

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bNotifyTick = true;

	Interval = 0.25f;
	RandomDeviation = 0.05f;

	CachedRegisteredTeamId = 255;
	bHasRegistered = false;
}

void UBTService_SelectTarget::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	AAIController* AI = nullptr;
	APawn* Pawn = nullptr;

	const bool bOk = ResolveContext(OwnerComp, AI, Pawn);
	if (!bOk)
	{
		return;
	}

	UAICommandSubsystem* Subsys = GetCommandSubsystem(OwnerComp);
	if (!Subsys)
	{
		return;
	}

	const uint8 TeamId = ResolveTeamId(Pawn, AI);
	RegisterIfNeeded(Subsys, Pawn, TeamId);

	if (bLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectTarget] BecomeRelevant %s Team=%d"), *Pawn->GetName(), (int32)TeamId);
	}
}

void UBTService_SelectTarget::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = nullptr;
	APawn* Pawn = nullptr;

	const bool bOk = ResolveContext(OwnerComp, AI, Pawn);
	if (!bOk)
	{
		Super::OnCeaseRelevant(OwnerComp, NodeMemory);
		return;
	}

	UAICommandSubsystem* Subsys = GetCommandSubsystem(OwnerComp);
	if (!Subsys)
	{
		Super::OnCeaseRelevant(OwnerComp, NodeMemory);
		return;
	}

	Subsys->UnregisterAgent(Pawn);

	CachedRegisteredTeamId = 255;
	bHasRegistered = false;
	LastLoggedTarget.Reset();

	if (bLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectTarget] CeaseRelevant %s Unregistered"), *Pawn->GetName());
	}

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTService_SelectTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = nullptr;
	APawn* Pawn = nullptr;

	const bool bOk = ResolveContext(OwnerComp, AI, Pawn);
	if (!bOk)
	{
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	UAICommandSubsystem* Subsys = GetCommandSubsystem(OwnerComp);
	if (!Subsys)
	{
		return;
	}

	const uint8 TeamId = ResolveTeamId(Pawn, AI);
	RegisterIfNeeded(Subsys, Pawn, TeamId);

	AActor* Assigned = Subsys->GetOrAssignTarget(Pawn, 0.0f, false);

	BB->SetValueAsObject(BlackboardKey.SelectedKeyName, Assigned);

	if (!bLog)
	{
		return;
	}

	if (LastLoggedTarget.Get() == Assigned)
	{
		return;
	}

	LastLoggedTarget = Assigned;

	UE_LOG(LogTemp, Warning, TEXT("[SelectTarget] %s -> %s"),
		*Pawn->GetName(),
		Assigned ? *Assigned->GetName() : TEXT("NONE"));
}

UAICommandSubsystem* UBTService_SelectTarget::GetCommandSubsystem(const UBehaviorTreeComponent& OwnerComp) const
{
	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<UAICommandSubsystem>();
}

bool UBTService_SelectTarget::ResolveContext(UBehaviorTreeComponent& OwnerComp, AAIController*& OutAI, APawn*& OutPawn) const
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

uint8 UBTService_SelectTarget::ResolveTeamId(const APawn* Pawn, const AAIController* AI) const
{
	const uint8 FromHealth = ResolveTeamIdFromHealth(Pawn);
	if (FromHealth != 0)
	{
		return FromHealth;
	}

	return ResolveTeamIdFromPlayerState(AI);
}

uint8 UBTService_SelectTarget::ResolveTeamIdFromHealth(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return 0;
	}

	const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		return 0;
	}

	const int32 Team = Health->GetTeamId();
	return (uint8)FMath::Clamp(Team, 0, 255);
}

uint8 UBTService_SelectTarget::ResolveTeamIdFromPlayerState(const AAIController* AI) const
{
	if (!AI)
	{
		return 0;
	}

	const APlayerState* PS = AI->PlayerState;
	if (!PS)
	{
		return 0;
	}

	const AShipPlayerState* ShipPS = Cast<AShipPlayerState>(PS);
	if (!ShipPS)
	{
		return 0;
	}

	const int32 Team = ShipPS->GetTeamID();
	return (uint8)FMath::Clamp(Team, 0, 255);
}

void UBTService_SelectTarget::RegisterIfNeeded(UAICommandSubsystem* Subsys, APawn* Pawn, const uint8 TeamId)
{
	if (!Subsys)
	{
		return;
	}

	if (!Pawn)
	{
		return;
	}

	if (!bHasRegistered)
	{
		Subsys->RegisterAgent(Pawn, TeamId);
		CachedRegisteredTeamId = TeamId;
		bHasRegistered = true;
		return;
	}

	if (CachedRegisteredTeamId == TeamId)
	{
		return;
	}

	Subsys->RegisterAgent(Pawn, TeamId);
	CachedRegisteredTeamId = TeamId;

	if (bLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectTarget] Re-Register %s Team=%d"), *Pawn->GetName(), (int32)TeamId);
	}
}