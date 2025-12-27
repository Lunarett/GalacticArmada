#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaseMatchGameMode.generated.h"

class ATeamPlayerStart;
class AShipPawn;
class UShipSpawnSubsystem;

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
	// Child game modes rely on these being present (they override them).
	virtual void BeginMatchPhase();
	virtual void CompleteMatch(bool bSuccess);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Match")
	float MatchStartDelay = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Teams")
	int32 PlayerTeamID = 0;

	UPROPERTY(EditDefaultsOnly, Category="Teams")
	int32 TeamA_ID = 0;

	UPROPERTY(EditDefaultsOnly, Category="Teams")
	int32 TeamB_ID = 1;

	// Targeting policy: how many agents per team are allowed to chase player pawns.
	UPROPERTY(EditDefaultsOnly, Category="AI|Targeting")
	int32 DefaultPlayerChaseBudgetPerTeam = 4;

	// AI ship pools per team.
	UPROPERTY(EditDefaultsOnly, Category="AI|Spawning")
	TArray<TSubclassOf<AShipPawn>> AllowedAISpawnableShips_TeamA;

	UPROPERTY(EditDefaultsOnly, Category="AI|Spawning")
	TArray<TSubclassOf<AShipPawn>> AllowedAISpawnableShips_TeamB;

protected:
	UFUNCTION()
	void StartMatchAfterDelay();

protected:
	void SpawnAllAI();
	void SpawnAIForTeam(int32 TeamID);

	bool TrySpawnAIShipAtStart(ATeamPlayerStart* PlayerStart, int32 TeamID);
	void AssignAIPlayerStateTeam(AShipPawn* SpawnedShip, int32 TeamID);

	TArray<ATeamPlayerStart*> FindAllPlayerStartsForTeam(int32 TeamID) const;

	TSubclassOf<AShipPawn> GetRandomAISpawnClass(int32 TeamID) const;
	bool IsValidAISpawnClass(const TSubclassOf<AShipPawn>& ShipClass) const;

	UShipSpawnSubsystem* GetShipSpawnSubsystem() const;

	void InitializePools();
	void CleanupSpawnedShips();

	void ConfigureAIChaseBudgets();

private:
	FTimerHandle MatchStartTimerHandle;

	UPROPERTY()
	bool bHasSpawnedAI = false;
};