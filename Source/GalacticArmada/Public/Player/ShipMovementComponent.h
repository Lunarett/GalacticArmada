#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/MovementComponent.h"
#include "ShipMovementComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShipMovementComponent, Log, All);

UCLASS()
class GALACTICARMADA_API UShipMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:	
	UShipMovementComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Thrust Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float MaxSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float MinSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float Acceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float Deceleration;


	// Roll Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float FlapAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float FlapSpeed;
	

	// Pitch Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float ElevatorAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float ElevatorSpeed;


	// Yaw Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float RudderAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Properties")
	float RudderSpeed;

private:
	float CurrentThrustInput;
	float CurrentRollInput;
	float CurrentPitchInput;
	float CurrentYawInput;

	float CurrentSpeed;
	float CurrentRoll;
	float CurrentPitch;
	float CurrentYaw;
	
public:
	void SetYawInput(float InputValue);
	void SetPitchInput(float InputValue);
	void SetRollInput(float InputValue);
	void SetThrustInput(float InputValue);

private:
	void UpdateRotationMovement(float DeltaSeconds, float& CurrentValue, float InputValue, float MaxAngle, float Speed, FRotator RotationAxis);
	void UpdateThrustMovement(float DeltaSeconds);

public:
	FORCEINLINE float GetShipMinSpeed() const { return MinSpeed; }
	FORCEINLINE float GetShipMaxSpeed() const { return MaxSpeed; }
	FORCEINLINE float GetShipCurrentSpeed() const { return CurrentSpeed; }
	FORCEINLINE float GetShipCurrentRoll() const { return CurrentYaw; }
	FORCEINLINE float GetShipCurrentPitch() const { return CurrentPitch; }
	FORCEINLINE float GetShipCurrentYaw() const { return CurrentYaw; }
};
