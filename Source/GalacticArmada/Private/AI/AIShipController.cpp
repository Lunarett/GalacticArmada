#include "AI/AIShipController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

#include "Components/HealthComponent.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerState.h"

AAIShipController::AAIShipController()
{
	PrimaryActorTick.bCanEverTick = false;

	// Critical: AI controllers do NOT always get a PlayerState unless you ask for it.
	bWantsPlayerState = true;
}

void AAIShipController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn)
	{
		return;
	}

	EnsurePlayerStateTeamFromPawn(InPawn);

	if (!BlackboardAsset || !BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AIShipController] Missing BlackboardAsset or BehaviorTreeAsset."));
		return;
	}

	UBlackboardComponent* BlackboardComp = nullptr;
	if (!UseBlackboard(BlackboardAsset, BlackboardComp) || !BlackboardComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[AIShipController] UseBlackboard failed."));
		return;
	}

	Blackboard = BlackboardComp;

	if (!RunBehaviorTree(BehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Error, TEXT("[AIShipController] RunBehaviorTree failed."));
	}
}

void AAIShipController::OnUnPossess()
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("UnPossess"));
	}

	Super::OnUnPossess();
}

void AAIShipController::EnsurePlayerStateTeamFromPawn(APawn* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	// If PlayerState didn't get created yet, force it.
	if (!PlayerState)
	{
		InitPlayerState();
	}

	AShipPlayerState* SPS = Cast<AShipPlayerState>(PlayerState);
	if (!SPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AIShipController] PlayerState is missing or not AShipPlayerState. PS=%s"),
			PlayerState ? *PlayerState->GetName() : TEXT("NULL"));
		return;
	}

	const UHealthComponent* Health = InPawn->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AIShipController] No HealthComponent on %s. TeamID not assigned to PlayerState."),
			*InPawn->GetName());
		return;
	}

	const int32 TeamId = Health->GetTeamId();
	//SPS->SetTeamID(TeamId);

	UE_LOG(LogTemp, Warning, TEXT("[AIShipController] Assigned PlayerState TeamID=%d for %s (PS=%s)"),
		TeamId, *InPawn->GetName(), *SPS->GetName());
}

ETeamAttitude::Type AAIShipController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const AShipPawn* MyShip = Cast<AShipPawn>(GetPawn());
	const AShipPawn* OtherShip = Cast<AShipPawn>(&Other);

	if (!MyShip || !OtherShip)
	{
		return ETeamAttitude::Neutral;
	}

	const UHealthComponent* MyHealth = MyShip->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* OtherHealth = OtherShip->FindComponentByClass<UHealthComponent>();

	if (!MyHealth || !OtherHealth)
	{
		return ETeamAttitude::Neutral;
	}

	return (MyHealth->GetTeamId() == OtherHealth->GetTeamId())
		? ETeamAttitude::Friendly
		: ETeamAttitude::Hostile;
}