// BTTask_MoveShipTo.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveShipTo.generated.h"

class AShipPawn;
class UShipMovementComponent;

UCLASS()
class GALACTICARMADA_API UBTTask_MoveShipTo : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveShipTo();

protected:
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FName TargetActorKeyName = "TargetActor";

	UPROPERTY(EditAnywhere, Category="Move", meta=(ClampMin="0.0"))
	float AcceptableRadius = 300.0f;

	UPROPERTY(EditAnywhere, Category="Move", meta=(ClampMin="100.0"))
	float SteerLookaheadDistance = 4000.0f;

	// -------------------------
	// Static / world avoidance
	// -------------------------
	UPROPERTY(EditAnywhere, Category="Avoidance|Static")
	bool bEnableStaticAvoidance = true;

	UPROPERTY(EditAnywhere, Category="Avoidance|Static", meta=(ClampMin="0.0"))
	float StaticProbeDistance = 7000.0f;

	UPROPERTY(EditAnywhere, Category="Avoidance|Static", meta=(ClampMin="0.0"))
	float StaticProbeRadius = 600.0f;

	UPROPERTY(EditAnywhere, Category="Avoidance|Static", meta=(ClampMin="0.0"))
	float StaticAvoidanceDistance = 3500.0f;

	UPROPERTY(EditAnywhere, Category="Avoidance|Static", meta=(ClampMin="0.0"))
	float StaticAvoidanceStrength = 2.5f;

	UPROPERTY(EditAnywhere, Category="Avoidance|Static", meta=(ClampMin="0.0"))
	float StaticAvoidanceInterpSpeed = 8.0f;

	// -------------------------
	// Pawn / agent avoidance
	// -------------------------
	UPROPERTY(EditAnywhere, Category="Avoidance|Pawns")
	bool bEnablePawnAvoidance = true;

	UPROPERTY(EditAnywhere, Category="Avoidance|Pawns", meta=(ClampMin="0.0"))
	float PawnQueryRadius = 2500.0f;

	UPROPERTY(EditAnywhere, Category="Avoidance|Pawns", meta=(ClampMin="0.0"))
	float PawnSeparationDistance = 1200.0f;

	UPROPERTY(EditAnywhere, Category="Avoidance|Pawns", meta=(ClampMin="0.0"))
	float PawnAvoidanceStrength = 1.0f;

	UPROPERTY(EditAnywhere, Category="Avoidance|Pawns", meta=(ClampMin="1"))
	int32 PawnMaxNeighbors = 10;

	UPROPERTY(EditAnywhere, Category="Avoidance|Pawns", meta=(ClampMin="0.0"))
	float PawnAvoidanceInterpSpeed = 10.0f;

	// Debug
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, Category="Debug", meta=(ClampMin="0.0"))
	float DebugDrawTime = 0.05f;

	UPROPERTY(EditAnywhere, Category="Debug", meta=(ClampMin="0.0"))
	float DebugLineThickness = 4.0f;

	UPROPERTY(EditAnywhere, Category="Debug", meta=(ClampMin="0.0"))
	float DebugHitSphereRadius = 180.0f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	bool ResolveRefs(UBehaviorTreeComponent& OwnerComp);

	bool FindClosestStaticHitAhead(const FVector& From, const FVector& Forward, FHitResult& OutHit) const;

	FVector ComputeStaticAvoidDir(float DeltaSeconds, const FVector& MyLoc, const FVector& Forward);
	FVector ComputePawnAvoidDir(float DeltaSeconds, const FVector& MyLoc) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AShipPawn> Ship;

	UPROPERTY(Transient)
	TObjectPtr<UShipMovementComponent> MovementComp;

	// Smoothed contributions
	FVector SmoothedStaticAvoidDir = FVector::ZeroVector;
	mutable FVector SmoothedPawnAvoidDir = FVector::ZeroVector;

	bool bLoggedWaitingForTarget = false;
	bool bLoggedNoShip = false;
};