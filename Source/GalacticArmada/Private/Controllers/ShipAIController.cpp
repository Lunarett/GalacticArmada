#include "Controllers/ShipAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Pawns/ShipPawn.h"
#include "DrawDebugHelpers.h"
#include "Components/CannonComponent.h"
#include "Components/ShipMovementComponent.h"

void AShipAIController::BeginPlay()
{
    Super::BeginPlay();
    ControlledShipPawn = Cast<AShipPawn>(GetPawn());

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShipPawn::StaticClass(), FoundActors);
    for (AActor* Actor : FoundActors)
    {
        if (AShipPawn* ShipPawn = Cast<AShipPawn>(Actor))
        {
            if (!ShipPawn->Controller->IsPlayerController()) continue;
            TargetShipPawn = ShipPawn;
            break;
        }
    }
}

void AShipAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsValid(TargetShipPawn) || !IsValid(ControlledShipPawn)) return;

    TargetRotation = GetTargetShipRotation();
    UpdateCollisionAvoidance();
    UpdateMovement(DeltaSeconds);
    UpdateFiring();
}

FRotator AShipAIController::GetTargetShipRotation() const
{
    if (!ControlledShipPawn) return FRotator().ZeroRotator;
    const FVector Direction = TargetShipPawn->GetActorLocation() - ControlledShipPawn->GetActorLocation();
    const FVector NormalizedDirection = Direction.GetSafeNormal();
    const FRotator DirectionRotation = UKismetMathLibrary::MakeRotFromX(NormalizedDirection);
    return UKismetMathLibrary::NormalizedDeltaRotator(DirectionRotation, GetPawn()->GetActorRotation());
}

void AShipAIController::UpdateCollisionAvoidance()
{
    if (!IsValid(ControlledShipPawn)) return;
    const FVector CollisionLocation = ControlledShipPawn->GetClosestCollisionLocation();
    if (CollisionLocation == FVector::ZeroVector) return;

    const FVector PawnLocation = ControlledShipPawn->GetActorLocation();
    const float DistanceToCollision = FVector::Distance(PawnLocation, CollisionLocation);

    if (DistanceToCollision < ControlledShipPawn->ObstacleAvoidanceDistance)
    {
        // Calculate Avoidance Direction
        const FVector AwayFromCollisionDirection = (PawnLocation - CollisionLocation).GetSafeNormal();

        // Calculate the strength of the avoidance based on distance to collision
        const float AvoidanceStrength = UKismetMathLibrary::MapRangeClamped(DistanceToCollision, 0.0f, ControlledShipPawn->ObstacleAvoidanceDistance, ControlledShipPawn->MaxAvoidanceStrength, ControlledShipPawn->MinAvoidanceStrength);

        // Adjust the target rotation by blending it with the avoidance direction
        const FRotator AvoidanceRotation = UKismetMathLibrary::MakeRotFromX(AwayFromCollisionDirection) * AvoidanceStrength;
        TargetRotation = FMath::Lerp(TargetRotation, TargetRotation + AvoidanceRotation, AvoidanceStrength);

        if (bEnableAvoidanceDebug)
        {
            DrawDebugLine(GetWorld(), ControlledShipPawn->GetActorLocation(), CollisionLocation, FColor::Red, false, 0.0f, 0, 60);
            DrawDebugSphere(GetWorld(), CollisionLocation, 300, 12, FColor::Red, false, 0.0f, 0, 30);
        }
    }
}

void AShipAIController::UpdateMovement(float DeltaSeconds) const
{
    if (!ControlledShipPawn) return;

    // Apply Target Rotation
    ControlledShipPawn->GetShipMovementComponent()->SetRollInput(RotationToInputAxis(TargetRotation.Roll, 180.0f));
    ControlledShipPawn->GetShipMovementComponent()->SetPitchInput(-RotationToInputAxis(TargetRotation.Pitch, 90.0f));
    ControlledShipPawn->GetShipMovementComponent()->SetYawInput(RotationToInputAxis(TargetRotation.Yaw, 180.0f));
    ControlledShipPawn->GetShipMovementComponent()->SetThrustInput(IsInRange() ? 1.0f : -1.0f);
}

float AShipAIController::RotationToInputAxis(float Value, float RotVal)
{
    return UKismetMathLibrary::MapRangeClamped(Value, -RotVal, RotVal, -1.0f, 1.0f);
}

bool AShipAIController::IsInRange() const
{
    const float Distance = FVector::Distance(ControlledShipPawn->GetActorLocation(), TargetShipPawn->GetActorLocation());
    return Distance > ControlledShipPawn->StoppingDistance;
}

void AShipAIController::UpdateFiring() const
{
    if (!ControlledShipPawn || !TargetShipPawn) return;

    const float DistanceToTarget = FVector::Distance(ControlledShipPawn->GetActorLocation(), TargetShipPawn->GetActorLocation());

    if (DistanceToTarget <= ControlledShipPawn->PrimaryFireRange)
    {
        // Fire Primary Cannons
        ControlledShipPawn->GetCannonComponent()->BeginCannonFire(0);
        ControlledShipPawn->GetCannonComponent()->EndCannonFire(1);
    }
    else if (DistanceToTarget <= ControlledShipPawn->SecondaryFireRange)
    {
        // Fire Secondary Cannons
        ControlledShipPawn->GetCannonComponent()->BeginCannonFire(1);
        ControlledShipPawn->GetCannonComponent()->EndCannonFire(0);
    }
    else
    {
        // Stop All Fire
        ControlledShipPawn->GetCannonComponent()->EndCannonFire(0);
        ControlledShipPawn->GetCannonComponent()->EndCannonFire(1);
    }
}
