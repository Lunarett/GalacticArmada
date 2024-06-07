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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	FRotator GetTargetShipRotation() const;
	void UpdateMovement(float DeltaSeconds);
	void UpdateCollisionAvoidance();
	static float RotationToInputAxis(float Value, float RotVal);
	bool IsInRange() const;
};