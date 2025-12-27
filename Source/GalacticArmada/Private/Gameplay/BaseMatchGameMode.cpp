#include "Gameplay/BaseMatchGameMode.h"

#include "Gameplay/GalacticGameInstance.h"
#include "Gameplay/TeamPlayerStart.h"
#include "Optimization/ActorPool/ActorPoolSubsystem.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerController.h"
#include "Player/ShipPlayerState.h"
#include "Systems/ShipSpawnSubsystem.h"
#include "Systems/AICommandSubsystem.h"

#include "Components/HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

ABaseMatchGameMode::ABaseMatchGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AShipPlayerController::StaticClass();
	PlayerStateClass = AShipPlayerState::StaticClass();
}

void ABaseMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	InitializePools();

	GetWorldTimerManager().SetTimer(
		MatchStartTimerHandle,
		this,
		&ABaseMatchGameMode::StartMatchAfterDelay,
		MatchStartDelay,
		false
	);
}

void ABaseMatchGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupSpawnedShips();
	Super::EndPlay(EndPlayReason);
}

void ABaseMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	AShipPlayerState* PS = Cast<AShipPlayerState>(NewPlayer->PlayerState);
	if (PS)
	{
		PS->SetTeamID(PlayerTeamID);
	}

	RestartPlayer(NewPlayer);

	if (!bHasSpawnedAI)
	{
		SpawnAllAI();
		bHasSpawnedAI = true;
	}

	ConfigureAIChaseBudgets();
}

void ABaseMatchGameMode::StartMatchAfterDelay()
{
	if (!bHasSpawnedAI)
	{
		SpawnAllAI();
		bHasSpawnedAI = true;
	}

	ConfigureAIChaseBudgets();
	BeginMatchPhase();
}

void ABaseMatchGameMode::BeginMatchPhase()
{
	// Intentionally empty: child game modes implement match rules.
}

void ABaseMatchGameMode::CompleteMatch(const bool bSuccess)
{
	(void)bSuccess;
	// Intentionally empty: child game modes implement match rules.
}

void ABaseMatchGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Invalid Player Controller"));
		return;
	}

	AActor* StartSpot = ChoosePlayerStart(NewPlayer);
	if (!StartSpot)
	{
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Could not find valid Player Start"));
		return;
	}

	APawn* NewPawn = SpawnDefaultPawnFor(NewPlayer, StartSpot);
	if (!NewPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Failed to spawn pawn"));
		return;
	}

	NewPlayer->Possess(NewPawn);
}

AActor* ABaseMatchGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	TArray<ATeamPlayerStart*> Starts = FindAllPlayerStartsForTeam(PlayerTeamID);

	for (ATeamPlayerStart* Start : Starts)
	{
		if (!Start)
		{
			continue;
		}

		if (!Start->GetIsPlayerOnly())
		{
			continue;
		}

		return Start;
	}

	if (Starts.Num() > 0)
	{
		const int32 Index = FMath::RandRange(0, Starts.Num() - 1);
		return Starts[Index];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

APawn* ABaseMatchGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (!NewPlayer)
	{
		return nullptr;
	}

	if (!StartSpot)
	{
		return nullptr;
	}

	UGalacticGameInstance* GameInstance = GetGameInstance<UGalacticGameInstance>();
	if (!GameInstance)
	{
		return nullptr;
	}

	const TSubclassOf<AShipPawn> ShipClass = GameInstance->GetDefaultPlayerShipClass();
	const bool bValidShipClass = IsValidAISpawnClass(ShipClass);

	if (!bValidShipClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnDefaultPawnFor: Invalid player ship class"));
		return nullptr;
	}

	UShipSpawnSubsystem* ShipSpawner = GetShipSpawnSubsystem();
	if (!ShipSpawner)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnDefaultPawnFor: Missing ShipSpawnSubsystem"));
		return nullptr;
	}

	AShipPawn* SpawnedShip = ShipSpawner->SpawnShip(ShipClass, StartSpot->GetActorTransform(), PlayerTeamID, false);
	if (!SpawnedShip)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnDefaultPawnFor: Failed to spawn player ship"));
		return nullptr;
	}

	SpawnedShip->SetOwner(NewPlayer);
	return SpawnedShip;
}

void ABaseMatchGameMode::SpawnAllAI()
{
	SpawnAIForTeam(TeamA_ID);
	SpawnAIForTeam(TeamB_ID);
}

void ABaseMatchGameMode::SpawnAIForTeam(const int32 TeamID)
{
	const TArray<ATeamPlayerStart*> Starts = FindAllPlayerStartsForTeam(TeamID);

	for (ATeamPlayerStart* Start : Starts)
	{
		if (!Start)
		{
			continue;
		}

		if (Start->GetIsPlayerOnly())
		{
			continue;
		}

		const bool bSpawned = TrySpawnAIShipAtStart(Start, TeamID);
		if (!bSpawned)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnAIForTeam: Failed to spawn AI for Team %d"), TeamID);
		}
	}
}

bool ABaseMatchGameMode::TrySpawnAIShipAtStart(ATeamPlayerStart* PlayerStart, const int32 TeamID)
{
	if (!PlayerStart)
	{
		return false;
	}

	const TSubclassOf<AShipPawn> ShipClass = GetRandomAISpawnClass(TeamID);
	const bool bValid = IsValidAISpawnClass(ShipClass);

	if (!bValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrySpawnAIShipAtStart: Invalid ship class for team %d"), TeamID);
		return false;
	}

	UShipSpawnSubsystem* ShipSpawner = GetShipSpawnSubsystem();
	if (!ShipSpawner)
	{
		return false;
	}

	AShipPawn* SpawnedShip = ShipSpawner->SpawnShip(ShipClass, PlayerStart->GetActorTransform(), TeamID, true);
	if (!SpawnedShip)
	{
		return false;
	}

	AssignAIPlayerStateTeam(SpawnedShip, TeamID);
	return true;
}

void ABaseMatchGameMode::AssignAIPlayerStateTeam(AShipPawn* SpawnedShip, const int32 TeamID)
{
	if (!SpawnedShip)
	{
		return;
	}

	AShipPlayerState* ShipState = SpawnedShip->GetPlayerState<AShipPlayerState>();
	if (!ShipState)
	{
		return;
	}

	ShipState->SetTeamID(TeamID);
}

TArray<ATeamPlayerStart*> ABaseMatchGameMode::FindAllPlayerStartsForTeam(const int32 TeamID) const
{
	TArray<AActor*> AllStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATeamPlayerStart::StaticClass(), AllStarts);

	TArray<ATeamPlayerStart*> FilteredStarts;

	for (AActor* Actor : AllStarts)
	{
		ATeamPlayerStart* Start = Cast<ATeamPlayerStart>(Actor);
		if (!Start)
		{
			continue;
		}

		if (Start->GetTeamID() != TeamID)
		{
			continue;
		}

		FilteredStarts.Add(Start);
	}

	return FilteredStarts;
}

TSubclassOf<AShipPawn> ABaseMatchGameMode::GetRandomAISpawnClass(const int32 TeamID) const
{
	const TArray<TSubclassOf<AShipPawn>>* PoolPtr = nullptr;

	if (TeamID == TeamA_ID)
	{
		PoolPtr = &AllowedAISpawnableShips_TeamA;
	}

	if (TeamID == TeamB_ID)
	{
		PoolPtr = &AllowedAISpawnableShips_TeamB;
	}

	if (!PoolPtr)
	{
		return nullptr;
	}

	const TArray<TSubclassOf<AShipPawn>>& Pool = *PoolPtr;

	if (Pool.Num() <= 0)
	{
		return nullptr;
	}

	const int32 Index = FMath::RandRange(0, Pool.Num() - 1);
	return Pool[Index];
}

bool ABaseMatchGameMode::IsValidAISpawnClass(const TSubclassOf<AShipPawn>& ShipClass) const
{
	if (!ShipClass)
	{
		return false;
	}

	return ShipClass->IsChildOf(AShipPawn::StaticClass());
}

UShipSpawnSubsystem* ABaseMatchGameMode::GetShipSpawnSubsystem() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<UShipSpawnSubsystem>();
}

void ABaseMatchGameMode::InitializePools()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UActorPoolSubsystem* Pool = World->GetSubsystem<UActorPoolSubsystem>();
	if (!Pool)
	{
		return;
	}

	UGalacticGameInstance* GameInstance = GetGameInstance<UGalacticGameInstance>();
	if (GameInstance)
	{
		const TSubclassOf<AShipPawn> PlayerShipClass = GameInstance->GetDefaultPlayerShipClass();
		if (IsValidAISpawnClass(PlayerShipClass))
		{
			Pool->InitializePool(*PlayerShipClass, 1);
		}
	}

	const int32 TeamAStarts = FindAllPlayerStartsForTeam(TeamA_ID).Num();
	const int32 TeamBStarts = FindAllPlayerStartsForTeam(TeamB_ID).Num();

	for (const TSubclassOf<AShipPawn>& C : AllowedAISpawnableShips_TeamA)
	{
		if (IsValidAISpawnClass(C))
		{
			Pool->InitializePool(*C, TeamAStarts);
		}
	}

	for (const TSubclassOf<AShipPawn>& C : AllowedAISpawnableShips_TeamB)
	{
		if (IsValidAISpawnClass(C))
		{
			Pool->InitializePool(*C, TeamBStarts);
		}
	}
}

void ABaseMatchGameMode::CleanupSpawnedShips()
{
	UShipSpawnSubsystem* ShipSpawner = GetShipSpawnSubsystem();
	if (!ShipSpawner)
	{
		return;
	}

	const TArray<TObjectPtr<AShipPawn>> ShipsCopy = ShipSpawner->GetAllShips();
	for (AShipPawn* Ship : ShipsCopy)
	{
		if (IsValid(Ship))
		{
			ShipSpawner->DespawnShip(Ship);
		}
	}
}

void ABaseMatchGameMode::ConfigureAIChaseBudgets()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UAICommandSubsystem* Cmd = World->GetSubsystem<UAICommandSubsystem>();
	if (!Cmd)
	{
		return;
	}

	UShipSpawnSubsystem* Spawner = World->GetSubsystem<UShipSpawnSubsystem>();
	if (!Spawner)
	{
		return;
	}

	TMap<uint8, int32> TeamSizes;

	const TArray<TObjectPtr<AShipPawn>>& Ships = Spawner->GetAllShips();
	for (const AShipPawn* Ship : Ships)
	{
		if (!IsValid(Ship))
		{
			continue;
		}

		const UHealthComponent* Health = Ship->FindComponentByClass<UHealthComponent>();
		if (!Health)
		{
			continue;
		}

		const int32 RawTeamId = Health->GetTeamId();
		const uint8 TeamId = (uint8)FMath::Clamp(RawTeamId, 0, 255);

		int32& Count = TeamSizes.FindOrAdd(TeamId);
		Count++;
	}

	for (const auto& Pair : TeamSizes)
	{
		const uint8 TeamId = Pair.Key;
		const int32 TeamSize = Pair.Value;

		int32 Budget = DefaultPlayerChaseBudgetPerTeam;
		Budget = FMath::Clamp(Budget, 0, TeamSize);

		Cmd->SetPlayerChaseBudgetForTeam(TeamId, Budget);
	}
}