// HealthComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GenericTeamAgentInterface.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnHealthChangedSignature,
	UHealthComponent*, HealthComp,
	float, NewHealth,
	float, Delta,
	AActor*, InstigatorActor
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDamagedSignature,
	UHealthComponent*, HealthComp,
	float, DamageAmount,
	AActor*, InstigatorActor
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnHealedSignature,
	UHealthComponent*, HealthComp,
	float, HealAmount,
	AActor*, HealerActor
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDeathSignature,
	AActor*, DeadActor,
	AController*, KillerController,
	AActor*, DamageCauser
);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GALACTICARMADA_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
	bool bStartAtMaxHealth = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bAllowFriendlyFire = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	bool bIsDead = false;

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnDamagedSignature OnDamaged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnHealedSignature OnHealed;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnDeathSignature OnDeath;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTakeAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser
	);

public:
	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyDamage(float DamageAmount, AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void Heal(float HealAmount, AActor* HealerActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void Kill(AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void SetMaxHealth(float NewMaxHealth, bool bClampCurrent = true);

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool GetIsDead() const { return bIsDead; }

private:
	bool ShouldBlockDamageAsFriendlyFire(AActor* InstigatorActor) const;

	// Team helpers (no stored team data here)
	static bool TryGetTeamFromActorOrController(const AActor* Actor, FGenericTeamId& OutTeam);
	static bool TryResolveInstigatorTeam(AActor* InstigatorActor, FGenericTeamId& OutTeam);

	static void ExtractKillerInfo(AActor* InstigatorActor, AController*& OutController, AActor*& OutDamageCauser);
};