#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShipPawn.generated.h"

class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UShipMovementComponent;
class UCannonComponent;
class UHealthComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS(Abstract)
class GALACTICARMADA_API AShipPawn : public APawn
{
	GENERATED_BODY()

public:
	AShipPawn();

protected:
	// ShipPawn - Components
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* ShipMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UShipMovementComponent* ShipMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCannonComponent* CannonComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;


	// ShipPawn - Effects
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects - Particles")
	UNiagaraSystem* ExplosionParticleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects - Particles")
	UNiagaraSystem* CollisionImpactParticleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects - Camera Shake")
	TSubclassOf<UCameraShakeBase> ImpactCameraShake;

private:
	bool bIsCollisionCooldown;
	FTimerHandle CollisionCooldownTimerHandle;

public:
	// Ship Movement Methods - Use these to control the movement of the ship
	void SetThrottle(const float InThrottle);
	void SetRoll(const float InRoll);
	void SetPitch(const float InPitch);
	void SetYaw(const float InYaw);

	// Ship Fire Methods - Use these methods to begin fire or end fire
	void BeginPrimaryFire();
	void BeginSecondaryFire();
	void EndPrimaryFire();
	void EndSecondaryFire();

private:
	void ClearCollisionCooldown();

	UFUNCTION()
	void OnShipCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                     FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void  OnPawnDied(AActor* DeadActor, AController* KillerController, AActor* DamageCauser);

public:
	FORCEINLINE UShipMovementComponent* GetShipMovementComponent() const { return ShipMovementComponent; }
	FORCEINLINE UCannonComponent* GetCannonComponent() const { return CannonComponent; }
};
