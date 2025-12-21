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
	ResolveUpdatedComponent();
}

void UShipMovementComponent::ResolveUpdatedComponent()
{
	if (!UpdatedComponent)
	{
		if (AActor* Owner = GetOwner())
		{
			SetUpdatedComponent(Owner->GetRootComponent());
		}
	}
}

void UShipMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (!UpdatedComponent)
	{
		ResolveUpdatedComponent();
		if (!UpdatedComponent) return;
	}

	if (bAIEnabled)
	{
		UpdateAIInputs();
	}

	SelectTargets();
	SmoothInputs(DeltaTime);

	ApplyArcadeRotation(DeltaTime);
	ApplyArcadeTranslation(DeltaTime);
}

void UShipMovementComponent::SetThrottleInput(float InputValue)
{
	PlayerThrottleTarget = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void UShipMovementComponent::SetRollInput(float InputValue)
{
	float V = FMath::Clamp(InputValue, -1.0f, 1.0f);
	if (bInvertRoll) V *= -1.0f;
	PlayerRollTarget = V;
}

void UShipMovementComponent::SetPitchInput(float InputValue)
{
	float V = FMath::Clamp(InputValue, -1.0f, 1.0f);
	if (bInvertPitch) V *= -1.0f;
	PlayerPitchTarget = V;
}

void UShipMovementComponent::SetYawInput(float InputValue)
{
	float V = FMath::Clamp(InputValue, -1.0f, 1.0f);
	if (bInvertYaw) V *= -1.0f;
	PlayerYawTarget = V;
}

void UShipMovementComponent::SetAIEnabled(bool bEnabled)
{
	bAIEnabled = bEnabled;

	if (!bAIEnabled)
	{
		bAIHasTargetLocation = false;
		AITargetLocationWS = FVector::ZeroVector;
		AIDesiredDirectionWS = FVector::ZeroVector;

		AIThrottleTarget = 0.0f;
		AIRollTarget = 0.0f;
		AIPitchTarget = 0.0f;
		AIYawTarget = 0.0f;
	}
}

void UShipMovementComponent::StopSteering()
{
	SetAIEnabled(false);
}

void UShipMovementComponent::SteerTowardDirection(const FVector& WorldDirection)
{
	bAIEnabled = true;
	bAIHasTargetLocation = false;
	AIDesiredDirectionWS = WorldDirection;
}

void UShipMovementComponent::SteerTowardLocation(const FVector& WorldLocation)
{
	bAIEnabled = true;
	bAIHasTargetLocation = true;
	AITargetLocationWS = WorldLocation;
}

FVector UShipMovementComponent::GetEffectiveForwardWS() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return FVector::ForwardVector;

	FVector Fwd = Owner->GetActorForwardVector().GetSafeNormal();
	if (bInvertForward)
	{
		Fwd *= -1.0f;
	}
	return Fwd;
}

float UShipMovementComponent::SafeAtan2Deg(float Y, float X)
{
	const float SafeX = (FMath::Abs(X) < 0.001f) ? (X >= 0.0f ? 0.001f : -0.001f) : X;
	return FMath::RadiansToDegrees(FMath::Atan2(Y, SafeX));
}

float UShipMovementComponent::Wrap180(float Deg)
{
	while (Deg > 180.0f) Deg -= 360.0f;
	while (Deg < -180.0f) Deg += 360.0f;
	return Deg;
}

void UShipMovementComponent::UpdateAIInputs()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector DesiredDir = FVector::ZeroVector;

	if (bAIHasTargetLocation)
	{
		const FVector ToTarget = AITargetLocationWS - Owner->GetActorLocation();

		if (ArrivalRadius > 0.0f && ToTarget.SizeSquared() <= FMath::Square(ArrivalRadius))
		{
			AIThrottleTarget = 0.0f;
			AIRollTarget = 0.0f;
			AIPitchTarget = 0.0f;
			AIYawTarget = 0.0f;
			return;
		}

		DesiredDir = ToTarget;
	}
	else
	{
		DesiredDir = AIDesiredDirectionWS;
	}

	if (!DesiredDir.Normalize())
	{
		AIThrottleTarget = 0.0f;
		AIRollTarget = 0.0f;
		AIPitchTarget = 0.0f;
		AIYawTarget = 0.0f;
		return;
	}

	const FVector Forward = GetEffectiveForwardWS();
	const FVector Right = Owner->GetActorRightVector().GetSafeNormal();
	const FVector Up = Owner->GetActorUpVector().GetSafeNormal();
	const FVector WorldUp = FVector::UpVector;

	// Angle-to-target for throttle gating
	const float DotFD = FMath::Clamp(FVector::DotProduct(Forward, DesiredDir), -1.0f, 1.0f);
	const float AngleToTargetDeg = FMath::RadiansToDegrees(FMath::Acos(DotFD));

	// Local components (for yaw/pitch errors)
	const float LocalX = FVector::DotProduct(DesiredDir, Forward);
	const float LocalY = FVector::DotProduct(DesiredDir, Right);
	const float LocalZ = FVector::DotProduct(DesiredDir, Up);

	float YawErrDeg   = SafeAtan2Deg(LocalY, LocalX);
	float PitchErrDeg = SafeAtan2Deg(LocalZ, LocalX);

	if (FMath::Abs(YawErrDeg) < AimDeadzoneDeg)   YawErrDeg = 0.0f;
	if (FMath::Abs(PitchErrDeg) < AimDeadzoneDeg) PitchErrDeg = 0.0f;

	float YawCmd   = (YawErrDeg / FMath::Max(YawFullInputAtDeg, 1.0f)) * YawGain;
	float PitchCmd = (PitchErrDeg / FMath::Max(PitchFullInputAtDeg, 1.0f)) * PitchGain;

	YawCmd   = FMath::Clamp(YawCmd,   -1.0f, 1.0f);
	PitchCmd = FMath::Clamp(PitchCmd, -1.0f, 1.0f);

	float RollCmd = FMath::Clamp(YawCmd * RollFromYaw, -1.0f, 1.0f);

	// ============================
	// FORCE UPRIGHT AUTO-LEVEL
	// ============================
	if (bForceUprightLeveling && FMath::Abs(YawErrDeg) < AutoLevelStartDeg)
	{
		// Create an "upright" orientation that preserves forward but uses WorldUp.
		// This completely eliminates the upside-down solution.
		const FQuat CurrentQ = Owner->GetActorQuat();
		const FQuat UprightQ = FRotationMatrix::MakeFromXZ(Forward, WorldUp).ToQuat();

		// Compute the delta from current to upright in the ship's local frame
		const FQuat DeltaQ = CurrentQ.Inverse() * UprightQ;
		const FRotator DeltaR = DeltaQ.Rotator();

		// We only want roll correction here.
		const float RollErrorDeg = Wrap180(DeltaR.Roll);

		float LevelCmd = FMath::Clamp((RollErrorDeg / FMath::Max(BankMaxDeg, 1.0f)) * AutoLevelGain, -1.0f, 1.0f);

		// Blend: when yaw error near 0 -> strong leveling; as yaw grows -> more bank-into-turn
		const float Alpha = 1.0f - FMath::Clamp(FMath::Abs(YawErrDeg) / FMath::Max(AutoLevelStartDeg, 0.001f), 0.0f, 1.0f);
		RollCmd = FMath::Lerp(RollCmd, LevelCmd, Alpha);
	}

	// Throttle control to avoid orbiting
	float ThrottleCmd = AIThrottle;

	if (AngleToTargetDeg >= StopThrustAngleDeg)
	{
		ThrottleCmd = bBrakeWhenVeryOffTarget ? -1.0f : 0.0f;
	}
	else if (AngleToTargetDeg > SlowdownStartAngleDeg)
	{
		const float T = (AngleToTargetDeg - SlowdownStartAngleDeg) / FMath::Max(StopThrustAngleDeg - SlowdownStartAngleDeg, 0.001f);
		const float Alpha = FMath::Clamp(T, 0.0f, 1.0f);
		ThrottleCmd = FMath::Lerp(AIThrottle, MinThrottleWhenOffTarget, Alpha);
	}

	ThrottleCmd = FMath::Clamp(ThrottleCmd, -1.0f, 1.0f);

	// Apply inversion flags
	if (bInvertYaw)   YawCmd   *= -1.0f;
	if (bInvertPitch) PitchCmd *= -1.0f;
	if (bInvertRoll)  RollCmd  *= -1.0f;

	AIThrottleTarget = ThrottleCmd;
	AIYawTarget = YawCmd;
	AIPitchTarget = PitchCmd;
	AIRollTarget = RollCmd;

	if (bDebugDraw && GetWorld())
	{
		const FVector Loc = Owner->GetActorLocation();
		const float S = DebugDrawScale;
		DrawDebugLine(GetWorld(), Loc, Loc + Forward * S,    FColor::Green, false, 0.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Loc, Loc + DesiredDir * S, FColor::Red,   false, 0.0f, 0, 3.0f);
	}
}

void UShipMovementComponent::SelectTargets()
{
	if (bAIEnabled)
	{
		TargetThrottle = AIThrottleTarget;
		TargetRoll = AIRollTarget;
		TargetPitch = AIPitchTarget;
		TargetYaw = AIYawTarget;
	}
	else
	{
		TargetThrottle = PlayerThrottleTarget;
		TargetRoll = PlayerRollTarget;
		TargetPitch = PlayerPitchTarget;
		TargetYaw = PlayerYawTarget;
	}
}

void UShipMovementComponent::SmoothInputs(float DeltaTime)
{
	const float S = FMath::Max(InputSmoothing, 0.0f);

	ThrottleInput = FMath::FInterpTo(ThrottleInput, TargetThrottle, DeltaTime, S);
	RollInput     = FMath::FInterpTo(RollInput,     TargetRoll,     DeltaTime, S);
	PitchInput    = FMath::FInterpTo(PitchInput,    TargetPitch,    DeltaTime, S);
	YawInput      = FMath::FInterpTo(YawInput,      TargetYaw,      DeltaTime, S);
}

void UShipMovementComponent::ApplyArcadeRotation(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const float YawRate   = MaxYawRateDeg   * YawInput;
	const float PitchRate = MaxPitchRateDeg * PitchInput;
	const float RollRate  = MaxRollRateDeg  * RollInput;

	const FRotator DeltaRot(PitchRate * DeltaTime, YawRate * DeltaTime, RollRate * DeltaTime);
	Owner->AddActorLocalRotation(DeltaRot, true);
}

void UShipMovementComponent::ApplyArcadeTranslation(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const float TargetSpeed = FMath::Clamp(ThrottleInput * MaxSpeed, MinSpeed, MaxSpeed);

	const bool bBraking = (ThrottleInput < -0.2f);
	float Rate = Deceleration;

	if (TargetSpeed > CurrentSpeed) Rate = Acceleration;
	else Rate = bBraking ? BrakeDeceleration : Deceleration;

	// Correct usage: Rate is units/sec (do NOT multiply by DeltaTime)
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, TargetSpeed, DeltaTime, Rate);

	const FVector Forward = GetEffectiveForwardWS();
	FHitResult Hit;
	Owner->AddActorWorldOffset(Forward * CurrentSpeed * DeltaTime, true, &Hit);
}