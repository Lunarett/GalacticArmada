#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Team")
	int32 TeamId = 0;

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
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	                         AController* InstigatedBy, AActor* DamageCauser);

public:
	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyDamage(const float DamageAmount, AActor* InstigatorActor = nullptr, const int32 InstigatorTeamId = -1);

	UFUNCTION(BlueprintCallable, Category="Health")
	void Heal(const float HealAmount, AActor* HealerActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void Kill(AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void SetMaxHealth(const float NewMaxHealth, const bool bClampCurrent = true);

	UFUNCTION(BlueprintCallable, Category="Team")
	void SetTeamId(const int32 NewTeamId) { TeamId = NewTeamId; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool GetIsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category="Team")
	int32 GetTeamId() const { return TeamId; }
};