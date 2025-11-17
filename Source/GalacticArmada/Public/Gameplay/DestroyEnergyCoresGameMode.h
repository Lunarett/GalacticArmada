#pragma once

#include "CoreMinimal.h"
#include "BaseMatchGameMode.h"
#include "DestroyEnergyCoresGameMode.generated.h"

class AEnergyCore;

UCLASS()
class GALACTICARMADA_API ADestroyEnergyCoresGameMode : public ABaseMatchGameMode
{
	GENERATED_BODY()

public:
	ADestroyEnergyCoresGameMode();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties")
	float EndMatchTimeScale = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties")
	UUserWidget* WinWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Properties")
	UUserWidget* LoseWidget;
	
protected:
	virtual void BeginPlay() override;
	virtual void BeginMatchPhase() override;
	virtual void CompleteMatch(bool bSuccess) override;

private:
	UPROPERTY()
	TArray<AEnergyCore*> EnergyCoresOnMap;
	

	int32 RemainingCoreCount = 0;

private:
	void BindPlayerDeathEvent();

	UFUNCTION()
	void HandleCoreDestroyed(AEnergyCore* DestroyedCore);
	
	UFUNCTION()
	void HandlePlayerDeath(AActor* DeadActor, AController* Killer, AActor* DamageCauser);

	void ApplySlowMotion() const;
	void EnableUIOnlyInput(APlayerController* PC) const;
};