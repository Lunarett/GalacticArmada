#include "Gameplay/BaseMatchGameMode.h"
#include "Gameplay/GalacticGameInstance.h"
#include "Gameplay/TeamPlayerStart.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerController.h"
#include "Player/ShipPlayerState.h"
#include "Kismet/GameplayStatics.h"

ABaseMatchGameMode::ABaseMatchGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AShipPlayerController::StaticClass();
	PlayerStateClass = AShipPlayerState::StaticClass();
}

void ABaseMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		MatchStartTimerHandle,
		this,
		&ABaseMatchGameMode::StartMatchAfterDelay,
		MatchStartDelay,
		false
	);
}

void ABaseMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer != nullptr)
	{
		AShipPlayerState* PlayerState = Cast<AShipPlayerState>(NewPlayer->PlayerState);
		if (PlayerState != nullptr)
		{
			PlayerState->SetTeamID(PlayerTeamID);
		}

		RestartPlayer(NewPlayer);

		if (!bHasSpawnedAI)
		{
			SpawnAllAI();
			bHasSpawnedAI = true;
		}
	}
}

void ABaseMatchGameMode::StartMatchAfterDelay()
{
	if (!bHasSpawnedAI)
	{
		SpawnAllAI();
		bHasSpawnedAI = true;
	}

	BeginMatchPhase();
}

void ABaseMatchGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Invalid Player Controller"));
		return;
	}

	AActor* StartSpot = ChoosePlayerStart(NewPlayer);
	if (StartSpot == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Could not find valid Player Start"));
		return;
	}

	APawn* NewPawn = SpawnDefaultPawnFor(NewPlayer, StartSpot);
	if (NewPawn == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Failed to spawn pawn"));
		return;
	}

	NewPlayer->Possess(NewPawn);
}

AActor* ABaseMatchGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (Player == nullptr)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	TArray<ATeamPlayerStart*> Starts = FindAllPlayerStartsForTeam(PlayerTeamID);

	for (ATeamPlayerStart* Start : Starts)
	{
		if (Start != nullptr && Start->GetIsPlayerOnly())
		{
			return Start;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("ChoosePlayerStart: No valid player-only start found. Falling back to random start"));

	if (Starts.Num() > 0)
	{
		return Starts[FMath::RandRange(0, Starts.Num() - 1)];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

APawn* ABaseMatchGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (NewPlayer == nullptr || StartSpot == nullptr)
	{
		return nullptr;
	}

	UGalacticGameInstance* GameInstance = GetGameInstance<UGalacticGameInstance>();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	TSubclassOf<AShipPawn> ShipClass = GameInstance->GetDefaultPlayerShipClass();
	if (!IsValidAISpawnClass(ShipClass))
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnDefaultPawnFor: Invalid player ship class"));
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = NewPlayer;
	Params.Instigator = NewPlayer->GetPawn();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return GetWorld()->SpawnActor<APawn>(ShipClass, StartSpot->GetActorTransform(), Params);
}

void ABaseMatchGameMode::SpawnAllAI()
{
	SpawnAIForTeam(TeamA_ID);
	SpawnAIForTeam(TeamB_ID);
}

void ABaseMatchGameMode::SpawnAIForTeam(const int32 TeamID)
{
	TArray<ATeamPlayerStart*> Starts = FindAllPlayerStartsForTeam(TeamID);

	for (ATeamPlayerStart* Start : Starts)
	{
		if (Start != nullptr && !Start->GetIsPlayerOnly())
		{
			const bool bSpawned = TrySpawnAIShipAtStart(Start, TeamID);
			if (!bSpawned)
			{
				UE_LOG(LogTemp, Warning, TEXT("SpawnAIForTeam: Failed to spawn AI at start location for Team %d"), TeamID);
			}
		}
	}
}

bool ABaseMatchGameMode::TrySpawnAIShipAtStart(ATeamPlayerStart* PlayerStart, const int32 TeamID)
{
	if (PlayerStart == nullptr)
	{
		return false;
	}

	TSubclassOf<AShipPawn> ShipClass = GetRandomAISpawnClass(TeamID);
	if (!IsValidAISpawnClass(ShipClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("TrySpawnAIShipAtStart: Invalid ship class for team %d"), TeamID);
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AShipPawn* SpawnedShip = GetWorld()->SpawnActor<AShipPawn>(ShipClass, PlayerStart->GetActorTransform(), Params);
	if (SpawnedShip == nullptr)
	{
		return false;
	}

	AssignAIPlayerStateTeam(SpawnedShip, TeamID);
	return true;
}

void ABaseMatchGameMode::AssignAIPlayerStateTeam(AShipPawn* SpawnedShip, const int32 TeamID)
{
	if (SpawnedShip == nullptr)
	{
		return;
	}

	AShipPlayerState* ShipState = SpawnedShip->GetPlayerState<AShipPlayerState>();
	if (ShipState != nullptr)
	{
		ShipState->SetTeamID(TeamID);
	}
}

TArray<ATeamPlayerStart*> ABaseMatchGameMode::FindAllPlayerStartsForTeam(const int32 TeamID) const
{
	TArray<AActor*> AllStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATeamPlayerStart::StaticClass(), AllStarts);

	TArray<ATeamPlayerStart*> FilteredStarts;
	for (AActor* Actor : AllStarts)
	{
		ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Actor);
		if (TeamStart != nullptr && TeamStart->GetTeamID() == TeamID)
		{
			FilteredStarts.Add(TeamStart);
		}
	}

	return FilteredStarts;
}

TSubclassOf<AShipPawn> ABaseMatchGameMode::GetRandomAISpawnClass(const int32 TeamID) const
{
	const TArray<TSubclassOf<AShipPawn>>* ShipPool = nullptr;

	if (TeamID == TeamA_ID)
	{
		ShipPool = &AllowedAISpawnableShips_TeamA;
	}
	else if (TeamID == TeamB_ID)
	{
		ShipPool = &AllowedAISpawnableShips_TeamB;
	}

	if (ShipPool != nullptr && ShipPool->Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, ShipPool->Num() - 1);
		return (*ShipPool)[RandomIndex];
	}

	return nullptr;
}

bool ABaseMatchGameMode::IsValidAISpawnClass(const TSubclassOf<AShipPawn>& ShipClass) const
{
	return ShipClass != nullptr && ShipClass->IsChildOf(AShipPawn::StaticClass());
}

int32 ABaseMatchGameMode::GetPlayerTeamID() const
{
	return PlayerTeamID;
}

int32 ABaseMatchGameMode::GetOpposingTeamID(const int32 TeamID) const
{
	if (TeamID == TeamA_ID)
	{
		return TeamB_ID;
	}

	return TeamA_ID;
}