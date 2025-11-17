#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Turret.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UNiagaraSystem;
class UHealthComponent;
class AProjectileBase;

UCLASS()
class GALACTICARMADA_API ATurret : public AActor
{
	GENERATED_BODY()

public:
	ATurret();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* YawMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* PitchMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* FirePoint;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* TargetDetectionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

protected:
	UPROPERTY(EditAnywhere, Category = "Firing")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Firing")
	float FireRate = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Firing")
	float FireAlignmentTolerance = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Firing")
	float FireDamage = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Firing")
	UNiagaraSystem* MuzzleEffect;

	UPROPERTY(EditAnywhere, Category = "Rotation")
	float YawRotationOffset = 90.0f;
	
	UPROPERTY(EditAnywhere, Category = "Rotation")
	float YawSpeed = 90.0f;

	UPROPERTY(EditAnywhere, Category = "Rotation")
	float PitchSpeed = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Rotation")
	float MinPitch = -10.0f;

	UPROPERTY(EditAnywhere, Category = "Rotation")
	float MaxPitch = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float TargetingRange = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	int32 TeamID = -1;

	UPROPERTY(EditAnywhere, Category = "Effects")
	UNiagaraSystem* DeathEffect;

private:
	FTimerHandle FireTimerHandle;

	UPROPERTY()
	AActor* CurrentTarget;

private:
	void SearchForTarget();
	void RotateTowardsTarget(float DeltaTime);
	bool IsTargetInLineOfFire() const;
	void Fire();
	float CalculateShortestYaw(float Current, float Target) const;
	float ClampPitch(float DesiredPitch) const;
	bool IsEnemy(const AActor* OtherActor) const;

	UFUNCTION()
	void HandleDeath(AActor* DeadActor, AController* InstigatedBy, AActor* DamageCauser);
};