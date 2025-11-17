// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/DestroyEnergyCoresGameMode.h"
#include "Actors/EnergyCore.h"
#include "Blueprint/UserWidget.h"
#include "Player/ShipPawn.h"
#include "Components/HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ADestroyEnergyCoresGameMode::ADestroyEnergyCoresGameMode()
{
}

void ADestroyEnergyCoresGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundCores;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnergyCore::StaticClass(), FoundCores);

	for (AActor* Actor : FoundCores)
	{
		AEnergyCore* Core = Cast<AEnergyCore>(Actor);
		if (Core != nullptr)
		{
			EnergyCoresOnMap.Add(Core);
		}
	}

	RemainingCoreCount = EnergyCoresOnMap.Num();

	for (AEnergyCore* Core : EnergyCoresOnMap)
	{
		Core->OnCoreDestroyed.AddDynamic(this, &ADestroyEnergyCoresGameMode::HandleCoreDestroyed);
	}

	BindPlayerDeathEvent();
}

void ADestroyEnergyCoresGameMode::BeginMatchPhase()
{
}

void ADestroyEnergyCoresGameMode::HandleCoreDestroyed(AEnergyCore* DestroyedCore)
{
	if (DestroyedCore == nullptr)
	{
		return;
	}

	RemainingCoreCount--;

	if (RemainingCoreCount <= 0)
	{
		CompleteMatch(true);
	}
}

void ADestroyEnergyCoresGameMode::BindPlayerDeathEvent()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC == nullptr)
	{
		return;
	}

	AShipPawn* Pawn = Cast<AShipPawn>(PC->GetPawn());
	if (Pawn == nullptr)
	{
		return;
	}

	UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (Health == nullptr)
	{
		return;
	}

	Health->OnDeath.AddDynamic(this, &ADestroyEnergyCoresGameMode::HandlePlayerDeath);
}

void ADestroyEnergyCoresGameMode::HandlePlayerDeath(AActor* DeadActor, AController* Killer, AActor* DamageCauser)
{
	CompleteMatch(false);
}

void ADestroyEnergyCoresGameMode::CompleteMatch(bool bSuccess)
{
	ApplySlowMotion();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC != nullptr)
	{
		EnableUIOnlyInput(PC);
	}

	if (bSuccess && WinWidget != nullptr)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WinWidget->GetClass());
		if (Widget)
		{
			Widget->AddToViewport();
		}
	}
	else if (!bSuccess && LoseWidget != nullptr)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(PC, LoseWidget->GetClass());
		if (Widget)
		{
			Widget->AddToViewport();
		}
	}
}

void ADestroyEnergyCoresGameMode::ApplySlowMotion() const
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), EndMatchTimeScale);
}

void ADestroyEnergyCoresGameMode::EnableUIOnlyInput(APlayerController* PC) const
{
	if (PC == nullptr)
	{
		return;
	}

	//PC->SetPause(true);
	PC->SetInputMode(FInputModeUIOnly());
	PC->bShowMouseCursor = true;
}

