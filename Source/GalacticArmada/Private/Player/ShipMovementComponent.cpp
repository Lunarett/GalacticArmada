#include "Player/ShipMovementComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogShipMovementComponent);

UShipMovementComponent::UShipMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShipMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveUpdatedComponentReference();
}

void UShipMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!UpdatedComponent)
	{
		ResolveUpdatedComponentReference();
		if (!UpdatedComponent)
		{
			return;
		}
	}

	UpdateSteeringInputs();
	SelectInputTargets();
	SmoothInputTargets(DeltaTime);
	ApplyRotation(DeltaTime);
	ApplyTranslation(DeltaTime);
}

void UShipMovementComponent::ResolveUpdatedComponentReference()
{
	if (UpdatedComponent)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	SetUpdatedComponent(Owner->GetRootComponent());
}

void UShipMovementComponent::SetThrottleInput(float InputValue)
{
	PlayerThrottleTarget = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SetRollInput(float InputValue)
{
	PlayerRollTarget = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SetPitchInput(float InputValue)
{
	PlayerPitchTarget = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SetYawInput(float InputValue)
{
	PlayerYawTarget = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SteerTowardWorldDirection(const FVector& WorldDirection)
{
	bHasSteeringTargetLocation = false;
	SteeringTargetDirectionWorldSpace = WorldDirection;
}

void UShipMovementComponent::SteerTowardWorldLocation(const FVector& WorldLocation)
{
	bHasSteeringTargetLocation = true;
	SteeringTargetLocationWorldSpace = WorldLocation;
}

void UShipMovementComponent::ClearSteering()
{
	bHasSteeringTargetLocation = false;
	SteeringTargetDirectionWorldSpace = FVector::ZeroVector;
	SteeringThrottleTarget = 0.0f;
	SteeringRollTarget = 0.0f;
	SteeringPitchTarget = 0.0f;
	SteeringYawTarget = 0.0f;
}

void UShipMovementComponent::UpdateSteeringInputs()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FVector DesiredDirection = FVector::ZeroVector;

	if (bHasSteeringTargetLocation)
	{
		const FVector ToTarget = SteeringTargetLocationWorldSpace - Owner->GetActorLocation();
		if (ArrivalRadius > 0.0f && ToTarget.SizeSquared() <= FMath::Square(ArrivalRadius))
		{
			ClearSteering();
			return;
		}

		DesiredDirection = ToTarget;
	}
	else
	{
		DesiredDirection = SteeringTargetDirectionWorldSpace;
	}

	if (!DesiredDirection.Normalize())
	{
		ClearSteering();
		return;
	}

	const FVector Forward = GetForwardVectorWorldSpace();
	const FVector Right = Owner->GetActorRightVector();
	const FVector Up = Owner->GetActorUpVector();
	const FVector WorldUp = FVector::UpVector;

	const float ForwardDot = FVector::DotProduct(Forward, DesiredDirection);
	const float AngleToTargetDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(ForwardDot, -1.0f, 1.0f)));

	// Convert desired direction into local-space components.
	// This allows yaw and pitch errors to be computed independently.
	const float LocalForward = FVector::DotProduct(DesiredDirection, Forward);
	const float LocalRight = FVector::DotProduct(DesiredDirection, Right);
	const float LocalUp = FVector::DotProduct(DesiredDirection, Up);

	float YawErrorDegrees = SafeAtan2Degrees(LocalRight, LocalForward);
	float PitchErrorDegrees = SafeAtan2Degrees(LocalUp, LocalForward);

	if (FMath::Abs(YawErrorDegrees) < AimDeadzoneDegrees)
	{
		YawErrorDegrees = 0.0f;
	}

	if (FMath::Abs(PitchErrorDegrees) < AimDeadzoneDegrees)
	{
		PitchErrorDegrees = 0.0f;
	}

	float YawCommand = (YawErrorDegrees / FMath::Max(FullYawInputAtDegrees, 1.0f)) * YawResponseGain;
	float PitchCommand = (PitchErrorDegrees / FMath::Max(FullPitchInputAtDegrees, 1.0f)) * PitchResponseGain;

	YawCommand = FMath::Clamp(YawCommand, -1.0f, 1.0f);
	PitchCommand = FMath::Clamp(PitchCommand, -1.0f, 1.0f);

	float RollCommand = FMath::Clamp(YawCommand * RollFromYawFactor, -1.0f, 1.0f);

	// Auto-leveling:
	// Construct an upright orientation that preserves forward direction.
	// Roll correction is extracted from the delta between current and upright rotation.
	if (FMath::Abs(YawErrorDegrees) < AutoLevelStartYawDegrees)
	{
		const FQuat CurrentRotation = Owner->GetActorQuat();
		const FQuat UprightRotation = FRotationMatrix::MakeFromXZ(Forward, WorldUp).ToQuat();
		const FQuat DeltaRotation = CurrentRotation.Inverse() * UprightRotation;

		const float RollErrorDegrees = NormalizeAngleDegrees(DeltaRotation.Rotator().Roll);
		const float LevelingCommand = FMath::Clamp(
			(RollErrorDegrees / FMath::Max(MaximumBankAngleDegrees, 1.0f)) * AutoLevelStrength,
			-1.0f,
			1.0f
		);

		const float BlendAlpha = 1.0f - FMath::Clamp(
			FMath::Abs(YawErrorDegrees) / FMath::Max(AutoLevelStartYawDegrees, 0.001f),
			0.0f,
			1.0f
		);

		RollCommand = FMath::Lerp(RollCommand, LevelingCommand, BlendAlpha);
	}

	float ThrottleCommand = 1.0f;

	if (AngleToTargetDegrees >= ThrottleStopAngleDegrees)
	{
		ThrottleCommand = bBrakeWhenSeverelyMisaligned ? -1.0f : 0.0f;
	}
	else if (AngleToTargetDegrees > ThrottleSlowdownAngleDegrees)
	{
		const float InterpolationAlpha =
			(AngleToTargetDegrees - ThrottleSlowdownAngleDegrees) /
			FMath::Max(ThrottleStopAngleDegrees - ThrottleSlowdownAngleDegrees, 0.001f);

		ThrottleCommand = FMath::Lerp(1.0f, MinimumThrottleWhenOffTarget, FMath::Clamp(InterpolationAlpha, 0.0f, 1.0f));
	}

	SteeringThrottleTarget = FMath::Clamp(ThrottleCommand, -1.0f, 1.0f);
	SteeringYawTarget = YawCommand;
	SteeringPitchTarget = PitchCommand;
	SteeringRollTarget = RollCommand;

	if (bDrawDebug && GetWorld())
	{
		const FVector Location = Owner->GetActorLocation();
		DrawDebugLine(GetWorld(), Location, Location + Forward * DebugDrawScale, FColor::Green, false, 0.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Location, Location + DesiredDirection * DebugDrawScale, FColor::Red, false, 0.0f, 0, 3.0f);
	}
}

void UShipMovementComponent::SelectInputTargets()
{
	TargetThrottle = SteeringThrottleTarget != 0.0f ? SteeringThrottleTarget : PlayerThrottleTarget;
	TargetRoll = SteeringRollTarget != 0.0f ? SteeringRollTarget : PlayerRollTarget;
	TargetPitch = SteeringPitchTarget != 0.0f ? SteeringPitchTarget : PlayerPitchTarget;
	TargetYaw = SteeringYawTarget != 0.0f ? SteeringYawTarget : PlayerYawTarget;
}

void UShipMovementComponent::SmoothInputTargets(float DeltaTime)
{
	const float Smoothing = FMath::Max(InputSmoothingSpeed, 0.0f);

	SmoothedThrottle = FMath::FInterpTo(SmoothedThrottle, TargetThrottle, DeltaTime, Smoothing);
	SmoothedRoll = FMath::FInterpTo(SmoothedRoll, TargetRoll, DeltaTime, Smoothing);
	SmoothedPitch = FMath::FInterpTo(SmoothedPitch, TargetPitch, DeltaTime, Smoothing);
	SmoothedYaw = FMath::FInterpTo(SmoothedYaw, TargetYaw, DeltaTime, Smoothing);
}

void UShipMovementComponent::ApplyRotation(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FRotator DeltaRotation(
		SmoothedPitch * MaximumPitchRateDegrees * DeltaTime,
		SmoothedYaw * MaximumYawRateDegrees * DeltaTime,
		SmoothedRoll * MaximumRollRateDegrees * DeltaTime
	);

	Owner->AddActorLocalRotation(DeltaRotation, true);
}

void UShipMovementComponent::ApplyTranslation(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const float TargetSpeed = FMath::Clamp(SmoothedThrottle * MaximumSpeed, MinimumSpeed, MaximumSpeed);
	const bool bIsBraking = SmoothedThrottle < -0.2f;

	float SpeedChangeRate = DecelerationRate;

	if (TargetSpeed > CurrentSpeed)
	{
		SpeedChangeRate = AccelerationRate;
	}
	else if (bIsBraking)
	{
		SpeedChangeRate = BrakeDecelerationRate;
	}

	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, TargetSpeed, DeltaTime, SpeedChangeRate);

	FHitResult Hit;
	Owner->AddActorWorldOffset(GetForwardVectorWorldSpace() * CurrentSpeed * DeltaTime, true, &Hit);
}

FVector UShipMovementComponent::GetForwardVectorWorldSpace() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorForwardVector().GetSafeNormal() : FVector::ForwardVector;
}

float UShipMovementComponent::SafeAtan2Degrees(float Y, float X)
{
	const float SafeX = FMath::Abs(X) < 0.001f ? (X >= 0.0f ? 0.001f : -0.001f) : X;
	return FMath::RadiansToDegrees(FMath::Atan2(Y, SafeX));
}

float UShipMovementComponent::NormalizeAngleDegrees(float Degrees)
{
	while (Degrees > 180.0f)
	{
		Degrees -= 360.0f;
	}

	while (Degrees < -180.0f)
	{
		Degrees += 360.0f;
	}

	return Degrees;
}