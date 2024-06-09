#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShipAIController.generated.h"

class AShipPawn;

UCLASS()
class GALACTICARMADA_API AShipAIController : public AAIController
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	AShipPawn* ControlledShipPawn;

	UPROPERTY(BlueprintReadOnly)
	AShipPawn* TargetShipPawn;

	UPROPERTY(BlueprintReadOnly)
	FRotator TargetRotation;

	UPROPERTY(EditDefaultsOnly, Category = "AI Debug")
	bool bEnableAvoidanceDebug = true;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	FTimerHandle CollisionAvoidanceTimerHandle;
	
	FRotator GetTargetShipRotation() const;
	void UpdateMovement(float DeltaSeconds) const;
	void UpdateCollisionAvoidance();
	void UpdateFiring() const;
	static float RotationToInputAxis(float Value, float RotVal);
	bool IsInRange() const;
};
