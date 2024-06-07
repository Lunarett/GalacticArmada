#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "ShipPawn.generated.h"

class USphereComponent;
class USpringArmComponent;
class UCameraComponent;
class UCannonComponent;
class UHealthComponent;
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UChaosVehicleMovementComponent;

UCLASS()
class GALACTICARMADA_API AShipPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	AShipPawn();
	
protected:
	// ShipPawn - Components
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	USphereComponent* DetectionSphereCollision;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cannon")
	UCannonComponent* Cannon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	UHealthComponent* Health;
	

	// ShipPawn - Effects
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UNiagaraSystem* ExplosionParticleEffect;


public:
	// ShipPawn - AI Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float StoppingDistance = 20000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float ObstacleAvoidanceDistance = 30000.0f;

	
protected:
	// ShipPawn - Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* ShipMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ThrustInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* RollInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* PitchInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* YawInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* PrimaryFireInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SecondaryFireInputAction;

	UPROPERTY(BlueprintReadOnly)
	UChaosVehicleMovementComponent* ChaosVehicleComp;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<AActor*> DetectedActors;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;

private:
	void HandleThrustInput(const FInputActionValue& Value);
	void HandleRollInput(const FInputActionValue& Value);
	void HandlePitchInput(const FInputActionValue& Value);
	void HandleYawInput(const FInputActionValue& Value);

	void BeginPrimaryFire();
	void EndPrimaryFire();
	void BeginSecondaryFire();
	void EndSecondaryFire();

	UFUNCTION()
	void OnPawnDied(AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	AActor* GetNearestActor();
	FVector GetClosestCollisionLocation() const;
	FORCEINLINE TArray<AActor*> GetDetectedActors() const { return DetectedActors; }
};
