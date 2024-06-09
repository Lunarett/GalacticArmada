#include "Components/ShipMovementComponent.h"

UShipMovementComponent::UShipMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize default values
	RudderAngle = 30.0f;
	RudderSpeed = 10.0f;

	ElevatorAngle = 30.0f;
	ElevatorSpeed = 10.0f;

	FlapAngle = 30.0f;
	FlapSpeed = 10.0f;

	MaxSpeed = 10000.0f;
	MinSpeed = 0.0f;
	Acceleration = 500.0f;
	Deceleration = 300.0f;

	CurrentYawInput = 0.0f;
	CurrentPitchInput = 0.0f;
	CurrentRollInput = 0.0f;
	CurrentThrustInput = 0.0f;

	CurrentYaw = 0.0f;
	CurrentPitch = 0.0f;
	CurrentRoll = 0.0f;
	CurrentSpeed = 0.0f;
}

void UShipMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateRotationMovement(DeltaTime, CurrentRoll, CurrentRollInput, FlapAngle, FlapSpeed, FRotator(0.0f, 0.0f, 1.0f));
	UpdateRotationMovement(DeltaTime, CurrentPitch, -CurrentPitchInput, ElevatorAngle, ElevatorSpeed, FRotator(1.0f, 0.0f, 0.0f));
	UpdateRotationMovement(DeltaTime, CurrentYaw, CurrentYawInput, RudderAngle, RudderSpeed, FRotator(0.0f, 1.0f, 0.0f));
	UpdateThrustMovement(DeltaTime);
}

void UShipMovementComponent::SetYawInput(float InputValue)
{
	CurrentYawInput = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SetPitchInput(float InputValue)
{
	CurrentPitchInput = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SetRollInput(float InputValue)
{
	CurrentRollInput = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SetThrustInput(float InputValue)
{
	CurrentThrustInput = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::UpdateRotationMovement(float DeltaSeconds, float& CurrentValue, float InputValue, float MaxAngle,
	float Speed, FRotator RotationAxis)
{
	const float TargetValue = InputValue * MaxAngle;
	CurrentValue = FMath::FInterpTo(CurrentValue, TargetValue, DeltaSeconds, Speed);
	FHitResult HitResult;
	GetOwner()->AddActorLocalRotation(RotationAxis * CurrentValue * DeltaSeconds, true, &HitResult, ETeleportType::TeleportPhysics);
}

void UShipMovementComponent::UpdateThrustMovement(float DeltaSeconds)
{
	const float TargetSpeed = FMath::Clamp(CurrentThrustInput * MaxSpeed, MinSpeed, MaxSpeed);
	CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaSeconds, CurrentThrustInput > 0 ? Acceleration : Deceleration);
	FHitResult HitResult;
	GetOwner()->AddActorWorldOffset(GetOwner()->GetActorForwardVector() * CurrentSpeed * DeltaSeconds, true, &HitResult, ETeleportType::TeleportPhysics);
}
