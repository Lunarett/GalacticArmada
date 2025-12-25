#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BaseMatchGameMode.generated.h"

class AShipPawn;
class ATeamPlayerStart;
class UShipSpawnSubsystem;

UCLASS(Abstract)
class GALACTICARMADA_API ABaseMatchGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABaseMatchGameMode();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties|Team")
	int32 TeamA_ID = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties|Team")
	int32 TeamB_ID = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties|Team")
	int32 PlayerTeamID = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties|Spawning")
	TArray<TSubclassOf<AShipPawn>> AllowedAISpawnableShips_TeamA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties|Spawning")
	TArray<TSubclassOf<AShipPawn>> AllowedAISpawnableShips_TeamB;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties")
	float MatchStartDelay = 5.0f;

private:
	FTimerHandle MatchStartTimerHandle;
	bool bHasSpawnedAI = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	virtual void BeginMatchPhase() PURE_VIRTUAL(ABaseMatchGameMode::BeginMatchPhase, );
	virtual void CompleteMatch(const bool bIsMatchSuccess) PURE_VIRTUAL(ABaseMatchGameMode::CompleteMatch, );

public:
	int32 GetPlayerTeamID() const;
	int32 GetOpposingTeamID(const int32 TeamID) const;

private:
	void StartMatchAfterDelay();
	void SpawnAIForTeam(const int32 TeamID);
	void SpawnAllAI();

	void InitializePools();

	TArray<ATeamPlayerStart*> FindAllPlayerStartsForTeam(const int32 TeamID) const;
	TSubclassOf<AShipPawn> GetRandomAISpawnClass(const int32 TeamID) const;

	bool TrySpawnAIShipAtStart(class ATeamPlayerStart* PlayerStart, const int32 TeamID);
	void AssignAIPlayerStateTeam(class AShipPawn* SpawnedShip, const int32 TeamID);

	bool IsValidAISpawnClass(const TSubclassOf<AShipPawn>& ShipClass) const;

	UShipSpawnSubsystem* GetShipSpawnSubsystem() const;

	void CleanupSpawnedShips();
};