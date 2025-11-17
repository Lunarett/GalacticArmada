#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputSubsystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "ShipPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AShipPawn;

UCLASS(Abstract)
class GALACTICARMADA_API AShipPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShipPlayerController();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* ShipInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	UInputAction* ThrottleInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	UInputAction* RollInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	UInputAction* PitchInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	UInputAction* YawInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	UInputAction* PrimaryFireInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	UInputAction* SecondaryFireInputAction;

private:
	UPROPERTY()
	AShipPawn* ShipPawn;

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void HandleThrottleInput(const FInputActionValue& Value);
	void HandleRollInput(const FInputActionValue& Value);
	void HandlePitchInput(const FInputActionValue& Value);
	void HandleYawInput(const FInputActionValue& Value);

	void HandleBeginPrimaryFireInput();
	void HandleEndPrimaryFireInput();
	void HandleBeginSecondaryFireInput();
	void HandleEndSecondaryFireInput();
};
