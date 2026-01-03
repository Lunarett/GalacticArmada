// AIShipController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GenericTeamAgentInterface.h"
#include "UObject/WeakObjectPtr.h"
#include "AIShipController.generated.h"

class UBehaviorTree;
class UBlackboardData;
class USphereComponent;
class UAITargetSelectionSubsystem;

UCLASS()
class GALACTICARMADA_API AAIShipController : public AAIController
{
	GENERATED_BODY()

public:
	AAIShipController();

	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	const TArray<TWeakObjectPtr<APawn>>& GetAvoidanceNeighbors() const { return AvoidanceNeighbors; }
	float GetAvoidanceSphereRadius() const { return AvoidanceSphereRadius; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBlackboardData> BlackboardAsset = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="AI|Avoidance", meta=(ClampMin="0.0"))
	float AvoidanceSphereRadius = 2500.0f;

	UPROPERTY(EditDefaultsOnly, Category="AI|Avoidance")
	FName AvoidanceSphereAttachSocket = NAME_None;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAITargetSelectionSubsystem> TargetSelectionSubsystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APawn> ControlledPawn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> AvoidanceSphere = nullptr;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<APawn>> AvoidanceNeighbors;

	FGenericTeamId TeamId = FGenericTeamId::NoTeam;
	bool bStartedBehavior = false;

private:
	void EnsureSubsystemCached();

	void CreateOrAttachAvoidanceSphere();
	void DestroyAvoidanceSphere();
	void ClearAvoidanceNeighbors();

	void RegisterPawnAsAgent();
	void UnregisterPawnAsAgent();

	FGenericTeamId ResolveTeamFromActor(const AActor& Actor) const;

	void StartBrainIfNeeded();
	void StopBrain();

private:
	UFUNCTION()
	void OnAvoidanceBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnAvoidanceEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);
};