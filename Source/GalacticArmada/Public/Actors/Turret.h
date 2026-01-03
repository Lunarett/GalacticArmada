// Turret.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "Turret.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UNiagaraSystem;
class UHealthComponent;
class AProjectileBase;

UCLASS()
class GALACTICARMADA_API ATurret
	: public AActor
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ATurret();

	// --------------------------
	// IGenericTeamAgentInterface
	// --------------------------
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override { TeamId = NewTeamId; }
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, Category="Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category="Components")
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, Category="Components")
	UStaticMeshComponent* YawMesh;

	UPROPERTY(VisibleAnywhere, Category="Components")
	UStaticMeshComponent* PitchMesh;

	UPROPERTY(VisibleAnywhere, Category="Components")
	USceneComponent* FirePoint;

	UPROPERTY(VisibleAnywhere, Category="Components")
	USphereComponent* TargetDetectionSphere;

	UPROPERTY(VisibleAnywhere, Category="Components")
	UHealthComponent* HealthComponent;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Firing")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category="Firing")
	float FireRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Firing")
	float FireAlignmentTolerance = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category="Rotation")
	float YawRotationOffset = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category="Rotation")
	float YawSpeed = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category="Rotation")
	float PitchSpeed = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category="Rotation")
	float MinPitch = -10.0f;

	UPROPERTY(EditDefaultsOnly, Category="Rotation")
	float MaxPitch = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category="Targeting")
	float TargetingRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category="Effects")
	UNiagaraSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, Category="Effects")
	UNiagaraSystem* DeathEffect;

private:
	// Team owned by the turret
	FGenericTeamId TeamId = FGenericTeamId::NoTeam;

	FTimerHandle FireTimerHandle;

	// Cached enemy set (small, maintained by overlaps)
	UPROPERTY()
	TSet<TObjectPtr<AActor>> EnemyTargets;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

private:
	void SelectBestTarget();
	void RotateTowardsTarget(float DeltaTime);
	bool IsTargetInLineOfFire() const;
	void Fire();

	float ClampPitch(float DesiredPitch) const;

	bool IsEnemy(const AActor* OtherActor) const;

	// --------------------------
	// Overlap handlers
	// --------------------------
	UFUNCTION()
	void OnTargetEnter(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnTargetExit(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	UFUNCTION()
	void HandleDeath(AActor* DeadActor, AController* Killer, AActor* DamageCauser);
};