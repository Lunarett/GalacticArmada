#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericTeamAgentInterface.h"
#include "BaseMatchGameMode.generated.h"

class ATeamPlayerStart;
class AShipPawn;
class UShipSpawnSubsystem;

/**
 * Base match game mode.
 *
 * Responsibilities:
 * - Assign a Generic Team ID to the player controller (controller owns team).
 * - Choose a team-appropriate PlayerStart.
 *   - Prefer "player-only" starts for the player's team (random among them).
 *   - If none are player-only, pick randomly from all starts for that team.
 * - Spawn AI for two active teams at non-player-only starts.
 * - Configure AI chase budgets based on team sizes.
 *
 * Team System Rule (by project decision):
 * - Only controllers know their team (IGenericTeamAgentInterface).
 * - Pawns and PlayerStates do not implement or store team identity.
 */
UCLASS()
class GALACTICARMADA_API ABaseMatchGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABaseMatchGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

protected:
	/** Child game modes implement match rules. */
	virtual void BeginMatchPhase();

	/** Child game modes implement match rules. */
	virtual void CompleteMatch(bool bSuccess);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0"))
	float MatchStartDelay = 0.0f;

	/**
	 * Team assigned to the player controller.
	 * 0..254 are valid team ids; 255 is reserved for NoTeam.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Teams", meta=(ClampMin="0", ClampMax="254"))
	uint8 PlayerTeamId = 0;

	/**
	 * Two active AI teams in this match.
	 * Designers place ATeamPlayerStart instances for these teams in the level.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Teams", meta=(ClampMin="0", ClampMax="254"))
	uint8 TeamAId = 0;

	UPROPERTY(EditDefaultsOnly, Category="Teams", meta=(ClampMin="0", ClampMax="254"))
	uint8 TeamBId = 1;

	/** Targeting policy: how many agents per team are allowed to chase player pawns. */
	UPROPERTY(EditDefaultsOnly, Category="AI|Targeting", meta=(ClampMin="0"))
	int32 DefaultPlayerChaseBudgetPerTeam = 4;

	/** AI ship pools per team. */
	UPROPERTY(EditDefaultsOnly, Category="AI|Spawning")
	TArray<TSubclassOf<AShipPawn>> AllowedAISpawnableShips_TeamA;

	UPROPERTY(EditDefaultsOnly, Category="AI|Spawning")
	TArray<TSubclassOf<AShipPawn>> AllowedAISpawnableShips_TeamB;

protected:
	UFUNCTION()
	void StartMatchAfterDelay();

protected:
	void SpawnAllAI();
	void SpawnAIForTeam(uint8 TeamId);

	bool TrySpawnAIShipAtStart(const ATeamPlayerStart* PlayerStart, uint8 TeamId);

	TArray<ATeamPlayerStart*> FindAllPlayerStartsForTeam(uint8 TeamId) const;
	ATeamPlayerStart* PickRandomStart(const TArray<ATeamPlayerStart*>& Starts) const;

	TSubclassOf<AShipPawn> GetRandomAISpawnClass(uint8 TeamId) const;
	bool IsValidShipClass(const TSubclassOf<AShipPawn>& ShipClass) const;

	UShipSpawnSubsystem* GetShipSpawnSubsystem() const;

	void InitializePools();
	void CleanupSpawnedShips();

	void ConfigureAIChaseBudgets();

	/** Assigns a GenericTeamId to a controller if it implements IGenericTeamAgentInterface. */
	void ApplyTeamToController(AController* Controller, const FGenericTeamId& TeamId) const;

	/** Reads a controller team id; falls back to PlayerTeamId if not available. */
	FGenericTeamId GetControllerTeamId(const AController* Controller) const;

	/** Reads a ship's team from its controller (only controller is authoritative). */
	FGenericTeamId GetShipTeamIdFromController(const AShipPawn* Ship) const;

private:
	FTimerHandle MatchStartTimerHandle;
	bool bHasSpawnedAI = false;
};