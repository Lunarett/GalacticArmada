#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShipPawn.generated.h"

class USkeletalMeshComponent;
class USphereComponent;
class USpringArmComponent;
class UCameraComponent;
class UShipMovementComponent;
class UCannonComponent;
class UHealthComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

USTRUCT(BlueprintType)
struct FThrusterEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thruster Effect")
	FName SocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thruster Effect")
	float MinScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thruster Effect")
	float MaxScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thruster Effect")
	UNiagaraSystem* ThrusterParticleEffect;
};

UCLASS()
class GALACTICARMADA_API AShipPawn : public APawn
{
	GENERATED_BODY()

public:
	AShipPawn();

protected:
	// ShipPawn - Components
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* ShipMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	USphereComponent* DetectionSphereCollision;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Component")
	UShipMovementComponent* ShipMovementComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cannon")
	UCannonComponent* CannonComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	UHealthComponent* HealthComponent;

	// ShipPawn - Collision Damage Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision Damage Properties")
	float MinCollisionDamage = 100.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision Damage Properties")
	float MaxCollisionDamage = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision Damage Properties")
	float CollisionCooldownDuration = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision Damage Properties")
	bool bBounceOffOnCollision = true;

	// ShipPawn - Effects
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects - Particles")
	UNiagaraSystem* ExplosionParticleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects - Particles")
	UNiagaraSystem* CollisionImpactParticleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects - Particles")
	TArray<FThrusterEffect> ThrusterEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects - Camera Shake")
	TSubclassOf<UCameraShakeBase> ImpactCameraShake;

public:
	// ShipPawn - AI Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float StoppingDistance = 20000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float ObstacleAvoidanceDistance = 30000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float MinAvoidanceStrength = 0.1f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float MaxAvoidanceStrength = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float PrimaryFireRange = 30000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float SecondaryFireRange = 60000.0f;

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
	TArray<AActor*> DetectedActors;

	UPROPERTY(BlueprintReadOnly)
	TArray<UNiagaraComponent*> ThrusterParticleEffects;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;

private:
	bool bIsCollisionCooldown;
	FTimerHandle CollisionCooldownTimerHandle;
	
	void InitializeThrusterEffects();
	void UpdateThrusterEffects();
	
	void HandleThrustInput(const FInputActionValue& Value);
	void HandleRollInput(const FInputActionValue& Value);
	void HandlePitchInput(const FInputActionValue& Value);
	void HandleYawInput(const FInputActionValue& Value);

	void BeginPrimaryFire();
	void EndPrimaryFire();
	void BeginSecondaryFire();
	void EndSecondaryFire();

	void ClearCollisionCooldown();

	UFUNCTION()
	void OnShipCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnPawnDied(AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnDetectionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	FVector GetClosestCollisionLocation() const;
	FORCEINLINE TArray<AActor*> GetDetectedActors() const { return DetectedActors; }
	FORCEINLINE UShipMovementComponent* GetShipMovementComponent() const { return ShipMovementComponent; }
	FORCEINLINE UCannonComponent* GetCannonComponent() const { return CannonComponent; }
};