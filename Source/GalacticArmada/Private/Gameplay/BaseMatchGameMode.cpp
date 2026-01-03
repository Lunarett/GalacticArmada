#include "Gameplay/BaseMatchGameMode.h"

#include "Gameplay/GalacticGameInstance.h"
#include "Gameplay/TeamPlayerStart.h"
#include "Optimization/ActorPool/ActorPoolSubsystem.h"
#include "Player/ShipPawn.h"
#include "Player/ShipPlayerController.h"
#include "Player/ShipPlayerState.h"
#include "Systems/ShipSpawnSubsystem.h"
#include "Systems/AITargetSelectionSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

const TArray<ATeamPlayerStart*> ABaseMatchGameMode::EmptyStarts;

static FString ObjPathSafe(const UObject* Obj)
{
	return Obj ? Obj->GetPathName() : FString(TEXT("None"));
}

FString ABaseMatchGameMode::TeamToString(const FGenericTeamId TeamId)
{
	if (TeamId == FGenericTeamId::NoTeam)
	{
		return TEXT("NoTeam(255)");
	}
	return FString::Printf(TEXT("%d"), (int32)TeamId.GetId());
}

ABaseMatchGameMode::ABaseMatchGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AShipPlayerController::StaticClass();
	PlayerStateClass = AShipPlayerState::StaticClass();
}

void ABaseMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[GM] BeginPlay World=%s GameMode=%s"),
		*ObjPathSafe(GetWorld()),
		*ObjPathSafe(this));

	CacheTeamStarts();
	LogCachedStarts();

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
	UE_LOG(LogTemp, Warning, TEXT("[GM] EndPlay Reason=%d"), (int32)EndPlayReason);
	CleanupSpawnedShips();
	Super::EndPlay(EndPlayReason);
}

void ABaseMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin Enter Player=%s Class=%s"),
		*ObjPathSafe(NewPlayer),
		NewPlayer ? *NewPlayer->GetClass()->GetName() : TEXT("None"));

	// Assign ASAP; Super::PostLogin may trigger spawn flow in some cases.
	if (NewPlayer)
	{
		LogControllerTeam(TEXT("PostLogin BEFORE ApplyTeam"), NewPlayer);

		ApplyTeamToController(NewPlayer, FGenericTeamId(PlayerTeamId));

		LogControllerTeam(TEXT("PostLogin AFTER ApplyTeam"), NewPlayer);
	}

	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin Exit Player=%s"), *ObjPathSafe(NewPlayer));
}

void ABaseMatchGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer Enter Player=%s"),
		*ObjPathSafe(NewPlayer));

	if (NewPlayer)
	{
		LogControllerTeam(TEXT("HandleStartingNewPlayer BEFORE ApplyTeam"), NewPlayer);

		ApplyTeamToController(NewPlayer, FGenericTeamId(PlayerTeamId));

		LogControllerTeam(TEXT("HandleStartingNewPlayer AFTER ApplyTeam"), NewPlayer);
	}

	// Engine flow: may call RestartPlayer -> ChoosePlayerStart -> SpawnDefaultPawnFor
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer AFTER Super Player=%s Pawn=%s"),
		*ObjPathSafe(NewPlayer),
		NewPlayer ? *ObjPathSafe(NewPlayer->GetPawn()) : TEXT("None"));

	if (!bHasSpawnedAI)
	{
		EnsureStartsCached();
		SpawnAllAI();
		bHasSpawnedAI = true;
	}

	ConfigureAIChaseBudgets();

	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer Exit Player=%s"), *ObjPathSafe(NewPlayer));
}

void ABaseMatchGameMode::StartMatchAfterDelay()
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] StartMatchAfterDelay fired"));

	if (!bHasSpawnedAI)
	{
		EnsureStartsCached();
		SpawnAllAI();
		bHasSpawnedAI = true;
	}

	ConfigureAIChaseBudgets();
	BeginMatchPhase();
}

void ABaseMatchGameMode::BeginMatchPhase()
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] BeginMatchPhase"));
}

void ABaseMatchGameMode::CompleteMatch(const bool bSuccess)
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] CompleteMatch Success=%d"), bSuccess ? 1 : 0);
}

AActor* ABaseMatchGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] ChoosePlayerStart Enter Player=%s"), *ObjPathSafe(Player));

	if (!Player)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	EnsureStartsCached();

	const FGenericTeamId TeamId = GetControllerTeamId(Player);

	// CRITICAL FIX:
	// ChoosePlayerStart can be called before PostLogin/HandleStartingNewPlayer sets the team.
	// If Team is NoTeam, treat as PlayerTeamId so we still pick correct player spawn.
	const uint8 EffectiveTeam =
		(TeamId == FGenericTeamId::NoTeam) ? PlayerTeamId : TeamId.GetId();

	if (TeamId == FGenericTeamId::NoTeam)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GM] ChoosePlayerStart: %s has NoTeam -> using PlayerTeamId=%d for start selection"),
			*GetNameSafe(Player), (int32)PlayerTeamId);
	}

	const TArray<ATeamPlayerStart*>& PlayerOnly = GetCachedPlayerStarts(EffectiveTeam);
	const TArray<ATeamPlayerStart*>& AIStarts   = GetCachedAIStarts(EffectiveTeam);

	UE_LOG(LogTemp, Warning, TEXT("[GM] ChoosePlayerStart Team=%s EffectiveTeam=%d PlayerOnly=%d AIStarts=%d"),
		*TeamToString(TeamId),
		(int32)EffectiveTeam,
		PlayerOnly.Num(),
		AIStarts.Num());

	ATeamPlayerStart* Selected = PickRandomStart(PlayerOnly);
	if (!Selected)
	{
		Selected = PickRandomStart(AIStarts);
	}

	if (!Selected)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] ChoosePlayerStart FAIL: No starts for EffectiveTeam=%d -> fallback"),
			(int32)EffectiveTeam);
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GM] ChoosePlayerStart Selected=%s Loc=%s PlayerOnly=%d"),
		*GetNameSafe(Selected),
		*Selected->GetActorLocation().ToString(),
		Selected->IsPlayerOnly() ? 1 : 0);

	return Selected;
}

APawn* ABaseMatchGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] SpawnDefaultPawnFor Enter Controller=%s Start=%s"),
		*ObjPathSafe(NewPlayer),
		*ObjPathSafe(StartSpot));

	if (!NewPlayer || !StartSpot)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] SpawnDefaultPawnFor FAIL: Missing Controller/StartSpot"));
		return nullptr;
	}

	UGalacticGameInstance* GI = GetGameInstance<UGalacticGameInstance>();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] SpawnDefaultPawnFor FAIL: Missing GameInstance"));
		return nullptr;
	}

	const TSubclassOf<AShipPawn> ShipClass = GI->GetDefaultPlayerShipClass();
	if (!IsValidShipClass(ShipClass))
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] SpawnDefaultPawnFor FAIL: Invalid ShipClass=%s"),
			ShipClass ? *ShipClass->GetName() : TEXT("None"));
		return nullptr;
	}

	UShipSpawnSubsystem* Spawner = GetShipSpawnSubsystem();
	if (!Spawner)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] SpawnDefaultPawnFor FAIL: Missing ShipSpawnSubsystem"));
		return nullptr;
	}

	const FGenericTeamId TeamId = GetControllerTeamId(NewPlayer);
	const FTransform SpawnXform = StartSpot->GetActorTransform();

	UE_LOG(LogTemp, Warning, TEXT("[GM] SpawnDefaultPawnFor Request: Controller=%s Team=%s Start=%s ReqLoc=%s"),
		*ObjPathSafe(NewPlayer),
		*TeamToString(TeamId),
		*ObjPathSafe(StartSpot),
		*SpawnXform.GetLocation().ToString());

	AShipPawn* Spawned = Spawner->SpawnShip(
		ShipClass,
		SpawnXform,
		(int32)TeamId.GetId(),
		false
	);

	LogSpawnResult(TEXT("SpawnDefaultPawnFor SpawnShip"), NewPlayer, SpawnXform, Spawned);

	if (!Spawned)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] SpawnDefaultPawnFor FAIL: SpawnShip returned null"));
		return nullptr;
	}

	Spawned->SetOwner(NewPlayer);
	return Spawned;
}

void ABaseMatchGameMode::RestartPlayer(AController* NewPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] RestartPlayer Enter Controller=%s Pawn=%s"),
		*ObjPathSafe(NewPlayer),
		NewPlayer ? *ObjPathSafe(NewPlayer->GetPawn()) : TEXT("None"));

	if (!NewPlayer)
	{
		return;
	}

	EnsureStartsCached();

	AActor* StartSpot = ChoosePlayerStart(NewPlayer);
	UE_LOG(LogTemp, Warning, TEXT("[GM] RestartPlayer StartSpot=%s Loc=%s"),
		*ObjPathSafe(StartSpot),
		StartSpot ? *StartSpot->GetActorLocation().ToString() : TEXT("None"));

	if (!StartSpot)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] RestartPlayer FAIL: no StartSpot"));
		return;
	}

	// If something else already created/possessed a pawn (AutoPossess, etc),
	// force it to the chosen start instead of letting it sit at origin.
	if (APawn* ExistingPawn = NewPlayer->GetPawn())
	{
		const FTransform T = StartSpot->GetActorTransform();
		ExistingPawn->TeleportTo(T.GetLocation(), T.Rotator(), false, true);

		UE_LOG(LogTemp, Warning, TEXT("[GM] RestartPlayer ExistingPawn moved Pawn=%s NewLoc=%s"),
			*ObjPathSafe(ExistingPawn),
			*ExistingPawn->GetActorLocation().ToString());

		return;
	}

	// Otherwise spawn through our subsystem path (this is the one you want).
	APawn* NewPawn = SpawnDefaultPawnFor(NewPlayer, StartSpot);
	if (!NewPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] RestartPlayer FAIL: SpawnDefaultPawnFor returned null"));
		return;
	}

	NewPlayer->Possess(NewPawn);

	UE_LOG(LogTemp, Warning, TEXT("[GM] RestartPlayer Possessed Pawn=%s FinalLoc=%s"),
		*ObjPathSafe(NewPawn),
		*NewPawn->GetActorLocation().ToString());
}

void ABaseMatchGameMode::SpawnAllAI()
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] SpawnAllAI TeamA=%d TeamB=%d"), (int32)TeamAId, (int32)TeamBId);
	SpawnAIForTeam(TeamAId);
	SpawnAIForTeam(TeamBId);
}

void ABaseMatchGameMode::SpawnAIForTeam(const uint8 TeamId)
{
	EnsureStartsCached();

	const TArray<ATeamPlayerStart*>& AIStarts = GetCachedAIStarts(TeamId);

	UE_LOG(LogTemp, Warning, TEXT("[GM] SpawnAIForTeam Team=%d Starts=%d"), (int32)TeamId, AIStarts.Num());

	for (const ATeamPlayerStart* Start : AIStarts)
	{
		if (!IsValid(Start))
		{
			UE_LOG(LogTemp, Error, TEXT("[GM] SpawnAIForTeam invalid Start ptr"));
			continue;
		}

		const bool bOk = TrySpawnAIShipAtStart(Start, TeamId);
		if (!bOk)
		{
			UE_LOG(LogTemp, Error, TEXT("[GM] SpawnAIForTeam FAIL Team=%d Start=%s"),
				(int32)TeamId, *ObjPathSafe(Start));
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
	if (!IsValidShipClass(ShipClass))
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] TrySpawnAIShipAtStart invalid ShipClass Team=%d"), (int32)TeamId);
		return false;
	}

	UShipSpawnSubsystem* Spawner = GetShipSpawnSubsystem();
	if (!Spawner)
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] TrySpawnAIShipAtStart missing ShipSpawnSubsystem"));
		return false;
	}

	const FTransform Xform = PlayerStart->GetActorTransform();

	AShipPawn* Spawned = Spawner->SpawnShip(
		ShipClass,
		Xform,
		(int32)TeamId,
		true
	);

	LogSpawnResult(TEXT("SpawnAI SpawnShip"), nullptr, Xform, Spawned);

	if (!Spawned)
	{
		return false;
	}

	if (AController* AIController = Spawned->GetController())
	{
		ApplyTeamToController(AIController, FGenericTeamId(TeamId));
		LogControllerTeam(TEXT("SpawnAI AFTER ApplyTeam"), AIController);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GM] SpawnAI OK Team=%d Start=%s ReqLoc=%s Spawned=%s FinalLoc=%s"),
		(int32)TeamId,
		*ObjPathSafe(PlayerStart),
		*Xform.GetLocation().ToString(),
		*ObjPathSafe(Spawned),
		Spawned ? *Spawned->GetActorLocation().ToString() : TEXT("None"));

	return true;
}

ATeamPlayerStart* ABaseMatchGameMode::PickRandomStart(const TArray<ATeamPlayerStart*>& Starts) const
{
	if (Starts.Num() <= 0)
	{
		return nullptr;
	}
	return Starts[FMath::RandRange(0, Starts.Num() - 1)];
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

	if (!PoolPtr || PoolPtr->Num() <= 0)
	{
		return nullptr;
	}

	return (*PoolPtr)[FMath::RandRange(0, PoolPtr->Num() - 1)];
}

bool ABaseMatchGameMode::IsValidShipClass(const TSubclassOf<AShipPawn>& ShipClass) const
{
	return ShipClass && ShipClass->IsChildOf(AShipPawn::StaticClass());
}

UShipSpawnSubsystem* ABaseMatchGameMode::GetShipSpawnSubsystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UShipSpawnSubsystem>() : nullptr;
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

	UGalacticGameInstance* GI = GetGameInstance<UGalacticGameInstance>();
	if (GI)
	{
		const TSubclassOf<AShipPawn> PlayerShipClass = GI->GetDefaultPlayerShipClass();
		if (IsValidShipClass(PlayerShipClass))
		{
			Pool->InitializePool(*PlayerShipClass, 1);
		}
	}

	EnsureStartsCached();

	const int32 TeamAStarts = GetCachedAIStarts(TeamAId).Num();
	const int32 TeamBStarts = GetCachedAIStarts(TeamBId).Num();

	for (const TSubclassOf<AShipPawn>& C : AllowedAISpawnableShips_TeamA)
	{
		if (IsValidShipClass(C))
		{
			Pool->InitializePool(*C, TeamAStarts);
		}
	}

	for (const TSubclassOf<AShipPawn>& C : AllowedAISpawnableShips_TeamB)
	{
		if (IsValidShipClass(C))
		{
			Pool->InitializePool(*C, TeamBStarts);
		}
	}
}

void ABaseMatchGameMode::CleanupSpawnedShips()
{
	UShipSpawnSubsystem* Spawner = GetShipSpawnSubsystem();
	if (!Spawner)
	{
		return;
	}

	const TArray<TObjectPtr<AShipPawn>> ShipsCopy = Spawner->GetAllShips();
	for (AShipPawn* Ship : ShipsCopy)
	{
		if (IsValid(Ship))
		{
			Spawner->DespawnShip(Ship);
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

	UAITargetSelectionSubsystem* TargetSubsys = World->GetSubsystem<UAITargetSelectionSubsystem>();
	UShipSpawnSubsystem* Spawner = World->GetSubsystem<UShipSpawnSubsystem>();
	if (!TargetSubsys || !Spawner)
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

		const FGenericTeamId TeamId = GetShipTeamIdFromController(Ship);
		if (TeamId == FGenericTeamId::NoTeam)
		{
			continue;
		}

		TeamSizes.FindOrAdd(TeamId.GetId())++;
	}

	for (const TPair<uint8, int32>& Pair : TeamSizes)
	{
		const uint8 TeamId = Pair.Key;
		const int32 TeamSize = Pair.Value;

		const int32 Budget = FMath::Clamp(DefaultPlayerChaseBudgetPerTeam, 0, TeamSize);
		UE_LOG(LogTemp, Warning, TEXT("[GM] ChaseBudget Team=%d Size=%d Budget=%d"), (int32)TeamId, TeamSize, Budget);

		TargetSubsys->SetPlayerChaseBudgetForTeam(TeamId, Budget);
	}
}

void ABaseMatchGameMode::ApplyTeamToController(AController* Controller, const FGenericTeamId& TeamId) const
{
	if (!Controller)
	{
		return;
	}

	IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Controller);
	UE_LOG(LogTemp, Warning, TEXT("[GM] ApplyTeamToController Controller=%s ImplementsIGenericTeam=%d SetTeam=%s"),
		*ObjPathSafe(Controller),
		TeamAgent ? 1 : 0,
		*TeamToString(TeamId));

	if (TeamAgent)
	{
		TeamAgent->SetGenericTeamId(TeamId);
	}
}

FGenericTeamId ABaseMatchGameMode::GetControllerTeamId(const AController* Controller) const
{
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Controller))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return FGenericTeamId(PlayerTeamId);
}

FGenericTeamId ABaseMatchGameMode::GetShipTeamIdFromController(const AShipPawn* Ship) const
{
	if (!Ship)
	{
		return FGenericTeamId::NoTeam;
	}

	const AController* Controller = Ship->GetController();
	if (!Controller)
	{
		return FGenericTeamId::NoTeam;
	}

	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Controller))
	{
		return TeamAgent->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

void ABaseMatchGameMode::CacheTeamStarts()
{
	CachedPlayerOnlyStartsByTeam.Reset();
	CachedAIStartsByTeam.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> All;
	UGameplayStatics::GetAllActorsOfClass(World, ATeamPlayerStart::StaticClass(), All);

	for (AActor* Actor : All)
	{
		ATeamPlayerStart* Start = Cast<ATeamPlayerStart>(Actor);
		if (!Start)
		{
			continue;
		}

		const uint8 TeamId = Start->GetTeamId();

		if (Start->IsPlayerOnly())
		{
			CachedPlayerOnlyStartsByTeam.FindOrAdd(TeamId).Add(Start);
		}
		else
		{
			CachedAIStartsByTeam.FindOrAdd(TeamId).Add(Start);
		}
	}
}

void ABaseMatchGameMode::EnsureStartsCached()
{
	if (CachedPlayerOnlyStartsByTeam.Num() == 0 && CachedAIStartsByTeam.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] EnsureStartsCached: cache empty -> recache now"));
		CacheTeamStarts();
		LogCachedStarts();
	}
}

const TArray<ATeamPlayerStart*>& ABaseMatchGameMode::GetCachedPlayerStarts(const uint8 TeamId) const
{
	if (const TArray<ATeamPlayerStart*>* Found = CachedPlayerOnlyStartsByTeam.Find(TeamId))
	{
		return *Found;
	}
	return EmptyStarts;
}

const TArray<ATeamPlayerStart*>& ABaseMatchGameMode::GetCachedAIStarts(const uint8 TeamId) const
{
	if (const TArray<ATeamPlayerStart*>* Found = CachedAIStartsByTeam.Find(TeamId))
	{
		return *Found;
	}
	return EmptyStarts;
}

void ABaseMatchGameMode::LogCachedStarts() const
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] Cached starts: PlayerOnlyTeams=%d AITeams=%d"),
		CachedPlayerOnlyStartsByTeam.Num(),
		CachedAIStartsByTeam.Num());

	for (const TPair<uint8, TArray<ATeamPlayerStart*>>& Pair : CachedPlayerOnlyStartsByTeam)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] Team %d PlayerOnlyStarts=%d"), (int32)Pair.Key, Pair.Value.Num());
	}

	for (const TPair<uint8, TArray<ATeamPlayerStart*>>& Pair : CachedAIStartsByTeam)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] Team %d AIStarts=%d"), (int32)Pair.Key, Pair.Value.Num());
	}
}

void ABaseMatchGameMode::LogControllerTeam(const TCHAR* Context, const AController* Controller) const
{
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] %s Controller=None"), Context);
		return;
	}

	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Controller);
	const FGenericTeamId TeamId = TeamAgent ? TeamAgent->GetGenericTeamId() : FGenericTeamId(PlayerTeamId);

	UE_LOG(LogTemp, Warning, TEXT("[GM] %s Controller=%s Class=%s ImplementsIGenericTeam=%d Team=%s Pawn=%s"),
		Context,
		*ObjPathSafe(Controller),
		*Controller->GetClass()->GetName(),
		TeamAgent ? 1 : 0,
		*TeamToString(TeamId),
		*ObjPathSafe(Controller->GetPawn()));
}

void ABaseMatchGameMode::LogStartChoice(const TCHAR* Context, const AController* Controller, const AActor* StartSpot) const
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] %s Controller=%s Start=%s StartLoc=%s StartRot=%s"),
		Context,
		*ObjPathSafe(Controller),
		*ObjPathSafe(StartSpot),
		StartSpot ? *StartSpot->GetActorLocation().ToString() : TEXT("None"),
		StartSpot ? *StartSpot->GetActorRotation().ToString() : TEXT("None"));
}

void ABaseMatchGameMode::LogSpawnResult(const TCHAR* Context, const AController* Controller, const FTransform& Requested, const APawn* SpawnedPawn) const
{
	const FVector ReqLoc = Requested.GetLocation();
	const FVector GotLoc = SpawnedPawn ? SpawnedPawn->GetActorLocation() : FVector::ZeroVector;

	UE_LOG(LogTemp, Warning, TEXT("[GM] %s Controller=%s ReqLoc=%s Spawned=%s GotLoc=%s RootLoc=%s"),
		Context,
		*ObjPathSafe(Controller),
		*ReqLoc.ToString(),
		*ObjPathSafe(SpawnedPawn),
		SpawnedPawn ? *GotLoc.ToString() : TEXT("None"),
		(SpawnedPawn && SpawnedPawn->GetRootComponent()) ? *SpawnedPawn->GetRootComponent()->GetComponentLocation().ToString() : TEXT("None"));
}