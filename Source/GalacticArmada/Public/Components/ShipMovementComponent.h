#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipMovementComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GALACTICARMADA_API UShipMovementComponent : public UActorComponent
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
	
public:
	void SetYawInput(float InputValue);
	void SetPitchInput(float InputValue);
	void SetRollInput(float InputValue);
	void SetThrustInput(float InputValue);

private:
	float CurrentThrustInput;
	float CurrentRollInput;
	float CurrentPitchInput;
	float CurrentYawInput;

	float CurrentSpeed;
	float CurrentRoll;
	float CurrentPitch;
	float CurrentYaw;

	void UpdateRotationMovement(float DeltaSeconds, float& CurrentValue, float InputValue, float MaxAngle, float Speed, FRotator RotationAxis);
	void UpdateThrustMovement(float DeltaSeconds);

public:
	FORCEINLINE float GetMinSpeed() const { return MinSpeed; }
	FORCEINLINE float GetMaxSpeed() const { return MaxSpeed; }
	FORCEINLINE float GetCurrentSpeed() const { return CurrentSpeed; }
	FORCEINLINE float GetCurrentRoll() const { return CurrentYaw; }
	FORCEINLINE float GetCurrentPitch() const { return CurrentPitch; }
	FORCEINLINE float GetCurrentYaw() const { return CurrentYaw; }
};
