#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "ShipMovementComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShipMovementComponent, Log, All);

/**
 * Handles spacecraft-style movement with smooth player input and
 * steering-based directional control that always maintains an upright orientation.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GALACTICARMADA_API UShipMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	// Constructor
	UShipMovementComponent();

protected:
	// Thrust configuration
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Thrust",
		meta=(ClampMin="0.0", ToolTip="Maximum forward speed the ship can reach at full throttle.")
	)
	float MaximumSpeed = 10000.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Thrust",
		meta=(ClampMin="0.0", ToolTip=
			"Lowest forward speed allowed when throttle is positive. Prevents full stop if desired.")
	)
	float MinimumSpeed = 3000.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Thrust",
		meta=(ClampMin="0.0", ToolTip="Rate at which the ship accelerates when increasing throttle.")
	)
	float AccelerationRate = 12000.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Thrust",
		meta=(ClampMin="0.0", ToolTip="Rate at which the ship naturally slows down when reducing throttle.")
	)
	float DecelerationRate = 9000.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Thrust",
		meta=(ClampMin="0.0", ToolTip="Stronger deceleration used when actively braking or reversing throttle.")
	)
	float BrakeDecelerationRate = 16000.0f;


	// Rotation configuration
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Rotation",
		meta=(ClampMin="0.0", ToolTip="Maximum yaw rotation speed in degrees per second.")
	)
	float MaximumYawRateDegrees = 120.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Rotation",
		meta=(ClampMin="0.0", ToolTip="Maximum pitch rotation speed in degrees per second.")
	)
	float MaximumPitchRateDegrees = 100.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Rotation",
		meta=(ClampMin="0.0", ToolTip="Maximum roll rotation speed in degrees per second.")
	)
	float MaximumRollRateDegrees = 160.0f;


	// Input smoothing
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Input",
		meta=(ClampMin="0.0", ToolTip=
			"How quickly inputs reach their target values. Higher values feel more responsive.")
	)
	float InputSmoothingSpeed = 14.0f;


	// Steering behavior
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ToolTip="Distance from the target location at which steering input is cleared.")
	)
	float ArrivalRadius = 300.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ToolTip="Angle threshold where small steering errors are ignored to prevent jitter.")
	)
	float AimDeadzoneDegrees = 0.25f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="1.0", ToolTip="Yaw error angle that results in full yaw input.")
	)
	float FullYawInputAtDegrees = 35.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="1.0", ToolTip="Pitch error angle that results in full pitch input.")
	)
	float FullPitchInputAtDegrees = 30.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ToolTip="Multiplier applied to yaw error before clamping.")
	)
	float YawResponseGain = 1.35f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ToolTip="Multiplier applied to pitch error before clamping.")
	)
	float PitchResponseGain = 1.35f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ToolTip="How much roll is added when yawing into a turn.")
	)
	float RollFromYawFactor = 0.9f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="1.0", ToolTip="Maximum allowed bank angle when steering.")
	)
	float MaximumBankAngleDegrees = 75.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ToolTip="Yaw error below which auto-leveling begins to take effect.")
	)
	float AutoLevelStartYawDegrees = 12.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ToolTip="Strength of roll correction used to keep the ship upright.")
	)
	float AutoLevelStrength = 2.2f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ClampMax="180.0", ToolTip="Angle where throttle begins to reduce to prevent orbiting.")
	)
	float ThrottleSlowdownAngleDegrees = 18.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="0.0", ClampMax="180.0", ToolTip="Angle where forward thrust is stopped or reversed.")
	)
	float ThrottleStopAngleDegrees = 70.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ClampMin="-1.0", ClampMax="1.0", ToolTip="Minimum throttle applied when significantly off target.")
	)
	float MinimumThrottleWhenOffTarget = 0.15f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Steering",
		meta=(ToolTip="If true, applies braking when the ship is facing far away from its target.")
	)
	bool bBrakeWhenSeverelyMisaligned = true;


	// Debug
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Debug",
		meta=(ToolTip="Draws debug lines for forward and desired steering direction.")
	)
	bool bDrawDebug = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Movement|Debug",
		meta=(ToolTip="Length of debug lines drawn in the world.")
	)
	float DebugDrawScale = 1500.0f;

protected:
	// Runtime state
	float CurrentSpeed = 0.0f;

	bool bHasSteeringTargetLocation = false;
	FVector SteeringTargetLocationWorldSpace = FVector::ZeroVector;
	FVector SteeringTargetDirectionWorldSpace = FVector::ZeroVector;

protected:
	// Input targets
	float PlayerThrottleTarget = 0.0f;
	float PlayerRollTarget = 0.0f;
	float PlayerPitchTarget = 0.0f;
	float PlayerYawTarget = 0.0f;

	float SteeringThrottleTarget = 0.0f;
	float SteeringRollTarget = 0.0f;
	float SteeringPitchTarget = 0.0f;
	float SteeringYawTarget = 0.0f;

	float TargetThrottle = 0.0f;
	float TargetRoll = 0.0f;
	float TargetPitch = 0.0f;
	float TargetYaw = 0.0f;

	float SmoothedThrottle = 0.0f;
	float SmoothedRoll = 0.0f;
	float SmoothedPitch = 0.0f;
	float SmoothedYaw = 0.0f;

protected:
	// Unreal overrides
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	// Player input
	UFUNCTION(BlueprintCallable, Category="Movement|Player")
	void SetThrottleInput(float InputValue);

	UFUNCTION(BlueprintCallable, Category="Movement|Player")
	void SetRollInput(float InputValue);

	UFUNCTION(BlueprintCallable, Category="Movement|Player")
	void SetPitchInput(float InputValue);

	UFUNCTION(BlueprintCallable, Category="Movement|Player")
	void SetYawInput(float InputValue);

	// Steering control
	UFUNCTION(BlueprintCallable, Category="Movement|Steering")
	void SteerTowardWorldDirection(const FVector& WorldDirection);

	UFUNCTION(BlueprintCallable, Category="Movement|Steering")
	void SteerTowardWorldLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category="Movement|Steering")
	void ClearSteering();

private:
	void ResolveUpdatedComponentReference();

	void UpdateSteeringInputs();
	void SelectInputTargets();
	void SmoothInputTargets(float DeltaTime);

	void ApplyRotation(float DeltaTime);
	void ApplyTranslation(float DeltaTime);

	FVector GetForwardVectorWorldSpace() const;

	static float SafeAtan2Degrees(float Y, float X);
	static float NormalizeAngleDegrees(float Degrees);

public:
	// Getters
	FORCEINLINE float GetMinimumSpeed() const { return MinimumSpeed; }
	FORCEINLINE float GetMaximumSpeed() const { return MaximumSpeed; }
	FORCEINLINE float GetCurrentSpeed() const { return CurrentSpeed; }
};