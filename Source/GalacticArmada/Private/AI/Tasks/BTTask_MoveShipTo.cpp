// BTTask_MoveShipTo.cpp

#include "AI/Tasks/BTTask_MoveShipTo.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

#include "Player/ShipPawn.h"
#include "Player/ShipMovementComponent.h"

UBTTask_MoveShipTo::UBTTask_MoveShipTo()
{
	NodeName = TEXT("Move Ship (Direct + Split Avoidance)");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

bool UBTTask_MoveShipTo::ResolveRefs(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI)
	{
		return false;
	}

	Ship = Cast<AShipPawn>(AI->GetPawn());
	if (!Ship)
	{
		return false;
	}

	MovementComp = Ship->GetShipMovementComponent();
	return MovementComp != nullptr;
}

EBTNodeResult::Type UBTTask_MoveShipTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (!ResolveRefs(OwnerComp))
	{
		return EBTNodeResult::Failed;
	}

	SmoothedStaticAvoidDir = FVector::ZeroVector;
	SmoothedPawnAvoidDir = FVector::ZeroVector;

	bLoggedWaitingForTarget = false;
	bLoggedNoShip = false;

	return EBTNodeResult::InProgress;
}

bool UBTTask_MoveShipTo::FindClosestStaticHitAhead(const FVector& From, const FVector& Forward, FHitResult& OutHit) const
{
	if (!Ship)
	{
		return false;
	}

	UWorld* World = Ship->GetWorld();
	if (!World)
	{
		return false;
	}

	const float Dist = FMath::Max(0.0f, StaticProbeDistance);
	const float Radius = FMath::Max(0.0f, StaticProbeRadius);
	if (Dist <= KINDA_SMALL_NUMBER || Radius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector Start = From;
	const FVector End = From + Forward * Dist;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MoveShipToStaticAvoid), false);
	Params.AddIgnoredActor(Ship);

	FCollisionObjectQueryParams Obj;
	Obj.AddObjectTypesToQuery(ECC_WorldStatic);
	Obj.AddObjectTypesToQuery(ECC_WorldDynamic);

	const bool bHit = World->SweepSingleByObjectType(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		Obj,
		FCollisionShape::MakeSphere(Radius),
		Params
	);

	return bHit && OutHit.bBlockingHit;
}

FVector UBTTask_MoveShipTo::ComputeStaticAvoidDir(float DeltaSeconds, const FVector& MyLoc, const FVector& Forward)
{
	if (!bEnableStaticAvoidance)
	{
		SmoothedStaticAvoidDir = FMath::VInterpTo(SmoothedStaticAvoidDir, FVector::ZeroVector, DeltaSeconds, StaticAvoidanceInterpSpeed);
		return SmoothedStaticAvoidDir;
	}

	FHitResult Hit;
	if (!FindClosestStaticHitAhead(MyLoc, Forward, Hit))
	{
		SmoothedStaticAvoidDir = FMath::VInterpTo(SmoothedStaticAvoidDir, FVector::ZeroVector, DeltaSeconds, StaticAvoidanceInterpSpeed);
		return SmoothedStaticAvoidDir;
	}

	const float DistToHit = FVector::Distance(MyLoc, Hit.ImpactPoint);
	const float AvoidDist = FMath::Max(0.0f, StaticAvoidanceDistance);

	if (AvoidDist <= KINDA_SMALL_NUMBER || DistToHit >= AvoidDist)
	{
		SmoothedStaticAvoidDir = FMath::VInterpTo(SmoothedStaticAvoidDir, FVector::ZeroVector, DeltaSeconds, StaticAvoidanceInterpSpeed);
		return SmoothedStaticAvoidDir;
	}

	FVector AwayDir = (MyLoc - Hit.ImpactPoint).GetSafeNormal();
	if (AwayDir.IsNearlyZero())
	{
		AwayDir = Hit.ImpactNormal.GetSafeNormal();
	}

	const float Alpha = 1.0f - FMath::Clamp(DistToHit / AvoidDist, 0.0f, 1.0f);
	const float W = Alpha * Alpha;

	SmoothedStaticAvoidDir = FMath::VInterpTo(SmoothedStaticAvoidDir, AwayDir, DeltaSeconds, StaticAvoidanceInterpSpeed);
	const FVector Result = SmoothedStaticAvoidDir * (W * FMath::Max(0.0f, StaticAvoidanceStrength));

	if (bDrawDebug)
	{
		if (UWorld* World = Ship ? Ship->GetWorld() : nullptr)
		{
			DrawDebugLine(World, MyLoc, Hit.ImpactPoint, FColor::Red, false, DebugDrawTime, 0, DebugLineThickness);
			DrawDebugSphere(World, Hit.ImpactPoint, DebugHitSphereRadius, 12, FColor::Red, false, DebugDrawTime, 0, DebugLineThickness);
		}
	}

	return Result;
}

FVector UBTTask_MoveShipTo::ComputePawnAvoidDir(float DeltaSeconds, const FVector& MyLoc) const
{
	if (!bEnablePawnAvoidance || !Ship)
	{
		SmoothedPawnAvoidDir = FMath::VInterpTo(SmoothedPawnAvoidDir, FVector::ZeroVector, DeltaSeconds, PawnAvoidanceInterpSpeed);
		return SmoothedPawnAvoidDir;
	}

	UWorld* World = Ship->GetWorld();
	if (!World)
	{
		SmoothedPawnAvoidDir = FMath::VInterpTo(SmoothedPawnAvoidDir, FVector::ZeroVector, DeltaSeconds, PawnAvoidanceInterpSpeed);
		return SmoothedPawnAvoidDir;
	}

	const float QueryR = FMath::Max(0.0f, PawnQueryRadius);
	const float SepR = FMath::Max(0.0f, PawnSeparationDistance);

	if (QueryR <= KINDA_SMALL_NUMBER || SepR <= KINDA_SMALL_NUMBER || PawnMaxNeighbors <= 0)
	{
		SmoothedPawnAvoidDir = FMath::VInterpTo(SmoothedPawnAvoidDir, FVector::ZeroVector, DeltaSeconds, PawnAvoidanceInterpSpeed);
		return SmoothedPawnAvoidDir;
	}

	FCollisionObjectQueryParams Obj;
	Obj.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MoveShipToPawnAvoid), false);
	Params.AddIgnoredActor(Ship);

	TArray<FOverlapResult> Overlaps;
	Overlaps.Reserve(PawnMaxNeighbors * 2);

	const bool bAny = World->OverlapMultiByObjectType(
		Overlaps,
		MyLoc,
		FQuat::Identity,
		Obj,
		FCollisionShape::MakeSphere(QueryR),
		Params
	);

	if (!bAny)
	{
		SmoothedPawnAvoidDir = FMath::VInterpTo(SmoothedPawnAvoidDir, FVector::ZeroVector, DeltaSeconds, PawnAvoidanceInterpSpeed);
		return SmoothedPawnAvoidDir;
	}

	struct FNeighbor
	{
		AActor* Actor = nullptr;
		float DistSq = 0.0f;
	};

	TArray<FNeighbor> Neigh;
	Neigh.Reserve(PawnMaxNeighbors);

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Other = O.GetActor();
		if (!Other || Other == Ship)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLoc, Other->GetActorLocation());
		if (DistSq > QueryR * QueryR)
		{
			continue;
		}

		if (Neigh.Num() < PawnMaxNeighbors)
		{
			Neigh.Add({Other, DistSq});
			continue;
		}

		int32 FarthestIdx = 0;
		float FarthestDistSq = Neigh[0].DistSq;
		for (int32 i = 1; i < Neigh.Num(); ++i)
		{
			if (Neigh[i].DistSq > FarthestDistSq)
			{
				FarthestDistSq = Neigh[i].DistSq;
				FarthestIdx = i;
			}
		}

		if (DistSq < FarthestDistSq)
		{
			Neigh[FarthestIdx] = {Other, DistSq};
		}
	}

	FVector Accum = FVector::ZeroVector;
	float WeightSum = 0.0f;

	for (const FNeighbor& N : Neigh)
	{
		if (!N.Actor)
		{
			continue;
		}

		const float DistSq = N.DistSq;
		if (DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Dist = FMath::Sqrt(DistSq);
		if (Dist >= SepR)
		{
			continue;
		}

		const FVector Away = (MyLoc - N.Actor->GetActorLocation()) / Dist;

		const float Alpha = 1.0f - FMath::Clamp(Dist / SepR, 0.0f, 1.0f);
		const float W = Alpha * Alpha;

		Accum += Away * W;
		WeightSum += W;
	}

	FVector Desired = FVector::ZeroVector;
	if (WeightSum > KINDA_SMALL_NUMBER)
	{
		Desired = (Accum / WeightSum).GetSafeNormal();
	}

	SmoothedPawnAvoidDir = FMath::VInterpTo(SmoothedPawnAvoidDir, Desired, DeltaSeconds, PawnAvoidanceInterpSpeed);
	const FVector Result = SmoothedPawnAvoidDir * FMath::Max(0.0f, PawnAvoidanceStrength);

	if (bDrawDebug && !Result.IsNearlyZero())
	{
		DrawDebugLine(World, MyLoc, MyLoc + Result * 2000.0f, FColor::Magenta, false, DebugDrawTime, 0, DebugLineThickness);
	}

	return Result;
}

void UBTTask_MoveShipTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!Ship || !MovementComp)
	{
		if (!bLoggedNoShip)
		{
			bLoggedNoShip = true;
			UE_LOG(LogTemp, Warning, TEXT("[MoveShipTo] Missing Ship/MovementComp"));
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = BB ? Cast<AActor>(BB->GetValueAsObject(TargetActorKeyName)) : nullptr;

	if (!TargetActor)
	{
		if (!bLoggedWaitingForTarget)
		{
			bLoggedWaitingForTarget = true;
			UE_LOG(LogTemp, Warning, TEXT("[MoveShipTo] Waiting for TargetActor"));
		}
		return;
	}
	bLoggedWaitingForTarget = false;

	const FVector MyLoc = Ship->GetActorLocation();
	const FVector TargetLoc = TargetActor->GetActorLocation();

	const float AcceptSq = FMath::Square(FMath::Max(0.0f, AcceptableRadius));
	if (FVector::DistSquared(MyLoc, TargetLoc) <= AcceptSq)
	{
		MovementComp->ClearSteering();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	FVector DesiredDir = (TargetLoc - MyLoc).GetSafeNormal();
	if (DesiredDir.IsNearlyZero())
	{
		return;
	}

	const FVector Forward = Ship->GetActorForwardVector().GetSafeNormal();

	const FVector StaticAvoid = ComputeStaticAvoidDir(DeltaSeconds, MyLoc, Forward);
	const FVector PawnAvoid = ComputePawnAvoidDir(DeltaSeconds, MyLoc);

	DesiredDir = (DesiredDir + StaticAvoid + PawnAvoid).GetSafeNormal();

	const float Lookahead = FMath::Max(100.0f, SteerLookaheadDistance);
	const FVector SteerPoint = MyLoc + DesiredDir * Lookahead;

	if (bDrawDebug)
	{
		if (UWorld* World = Ship->GetWorld())
		{
			DrawDebugLine(World, MyLoc, TargetLoc, FColor::Yellow, false, DebugDrawTime, 0, DebugLineThickness);
			DrawDebugLine(World, MyLoc, SteerPoint, FColor::Cyan, false, DebugDrawTime, 0, DebugLineThickness);
			DrawDebugSphere(World, SteerPoint, DebugHitSphereRadius * 0.7f, 10, FColor::Cyan, false, DebugDrawTime, 0, DebugLineThickness);
		}
	}

	MovementComp->SteerTowardWorldLocation(SteerPoint);
}

EBTNodeResult::Type UBTTask_MoveShipTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (MovementComp)
	{
		MovementComp->ClearSteering();
	}
	return EBTNodeResult::Aborted;
}