#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "ShipMovementComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShipMovementComponent, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GALACTICARMADA_API UShipMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	UShipMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	// Player control (ignored when AI enabled)
	UFUNCTION(BlueprintCallable, Category="Ship Movement|Player")
	void SetThrottleInput(float InputValue);

	UFUNCTION(BlueprintCallable, Category="Ship Movement|Player")
	void SetRollInput(float InputValue);

	UFUNCTION(BlueprintCallable, Category="Ship Movement|Player")
	void SetPitchInput(float InputValue);

	UFUNCTION(BlueprintCallable, Category="Ship Movement|Player")
	void SetYawInput(float InputValue);

	// AI control
	UFUNCTION(BlueprintCallable, Category="Ship Movement|AI")
	void SetAIEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Ship Movement|AI")
	void SteerTowardDirection(const FVector& WorldDirection);

	UFUNCTION(BlueprintCallable, Category="Ship Movement|AI")
	void SteerTowardLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category="Ship Movement|AI")
	void StopSteering();

public:
	FORCEINLINE float GetShipMinSpeed() const { return MinSpeed; }
	FORCEINLINE float GetShipMaxSpeed() const { return MaxSpeed; }
	FORCEINLINE float GetShipCurrentSpeed() const { return CurrentSpeed; }
	FORCEINLINE bool  IsAIEnabled() const { return bAIEnabled; }

protected:
	// -------------------------
	// Setup / axis fixes
	// -------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Setup")
	bool bInvertForward = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Setup")
	bool bInvertRoll = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Setup")
	bool bInvertPitch = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Setup")
	bool bInvertYaw = false;

	// If true, auto-level will NEVER allow "upside down level".
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Setup")
	bool bForceUprightLeveling = true;

	// -------------------------
	// Translation (arcade)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Thrust", meta=(ClampMin="0.0"))
	float MaxSpeed = 10000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Thrust", meta=(ClampMin="0.0"))
	float MinSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Thrust", meta=(ClampMin="0.0"))
	float Acceleration = 12000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Thrust", meta=(ClampMin="0.0"))
	float Deceleration = 9000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Thrust", meta=(ClampMin="0.0"))
	float BrakeDeceleration = 16000.0f;

	// -------------------------
	// Rotation (arcade)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Rotation", meta=(ClampMin="0.0"))
	float MaxYawRateDeg = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Rotation", meta=(ClampMin="0.0"))
	float MaxPitchRateDeg = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Rotation", meta=(ClampMin="0.0"))
	float MaxRollRateDeg = 160.0f;

	// -------------------------
	// Input smoothing
	// -------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Input", meta=(ClampMin="0.0"))
	float InputSmoothing = 14.0f;

	// -------------------------
	// AI steering
	// -------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI")
	bool bAIEnabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float AIThrottle = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0"))
	float ArrivalRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0"))
	float AimDeadzoneDeg = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="1.0"))
	float YawFullInputAtDeg = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="1.0"))
	float PitchFullInputAtDeg = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0"))
	float YawGain = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0"))
	float PitchGain = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0"))
	float RollFromYaw = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="1.0"))
	float BankMaxDeg = 75.0f;

	// Auto-level blend condition
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0"))
	float AutoLevelStartDeg = 12.0f;

	// Strength of leveling command
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0"))
	float AutoLevelGain = 2.2f;

	// Throttle alignment assist (prevents orbiting)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0", ClampMax="180.0"))
	float SlowdownStartAngleDeg = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="0.0", ClampMax="180.0"))
	float StopThrustAngleDeg = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float MinThrottleWhenOffTarget = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|AI")
	bool bBrakeWhenVeryOffTarget = true;

	// -------------------------
	// Debug
	// -------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Debug")
	bool bDebugDraw = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Debug")
	float DebugDrawScale = 1500.0f;

private:
	// Player targets
	float PlayerThrottleTarget = 0.0f;
	float PlayerRollTarget = 0.0f;
	float PlayerPitchTarget = 0.0f;
	float PlayerYawTarget = 0.0f;

	// AI targets
	float AIThrottleTarget = 0.0f;
	float AIRollTarget = 0.0f;
	float AIPitchTarget = 0.0f;
	float AIYawTarget = 0.0f;

	// Selected targets
	float TargetThrottle = 0.0f;
	float TargetRoll = 0.0f;
	float TargetPitch = 0.0f;
	float TargetYaw = 0.0f;

	// Smoothed inputs
	float ThrottleInput = 0.0f;
	float RollInput = 0.0f;
	float PitchInput = 0.0f;
	float YawInput = 0.0f;

	// State
	float CurrentSpeed = 0.0f;

	// AI target
	bool bAIHasTargetLocation = false;
	FVector AITargetLocationWS = FVector::ZeroVector;
	FVector AIDesiredDirectionWS = FVector::ZeroVector;

private:
	void ResolveUpdatedComponent();

	void UpdateAIInputs();
	void SelectTargets();
	void SmoothInputs(float DeltaTime);

	void ApplyArcadeRotation(float DeltaTime);
	void ApplyArcadeTranslation(float DeltaTime);

	FVector GetEffectiveForwardWS() const;

	static float SafeAtan2Deg(float Y, float X);
	static float Wrap180(float Deg);
};