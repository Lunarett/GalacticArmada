#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "ShipPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class AShipPawn;
class UAICommandSubsystem;

/**
 * Ship player controller.
 *
 * Responsibilities:
 * - Enhanced Input mapping + bindings
 * - Forward input to the currently possessed ship pawn
 * - Owns the game's team identity (GenericTeamId) for this player
 * - Registers/unregisters the possessed pawn with UAICommandSubsystem using that team id
 *
 * Notes:
 * - This project is single-player: no replication code.
 * - Only the controller knows the team. Pawn/PlayerState do not implement team logic.
 */
UCLASS(Abstract)
class GALACTICARMADA_API AShipPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AShipPlayerController();

	//~ Begin IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~ End IGenericTeamAgentInterface

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputMappingContext* ShipInputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Actions")
	UInputAction* ThrottleInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Actions")
	UInputAction* RollInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Actions")
	UInputAction* PitchInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Actions")
	UInputAction* YawInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Actions")
	UInputAction* PrimaryFireInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Actions")
	UInputAction* SecondaryFireInputAction = nullptr;

private:
	/** Currently possessed ship pawn (possession can change). */
	UPROPERTY(Transient)
	TObjectPtr<AShipPawn> ShipPawn = nullptr;

	/** Cached subsystem reference (resolved once; lazy init). */
	UPROPERTY(Transient)
	TObjectPtr<UAICommandSubsystem> AICommandSubsystem = nullptr;

	/** Controller-owned team id (0..254 valid; 255 = NoTeam). */
	FGenericTeamId TeamId = FGenericTeamId::NoTeam;

private:
	/** Ensures AICommandSubsystem is cached (lazy, runs at most once successfully). */
	void EnsureSubsystemsCached();

	void PropagateTeamToPawn();

	/** Adds the input mapping context to the local Enhanced Input subsystem. */
	void InitializeInputMapping();

	/** Registers the currently possessed pawn as an agent using our team id. */
	void RegisterPawnAsAgent();

	/** Unregisters the currently possessed pawn as an agent. */
	void UnregisterPawnAsAgent();

	/** Tries to resolve a team id from an actor by looking for a controller team agent. */
	FGenericTeamId ResolveTeamFromActor(const AActor& Actor) const;

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