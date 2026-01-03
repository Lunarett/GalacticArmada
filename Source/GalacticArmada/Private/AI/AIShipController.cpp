// AIShipController.cpp

#include "AI/AIShipController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Systems/AICommandSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogAIShipController, Log, All);

AAIShipController::AAIShipController()
{
}

void AAIShipController::EnsureSubsystemsCached()
{
	if (AICommandSubsystem)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	AICommandSubsystem = World->GetSubsystem<UAICommandSubsystem>();
}

void AAIShipController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledPawn = InPawn;
	bStartedBehavior = false;

	EnsureSubsystemsCached();

	PropagateTeamToPawn();
	RegisterPawnAsAgent();

	StartBrainIfNeeded();
}

void AAIShipController::OnUnPossess()
{
	StopBrain();
	UnregisterPawnAsAgent();

	Super::OnUnPossess();

	ControlledPawn = nullptr;
	bStartedBehavior = false;
}

// --------------------------
// IGenericTeamAgentInterface
// --------------------------

FGenericTeamId AAIShipController::GetGenericTeamId() const
{
	return TeamId;
}

void AAIShipController::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	if (TeamId == NewTeamId)
	{
		return;
	}

	TeamId = NewTeamId;

	// Keep pawn + subsystem registration in sync if we are possessing.
	PropagateTeamToPawn();

	UnregisterPawnAsAgent();
	RegisterPawnAsAgent();
}

FGenericTeamId AAIShipController::ResolveTeamFromActor(const AActor& Actor) const
{
	const IGenericTeamAgentInterface* const DirectTeamAgent = Cast<IGenericTeamAgentInterface>(&Actor);
	if (DirectTeamAgent)
	{
		return DirectTeamAgent->GetGenericTeamId();
	}

	const APawn* const OtherPawn = Cast<APawn>(&Actor);
	if (!OtherPawn)
	{
		return FGenericTeamId::NoTeam;
	}

	const AController* const OtherController = OtherPawn->GetController();
	if (!OtherController)
	{
		return FGenericTeamId::NoTeam;
	}

	const IGenericTeamAgentInterface* const ControllerTeamAgent = Cast<IGenericTeamAgentInterface>(OtherController);
	if (!ControllerTeamAgent)
	{
		return FGenericTeamId::NoTeam;
	}

	return ControllerTeamAgent->GetGenericTeamId();
}

ETeamAttitude::Type AAIShipController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const FGenericTeamId OtherTeam = ResolveTeamFromActor(Other);

	if (TeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	if (OtherTeam == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	if (TeamId == OtherTeam)
	{
		return ETeamAttitude::Friendly;
	}

	return ETeamAttitude::Hostile;
}

// --------------------------
// Team propagation + agent reg
// --------------------------

void AAIShipController::PropagateTeamToPawn()
{
	if (!ControlledPawn)
	{
		return;
	}

	IGenericTeamAgentInterface* const PawnTeamAgent = Cast<IGenericTeamAgentInterface>(ControlledPawn);
	if (!PawnTeamAgent)
	{
		return;
	}

	if (PawnTeamAgent->GetGenericTeamId() == TeamId)
	{
		return;
	}

	PawnTeamAgent->SetGenericTeamId(TeamId);
}

void AAIShipController::RegisterPawnAsAgent()
{
	if (!ControlledPawn)
	{
		return;
	}

	if (!AICommandSubsystem)
	{
		return;
	}

	AICommandSubsystem->RegisterAgent(ControlledPawn, TeamId.GetId());
}

void AAIShipController::UnregisterPawnAsAgent()
{
	if (!ControlledPawn)
	{
		return;
	}

	if (!AICommandSubsystem)
	{
		return;
	}

	AICommandSubsystem->UnregisterAgent(ControlledPawn);
}

// --------------------------
// Behavior Tree basics
// --------------------------

void AAIShipController::StartBrainIfNeeded()
{
	if (bStartedBehavior)
	{
		return;
	}

	if (!BlackboardAsset || !BehaviorTreeAsset)
	{
		return;
	}

	UBlackboardComponent* BBComp = nullptr;
	if (!UseBlackboard(BlackboardAsset, BBComp))
	{
		return;
	}

	if (!RunBehaviorTree(BehaviorTreeAsset))
	{
		return;
	}

	bStartedBehavior = true;
}

void AAIShipController::StopBrain()
{
	bStartedBehavior = false;

	if (UBrainComponent* Brain = BrainComponent)
	{
		Brain->StopLogic(TEXT("UnPossess"));
	}
}