#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericTeamAgentInterface.h"
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

	// IMPORTANT: PostLogin can happen before HandleStartingNewPlayer in some setups,
	// and Super::PostLogin can trigger the spawn flow.
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	// Assign team before Super so the engine spawn flow sees the right team.
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	virtual void RestartPlayer(AController* NewPlayer) override;

	
protected:
	virtual void BeginMatchPhase();
	virtual void CompleteMatch(bool bSuccess);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0"))
	float MatchStartDelay = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Teams", meta=(ClampMin="0", ClampMax="254"))
	uint8 PlayerTeamId = 0;

	UPROPERTY(EditDefaultsOnly, Category="Teams", meta=(ClampMin="0", ClampMax="254"))
	uint8 TeamAId = 0;

	UPROPERTY(EditDefaultsOnly, Category="Teams", meta=(ClampMin="0", ClampMax="254"))
	uint8 TeamBId = 1;

	UPROPERTY(EditDefaultsOnly, Category="AI|Targeting", meta=(ClampMin="0"))
	int32 DefaultPlayerChaseBudgetPerTeam = 4;

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

	ATeamPlayerStart* PickRandomStart(const TArray<ATeamPlayerStart*>& Starts) const;

	TSubclassOf<AShipPawn> GetRandomAISpawnClass(uint8 TeamId) const;
	bool IsValidShipClass(const TSubclassOf<AShipPawn>& ShipClass) const;

	UShipSpawnSubsystem* GetShipSpawnSubsystem() const;

	void InitializePools();
	void CleanupSpawnedShips();
	void ConfigureAIChaseBudgets();

	void ApplyTeamToController(AController* Controller, const FGenericTeamId& TeamId) const;
	FGenericTeamId GetControllerTeamId(const AController* Controller) const;
	FGenericTeamId GetShipTeamIdFromController(const AShipPawn* Ship) const;

private:
	// Starts cache
	void CacheTeamStarts();
	void EnsureStartsCached();
	const TArray<ATeamPlayerStart*>& GetCachedPlayerStarts(uint8 TeamId) const;
	const TArray<ATeamPlayerStart*>& GetCachedAIStarts(uint8 TeamId) const;

	// Logs
	void LogCachedStarts() const;
	void LogControllerTeam(const TCHAR* Context, const AController* Controller) const;
	void LogStartChoice(const TCHAR* Context, const AController* Controller, const AActor* StartSpot) const;
	void LogSpawnResult(const TCHAR* Context, const AController* Controller, const FTransform& Requested, const APawn* SpawnedPawn) const;

	static FString TeamToString(FGenericTeamId TeamId);

private:
	FTimerHandle MatchStartTimerHandle;
	bool bHasSpawnedAI = false;

	TMap<uint8, TArray<ATeamPlayerStart*>> CachedPlayerOnlyStartsByTeam;
	TMap<uint8, TArray<ATeamPlayerStart*>> CachedAIStartsByTeam;

	static const TArray<ATeamPlayerStart*> EmptyStarts;
};