#include "Controllers/ShipAIController.h"
#include "ChaosVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Pawns/ShipPawn.h"
#include "DrawDebugHelpers.h"

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

    // Set the default target rotation to the target ship rotation
    TargetRotation = GetTargetShipRotation();

    // Update collision avoidance to modify the target rotation if necessary
    UpdateCollisionAvoidance();

    // Update movement based on the modified target rotation
    UpdateMovement(DeltaSeconds);
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
    const FVector CollisionLocation = ControlledShipPawn->GetClosestCollisionLocation();
    if (CollisionLocation == FVector::ZeroVector) return;

    const FVector PawnLocation = ControlledShipPawn->GetActorLocation();
    float DistanceToCollision = FVector::Distance(PawnLocation, CollisionLocation);

    if (DistanceToCollision < ControlledShipPawn->ObstacleAvoidanceDistance)
    {
        // Calculate avoidance direction
        FVector AwayFromCollisionDirection = (PawnLocation - CollisionLocation).GetSafeNormal();

        // Calculate the strength of the avoidance based on distance to collision
        float AvoidanceStrength = UKismetMathLibrary::MapRangeClamped(DistanceToCollision, 0.0f, ControlledShipPawn->ObstacleAvoidanceDistance, 1.0f, 0.0f);

        // Adjust the target rotation by blending it with the avoidance direction
        FRotator AvoidanceRotation = UKismetMathLibrary::MakeRotFromX(AwayFromCollisionDirection) * AvoidanceStrength;
        TargetRotation = FMath::Lerp(TargetRotation, TargetRotation + AvoidanceRotation, AvoidanceStrength);

        // Debug visuals for collision avoidance
        DrawDebugLine(GetWorld(), PawnLocation, CollisionLocation, FColor::Red, false, 0.0f, 0, 50.0f);
        DrawDebugSphere(GetWorld(), CollisionLocation, 200.0f, 12, FColor::Red, false, 0.0f, 0, 30.0f);
    }
}

void AShipAIController::UpdateMovement(float DeltaSeconds)
{
    if (!ControlledShipPawn) return;

    // Apply the updated target rotation to the ship's movement component
    ControlledShipPawn->GetVehicleMovementComponent()->SetRollInput(RotationToInputAxis(TargetRotation.Roll, 180.0f));
    ControlledShipPawn->GetVehicleMovementComponent()->SetPitchInput(-RotationToInputAxis(TargetRotation.Pitch, 90.0f));
    ControlledShipPawn->GetVehicleMovementComponent()->SetYawInput(RotationToInputAxis(TargetRotation.Yaw, 180.0f));
    ControlledShipPawn->GetVehicleMovementComponent()->SetThrottleInput(IsInRange() ? 1 : 0);

    // Debug visual for the final movement direction
    FVector PawnLocation = ControlledShipPawn->GetActorLocation();
    FVector ForwardVector = ControlledShipPawn->GetActorForwardVector();
    DrawDebugLine(GetWorld(), PawnLocation, PawnLocation + ForwardVector * 500.0f, FColor::Blue, false, 0.0f, 0, 50.0f);
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