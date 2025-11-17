#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnergyCore.generated.h"

class UNiagaraSystem;
class UStaticMeshComponent;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoreDestroyedSignature, AEnergyCore*, DestroyedCore);

UCLASS()
class GALACTICARMADA_API AEnergyCore : public AActor
{
	GENERATED_BODY()

public:
	AEnergyCore();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* CoreMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UHealthComponent* HealthComponent;

	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* DeathEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	float EffectScale = 10;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FVector EffectOffset;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCoreDestroyedSignature OnCoreDestroyed;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleCoreDeath(AActor* DeadActor, AController* Killer, AActor* DamageCauser);
};