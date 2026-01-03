// AIShipController.cpp

#include "AI/AIShipController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#include "Systems/AITargetSelectionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogAIShipController, Log, All);

AAIShipController::AAIShipController()
{
	bWantsPlayerState = false;
}

void AAIShipController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledPawn = InPawn;
	ClearAvoidanceNeighbors();

	EnsureSubsystemCached();
	RegisterPawnAsAgent();

	CreateOrAttachAvoidanceSphere();
	StartBrainIfNeeded();
}

void AAIShipController::OnUnPossess()
{
	StopBrain();
	DestroyAvoidanceSphere();

	UnregisterPawnAsAgent();
	ClearAvoidanceNeighbors();

	ControlledPawn = nullptr;

	Super::OnUnPossess();
}

void AAIShipController::EnsureSubsystemCached()
{
	if (TargetSelectionSubsystem)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		TargetSelectionSubsystem = World->GetSubsystem<UAITargetSelectionSubsystem>();
	}
}

void AAIShipController::RegisterPawnAsAgent()
{
	if (!TargetSelectionSubsystem || !ControlledPawn)
	{
		return;
	}

	// Subsystem expects explicit team id.
	TargetSelectionSubsystem->RegisterAgent(ControlledPawn, TeamId.GetId());
}

void AAIShipController::UnregisterPawnAsAgent()
{
	if (!TargetSelectionSubsystem || !ControlledPawn)
	{
		return;
	}

	TargetSelectionSubsystem->UnregisterAgent(ControlledPawn);
}

void AAIShipController::CreateOrAttachAvoidanceSphere()
{
	if (!ControlledPawn)
	{
		return;
	}

	if (AvoidanceSphere)
	{
		AvoidanceSphere->OnComponentBeginOverlap.RemoveAll(this);
		AvoidanceSphere->OnComponentEndOverlap.RemoveAll(this);
		AvoidanceSphere->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		AvoidanceSphere->DestroyComponent();
		AvoidanceSphere = nullptr;
	}

	AvoidanceSphere = NewObject<USphereComponent>(this, TEXT("AI_AvoidanceSphere"));
	if (!AvoidanceSphere)
	{
		return;
	}

	AvoidanceSphere->RegisterComponent();

	AvoidanceSphere->SetSphereRadius(FMath::Max(0.0f, AvoidanceSphereRadius));
	AvoidanceSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AvoidanceSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AvoidanceSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AvoidanceSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	AvoidanceSphere->OnComponentBeginOverlap.AddDynamic(this, &AAIShipController::OnAvoidanceBeginOverlap);
	AvoidanceSphere->OnComponentEndOverlap.AddDynamic(this, &AAIShipController::OnAvoidanceEndOverlap);

	USceneComponent* AttachParent = ControlledPawn->GetRootComponent();
	if (!AttachParent)
	{
		return;
	}

	AvoidanceSphere->AttachToComponent(
		AttachParent,
		FAttachmentTransformRules::KeepRelativeTransform,
		AvoidanceSphereAttachSocket
	);

	AvoidanceSphere->SetRelativeLocation(FVector::ZeroVector);
}

void AAIShipController::DestroyAvoidanceSphere()
{
	if (!AvoidanceSphere)
	{
		return;
	}

	AvoidanceSphere->OnComponentBeginOverlap.RemoveAll(this);
	AvoidanceSphere->OnComponentEndOverlap.RemoveAll(this);

	AvoidanceSphere->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	AvoidanceSphere->DestroyComponent();
	AvoidanceSphere = nullptr;
}

void AAIShipController::ClearAvoidanceNeighbors()
{
	AvoidanceNeighbors.Reset();
}

void AAIShipController::StartBrainIfNeeded()
{
	if (bStartedBehavior)
	{
		return;
	}

	if (!BehaviorTreeAsset || !BlackboardAsset)
	{
		return;
	}

	UBlackboardComponent* BBComp = nullptr;
	if (!UseBlackboard(BlackboardAsset, BBComp))
	{
		return;
	}

	RunBehaviorTree(BehaviorTreeAsset);
	bStartedBehavior = true;
}

void AAIShipController::StopBrain()
{
	if (!bStartedBehavior)
	{
		return;
	}

	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("UnPossess"));
	}
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

	// If already possessed + registered, refresh registration with correct team.
	if (ControlledPawn && TargetSelectionSubsystem)
	{
		TargetSelectionSubsystem->UnregisterAgent(ControlledPawn);
		TargetSelectionSubsystem->RegisterAgent(ControlledPawn, TeamId.GetId());
	}
}

FGenericTeamId AAIShipController::ResolveTeamFromActor(const AActor& Actor) const
{
	const IGenericTeamAgentInterface* DirectAgent = Cast<IGenericTeamAgentInterface>(&Actor);
	if (DirectAgent)
	{
		return DirectAgent->GetGenericTeamId();
	}

	const APawn* OtherPawn = Cast<APawn>(&Actor);
	if (!OtherPawn)
	{
		return FGenericTeamId::NoTeam;
	}

	const AController* OtherController = OtherPawn->GetController();
	if (!OtherController)
	{
		return FGenericTeamId::NoTeam;
	}

	const IGenericTeamAgentInterface* CtrlAgent = Cast<IGenericTeamAgentInterface>(OtherController);
	if (!CtrlAgent)
	{
		return FGenericTeamId::NoTeam;
	}

	return CtrlAgent->GetGenericTeamId();
}

ETeamAttitude::Type AAIShipController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const FGenericTeamId OtherTeam = ResolveTeamFromActor(Other);

	if (TeamId == FGenericTeamId::NoTeam || OtherTeam == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	return (TeamId == OtherTeam) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

// --------------------------
// Avoidance overlap handlers
// --------------------------

void AAIShipController::OnAvoidanceBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	(void)OverlappedComp;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	if (!OtherActor || !ControlledPawn || OtherActor == ControlledPawn)
	{
		return;
	}

	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn)
	{
		return;
	}

	const TWeakObjectPtr<APawn> Weak = OtherPawn;
	if (AvoidanceNeighbors.Contains(Weak))
	{
		return;
	}

	AvoidanceNeighbors.Add(Weak);
}

void AAIShipController::OnAvoidanceEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	(void)OverlappedComp;
	(void)OtherComp;
	(void)OtherBodyIndex;

	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn)
	{
		return;
	}

	AvoidanceNeighbors.RemoveSwap(TWeakObjectPtr<APawn>(OtherPawn));
}