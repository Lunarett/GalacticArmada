#include "Gameplay/BaseMatchGameMode.h"

#include "Gameplay/GalacticGameInstance.h"
#include "Gameplay/TeamPlayerStart.h"
#include "Optimization/ActorPool/ActorPoolSubsystem.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerController.h"
#include "Player/ShipPlayerState.h"
#include "Systems/ShipSpawnSubsystem.h"
#include "Systems/AICommandSubsystem.h"

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

	// Controller is the only authoritative team holder.
	ApplyTeamToController(NewPlayer, FGenericTeamId(PlayerTeamId));

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
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Invalid controller"));
		return;
	}

	AActor* StartSpot = ChoosePlayerStart(NewPlayer);
	if (!StartSpot)
	{
		UE_LOG(LogTemp, Error, TEXT("RestartPlayer: Could not find valid PlayerStart"));
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

	const FGenericTeamId TeamId = GetControllerTeamId(Player);
	const uint8 RawTeamId = TeamId.GetId();

	const TArray<ATeamPlayerStart*> TeamStarts = FindAllPlayerStartsForTeam(RawTeamId);
	if (TeamStarts.Num() <= 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// Prefer "player-only" starts first.
	TArray<ATeamPlayerStart*> PlayerOnlyStarts;
	PlayerOnlyStarts.Reserve(TeamStarts.Num());

	for (ATeamPlayerStart* const Start : TeamStarts)
	{
		if (!Start)
		{
			continue;
		}

		if (Start->IsPlayerOnly())
		{
			PlayerOnlyStarts.Add(Start);
		}
	}

	ATeamPlayerStart* SelectedStart = PickRandomStart(PlayerOnlyStarts);
	if (SelectedStart)
	{
		return SelectedStart;
	}

	SelectedStart = PickRandomStart(TeamStarts);
	if (SelectedStart)
	{
		return SelectedStart;
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

	UGalacticGameInstance* const GameInstance = GetGameInstance<UGalacticGameInstance>();
	if (!GameInstance)
	{
		return nullptr;
	}

	const TSubclassOf<AShipPawn> ShipClass = GameInstance->GetDefaultPlayerShipClass();
	const bool bValidShipClass = IsValidShipClass(ShipClass);

	if (!bValidShipClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnDefaultPawnFor: Invalid player ship class"));
		return nullptr;
	}

	UShipSpawnSubsystem* const ShipSpawner = GetShipSpawnSubsystem();
	if (!ShipSpawner)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnDefaultPawnFor: Missing ShipSpawnSubsystem"));
		return nullptr;
	}

	// Team is owned by controller; the spawn call may still want a team value for internal logic.
	const FGenericTeamId TeamId = GetControllerTeamId(NewPlayer);

	AShipPawn* const SpawnedShip = ShipSpawner->SpawnShip(
		ShipClass,
		StartSpot->GetActorTransform(),
		static_cast<int32>(TeamId.GetId()),
		false
	);

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
	SpawnAIForTeam(TeamAId);
	SpawnAIForTeam(TeamBId);
}

void ABaseMatchGameMode::SpawnAIForTeam(const uint8 TeamId)
{
	const TArray<ATeamPlayerStart*> Starts = FindAllPlayerStartsForTeam(TeamId);

	for (const ATeamPlayerStart* const Start : Starts)
	{
		if (!Start)
		{
			continue;
		}

		if (Start->IsPlayerOnly())
		{
			continue;
		}

		const bool bSpawned = TrySpawnAIShipAtStart(Start, TeamId);
		if (!bSpawned)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnAIForTeam: Failed to spawn AI for Team %d"), static_cast<int32>(TeamId));
		}
	}
}

bool ABaseMatchGameMode::TrySpawnAIShipAtStart(const ATeamPlayerStart* PlayerStart, const uint8 TeamId)
{
	if (!PlayerStart)
	{
		return false;
	}

	const TSubclassOf<AShipPawn> ShipClass = GetRandomAISpawnClass(TeamId);
	const bool bValid = IsValidShipClass(ShipClass);

	if (!bValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrySpawnAIShipAtStart: Invalid ship class for team %d"), static_cast<int32>(TeamId));
		return false;
	}

	UShipSpawnSubsystem* const ShipSpawner = GetShipSpawnSubsystem();
	if (!ShipSpawner)
	{
		return false;
	}

	AShipPawn* const SpawnedShip = ShipSpawner->SpawnShip(
		ShipClass,
		PlayerStart->GetActorTransform(),
		static_cast<int32>(TeamId),
		true
	);

	if (!SpawnedShip)
	{
		return false;
	}

	// Controller owns team. If the spawned AI already has a controller, apply team to it.
	AController* const AIController = SpawnedShip->GetController();
	if (AIController)
	{
		ApplyTeamToController(AIController, FGenericTeamId(TeamId));
	}

	return true;
}

TArray<ATeamPlayerStart*> ABaseMatchGameMode::FindAllPlayerStartsForTeam(const uint8 TeamId) const
{
	TArray<AActor*> AllStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATeamPlayerStart::StaticClass(), AllStarts);

	TArray<ATeamPlayerStart*> FilteredStarts;
	FilteredStarts.Reserve(AllStarts.Num());

	for (AActor* const Actor : AllStarts)
	{
		ATeamPlayerStart* const Start = Cast<ATeamPlayerStart>(Actor);
		if (!Start)
		{
			continue;
		}

		if (Start->GetTeamId() != TeamId)
		{
			continue;
		}

		FilteredStarts.Add(Start);
	}

	return FilteredStarts;
}

ATeamPlayerStart* ABaseMatchGameMode::PickRandomStart(const TArray<ATeamPlayerStart*>& Starts) const
{
	const int32 NumStarts = Starts.Num();
	if (NumStarts <= 0)
	{
		return nullptr;
	}

	const int32 Index = FMath::RandRange(0, NumStarts - 1);
	return Starts[Index];
}

TSubclassOf<AShipPawn> ABaseMatchGameMode::GetRandomAISpawnClass(const uint8 TeamId) const
{
	const TArray<TSubclassOf<AShipPawn>>* PoolPtr = nullptr;

	if (TeamId == TeamAId)
	{
		PoolPtr = &AllowedAISpawnableShips_TeamA;
	}
	else if (TeamId == TeamBId)
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

bool ABaseMatchGameMode::IsValidShipClass(const TSubclassOf<AShipPawn>& ShipClass) const
{
	if (!ShipClass)
	{
		return false;
	}

	return ShipClass->IsChildOf(AShipPawn::StaticClass());
}

UShipSpawnSubsystem* ABaseMatchGameMode::GetShipSpawnSubsystem() const
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<UShipSpawnSubsystem>();
}

void ABaseMatchGameMode::InitializePools()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	UActorPoolSubsystem* const Pool = World->GetSubsystem<UActorPoolSubsystem>();
	if (!Pool)
	{
		return;
	}

	UGalacticGameInstance* const GameInstance = GetGameInstance<UGalacticGameInstance>();
	if (GameInstance)
	{
		const TSubclassOf<AShipPawn> PlayerShipClass = GameInstance->GetDefaultPlayerShipClass();
		if (IsValidShipClass(PlayerShipClass))
		{
			Pool->InitializePool(*PlayerShipClass, 1);
		}
	}

	int32 TeamA_AIStarts = 0;
	int32 TeamB_AIStarts = 0;

	{
		const TArray<ATeamPlayerStart*> TeamAStarts = FindAllPlayerStartsForTeam(TeamAId);
		for (const ATeamPlayerStart* const Start : TeamAStarts)
		{
			if (!Start)
			{
				continue;
			}

			if (Start->IsPlayerOnly())
			{
				continue;
			}

			TeamA_AIStarts++;
		}
	}

	{
		const TArray<ATeamPlayerStart*> TeamBStarts = FindAllPlayerStartsForTeam(TeamBId);
		for (const ATeamPlayerStart* const Start : TeamBStarts)
		{
			if (!Start)
			{
				continue;
			}

			if (Start->IsPlayerOnly())
			{
				continue;
			}

			TeamB_AIStarts++;
		}
	}

	for (const TSubclassOf<AShipPawn>& C : AllowedAISpawnableShips_TeamA)
	{
		if (IsValidShipClass(C))
		{
			Pool->InitializePool(*C, TeamA_AIStarts);
		}
	}

	for (const TSubclassOf<AShipPawn>& C : AllowedAISpawnableShips_TeamB)
	{
		if (IsValidShipClass(C))
		{
			Pool->InitializePool(*C, TeamB_AIStarts);
		}
	}
}

void ABaseMatchGameMode::CleanupSpawnedShips()
{
	UShipSpawnSubsystem* const ShipSpawner = GetShipSpawnSubsystem();
	if (!ShipSpawner)
	{
		return;
	}

	const TArray<TObjectPtr<AShipPawn>> ShipsCopy = ShipSpawner->GetAllShips();
	for (AShipPawn* const Ship : ShipsCopy)
	{
		if (!IsValid(Ship))
		{
			continue;
		}

		ShipSpawner->DespawnShip(Ship);
	}
}

void ABaseMatchGameMode::ConfigureAIChaseBudgets()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	UAICommandSubsystem* const Cmd = World->GetSubsystem<UAICommandSubsystem>();
	if (!Cmd)
	{
		return;
	}

	UShipSpawnSubsystem* const Spawner = World->GetSubsystem<UShipSpawnSubsystem>();
	if (!Spawner)
	{
		return;
	}

	TMap<uint8, int32> TeamSizes;

	const TArray<TObjectPtr<AShipPawn>>& Ships = Spawner->GetAllShips();
	for (const AShipPawn* const Ship : Ships)
	{
		if (!IsValid(Ship))
		{
			continue;
		}

		const FGenericTeamId TeamId = GetShipTeamIdFromController(Ship);
		if (TeamId == FGenericTeamId::NoTeam)
		{
			continue;
		}

		const uint8 Key = TeamId.GetId();
		int32& Count = TeamSizes.FindOrAdd(Key);
		Count++;
	}

	for (const TPair<uint8, int32>& Pair : TeamSizes)
	{
		const uint8 TeamId = Pair.Key;
		const int32 TeamSize = Pair.Value;

		int32 Budget = DefaultPlayerChaseBudgetPerTeam;
		Budget = FMath::Clamp(Budget, 0, TeamSize);

		Cmd->SetPlayerChaseBudgetForTeam(TeamId, Budget);
	}
}

void ABaseMatchGameMode::ApplyTeamToController(AController* Controller, const FGenericTeamId& TeamId) const
{
	if (!Controller)
	{
		return;
	}

	IGenericTeamAgentInterface* const TeamAgent = Cast<IGenericTeamAgentInterface>(Controller);
	if (!TeamAgent)
	{
		return;
	}

	TeamAgent->SetGenericTeamId(TeamId);
}

FGenericTeamId ABaseMatchGameMode::GetControllerTeamId(const AController* Controller) const
{
	if (!Controller)
	{
		return FGenericTeamId(PlayerTeamId);
	}

	const IGenericTeamAgentInterface* const TeamAgent = Cast<IGenericTeamAgentInterface>(Controller);
	if (!TeamAgent)
	{
		return FGenericTeamId(PlayerTeamId);
	}

	return TeamAgent->GetGenericTeamId();
}

FGenericTeamId ABaseMatchGameMode::GetShipTeamIdFromController(const AShipPawn* Ship) const
{
	if (!Ship)
	{
		return FGenericTeamId::NoTeam;
	}

	const AController* const Controller = Ship->GetController();
	if (!Controller)
	{
		return FGenericTeamId::NoTeam;
	}

	const IGenericTeamAgentInterface* const TeamAgent = Cast<IGenericTeamAgentInterface>(Controller);
	if (!TeamAgent)
	{
		return FGenericTeamId::NoTeam;
	}

	return TeamAgent->GetGenericTeamId();
}