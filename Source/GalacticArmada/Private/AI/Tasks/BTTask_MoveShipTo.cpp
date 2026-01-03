#include "AI/Tasks/BTTask_MoveShipTo.h"

#include "AI/AIShipController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

#include "Player/ShipPawn.h"
#include "Player/ShipMovementComponent.h"

UBTTask_MoveShipTo::UBTTask_MoveShipTo()
{
	NodeName = TEXT("Move Ship (Cached Neighbors + Throttled Static Avoid)");
	bNotifyTick = true;
}

uint16 UBTTask_MoveShipTo::GetInstanceMemorySize() const
{
	return sizeof(FMoveMem);
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

	FMoveMem& Mem = *reinterpret_cast<FMoveMem*>(NodeMemory);
	Mem.NextStaticProbeTime = 0.0f;
	Mem.SmoothedStaticAvoid = FVector::ZeroVector;
	Mem.SmoothedPawnAvoid = FVector::ZeroVector;

	bLoggedNoShip = false;
	bLoggedWaitingForTarget = false;

	return EBTNodeResult::InProgress;
}

bool UBTTask_MoveShipTo::ProbeStaticAhead(const FVector& From, const FVector& Forward, FHitResult& OutHit) const
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

FVector UBTTask_MoveShipTo::UpdateStaticAvoid(float DeltaSeconds, float Now, FMoveMem& Mem, const FVector& MyLoc, const FVector& Forward)
{
	const float Interp = FMath::Max(0.0f, StaticAvoidanceInterpSpeed);

	if (!bEnableStaticAvoidance)
	{
		Mem.SmoothedStaticAvoid = FMath::VInterpTo(Mem.SmoothedStaticAvoid, FVector::ZeroVector, DeltaSeconds, Interp);
		return Mem.SmoothedStaticAvoid;
	}

	// Rate-limit the expensive sweep.
	if (Now < Mem.NextStaticProbeTime)
	{
		return Mem.SmoothedStaticAvoid;
	}

	Mem.NextStaticProbeTime = Now + FMath::Max(0.05f, StaticProbeInterval);

	FHitResult Hit;
	if (!ProbeStaticAhead(MyLoc, Forward, Hit))
	{
		Mem.SmoothedStaticAvoid = FMath::VInterpTo(Mem.SmoothedStaticAvoid, FVector::ZeroVector, DeltaSeconds, Interp);
		return Mem.SmoothedStaticAvoid;
	}

	const float AvoidDist = FMath::Max(0.0f, StaticAvoidanceDistance);
	const float DistToHit = FVector::Distance(MyLoc, Hit.ImpactPoint);

	if (AvoidDist <= KINDA_SMALL_NUMBER || DistToHit >= AvoidDist)
	{
		Mem.SmoothedStaticAvoid = FMath::VInterpTo(Mem.SmoothedStaticAvoid, FVector::ZeroVector, DeltaSeconds, Interp);
		return Mem.SmoothedStaticAvoid;
	}

	FVector Away = (MyLoc - Hit.ImpactPoint).GetSafeNormal();
	if (Away.IsNearlyZero())
	{
		Away = Hit.ImpactNormal.GetSafeNormal();
	}

	const float Alpha = 1.0f - FMath::Clamp(DistToHit / AvoidDist, 0.0f, 1.0f);
	const float W = Alpha * Alpha;
	const float Strength = W * FMath::Max(0.0f, StaticAvoidanceStrength);

	Mem.SmoothedStaticAvoid = FMath::VInterpTo(Mem.SmoothedStaticAvoid, Away * Strength, DeltaSeconds, Interp);

	if (bDrawDebug)
	{
		if (UWorld* World = Ship ? Ship->GetWorld() : nullptr)
		{
			DrawDebugLine(World, MyLoc, Hit.ImpactPoint, FColor::Red, false, DebugDrawTime, 0, DebugLineThickness);
			DrawDebugSphere(World, Hit.ImpactPoint, DebugHitSphereRadius, 12, FColor::Red, false, DebugDrawTime, 0, DebugLineThickness);
		}
	}

	return Mem.SmoothedStaticAvoid;
}

FVector UBTTask_MoveShipTo::UpdatePawnAvoid(float DeltaSeconds, FMoveMem& Mem, const FVector& MyLoc) const
{
	const float Interp = FMath::Max(0.0f, PawnAvoidanceInterpSpeed);

	if (!bEnablePawnAvoidance)
	{
		Mem.SmoothedPawnAvoid = FMath::VInterpTo(Mem.SmoothedPawnAvoid, FVector::ZeroVector, DeltaSeconds, Interp);
		return Mem.SmoothedPawnAvoid;
	}

	const AAIShipController* AI = Ship ? Cast<AAIShipController>(Ship->GetController()) : nullptr;
	if (!AI)
	{
		Mem.SmoothedPawnAvoid = FMath::VInterpTo(Mem.SmoothedPawnAvoid, FVector::ZeroVector, DeltaSeconds, Interp);
		return Mem.SmoothedPawnAvoid;
	}

	const TArray<TWeakObjectPtr<APawn>>& Neigh = AI->GetAvoidanceNeighbors();

	const float SepR = FMath::Max(0.0f, PawnSeparationDistance);
	if (SepR <= KINDA_SMALL_NUMBER || PawnMaxNeighbors <= 0 || Neigh.Num() <= 0)
	{
		Mem.SmoothedPawnAvoid = FMath::VInterpTo(Mem.SmoothedPawnAvoid, FVector::ZeroVector, DeltaSeconds, Interp);
		return Mem.SmoothedPawnAvoid;
	}

	// Keep “closest N” without allocating arrays:
	// We scan once and accumulate using a simple "worst slot replacement" over a small fixed N.
	const int32 N = FMath::Clamp(PawnMaxNeighbors, 1, 32);

	struct FCand
	{
		APawn* Pawn = nullptr;
		float DistSq = 0.0f;
	};

	FCand Cands[32];
	int32 Count = 0;

	const float SepRSq = SepR * SepR;

	for (const TWeakObjectPtr<APawn>& W : Neigh)
	{
		APawn* Other = W.Get();
		if (!Other || Other == Ship)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLoc, Other->GetActorLocation());
		if (DistSq >= SepRSq)
		{
			continue;
		}

		if (Count < N)
		{
			Cands[Count++] = { Other, DistSq };
			continue;
		}

		// replace farthest
		int32 FarthestIdx = 0;
		float FarthestDist = Cands[0].DistSq;
		for (int32 i = 1; i < Count; ++i)
		{
			if (Cands[i].DistSq > FarthestDist)
			{
				FarthestDist = Cands[i].DistSq;
				FarthestIdx = i;
			}
		}

		if (DistSq < FarthestDist)
		{
			Cands[FarthestIdx] = { Other, DistSq };
		}
	}

	if (Count <= 0)
	{
		Mem.SmoothedPawnAvoid = FMath::VInterpTo(Mem.SmoothedPawnAvoid, FVector::ZeroVector, DeltaSeconds, Interp);
		return Mem.SmoothedPawnAvoid;
	}

	FVector Accum = FVector::ZeroVector;
	float WeightSum = 0.0f;

	for (int32 i = 0; i < Count; ++i)
	{
		APawn* Other = Cands[i].Pawn;
		if (!Other)
		{
			continue;
		}

		const float DistSq = Cands[i].DistSq;
		if (DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Dist = FMath::Sqrt(DistSq);

		// weight falls off to 0 at SepR
		const float Alpha = 1.0f - FMath::Clamp(Dist / SepR, 0.0f, 1.0f);
		const float W = Alpha * Alpha;

		const FVector Away = (MyLoc - Other->GetActorLocation()) / Dist; // normalized
		Accum += Away * W;
		WeightSum += W;
	}

	FVector Desired = FVector::ZeroVector;
	if (WeightSum > KINDA_SMALL_NUMBER)
	{
		Desired = (Accum / WeightSum).GetSafeNormal();
	}

	const float Strength = FMath::Max(0.0f, PawnAvoidanceStrength);
	Mem.SmoothedPawnAvoid = FMath::VInterpTo(Mem.SmoothedPawnAvoid, Desired * Strength, DeltaSeconds, Interp);

	if (bDrawDebug && Ship && Ship->GetWorld() && !Mem.SmoothedPawnAvoid.IsNearlyZero())
	{
		DrawDebugLine(Ship->GetWorld(), MyLoc, MyLoc + Mem.SmoothedPawnAvoid * 2000.0f, FColor::Magenta, false, DebugDrawTime, 0, DebugLineThickness);
	}

	return Mem.SmoothedPawnAvoid;
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

	FMoveMem& Mem = *reinterpret_cast<FMoveMem*>(NodeMemory);

	const float Now = Ship->GetWorld() ? Ship->GetWorld()->GetTimeSeconds() : 0.0f;

	const FVector Forward = Ship->GetActorForwardVector().GetSafeNormal();

	const FVector StaticAvoid = UpdateStaticAvoid(DeltaSeconds, Now, Mem, MyLoc, Forward);
	const FVector PawnAvoid = UpdatePawnAvoid(DeltaSeconds, Mem, MyLoc);

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