#include "Components/HealthComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogHealthComponent, Log, All);

namespace
{
	static void ExtractKillerInfo(AActor* InstigatorActor, AController*& OutController, AActor*& OutDamageCauser)
	{
		OutController = nullptr;
		OutDamageCauser = InstigatorActor;

		if (InstigatorActor == nullptr)
		{
			return;
		}

		if (APawn* Pawn = Cast<APawn>(InstigatorActor))
		{
			OutController = Pawn->GetController();
			return;
		}

		if (AController* Controller = Cast<AController>(InstigatorActor))
		{
			OutController = Controller;
			OutDamageCauser = Controller->GetPawn();
			return;
		}

		if (APawn* OwnerPawn = Cast<APawn>(InstigatorActor->GetOwner()))
		{
			OutController = OwnerPawn->GetController();
		}
	}
}

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bStartAtMaxHealth)
	{
		CurrentHealth = MaxHealth;
	}
	else
	{
		CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);
	}

	bIsDead = CurrentHealth <= 0.f;

	AActor* Owner = GetOwner();

	if (Owner != nullptr)
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}

	UE_LOG(LogHealthComponent, Log,
	       TEXT("Health Init: Owner=%s Max=%.2f Current=%.2f Dead=%d"),
	       Owner ? *Owner->GetName() : TEXT("None"),
	       MaxHealth,
	       CurrentHealth,
	       bIsDead ? 1 : 0);
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                           AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f)
	{
		return;
	}

	if (bIsDead)
	{
		return;
	}

	int32 InstigatorTeamId = -1;

	if (InstigatedBy != nullptr)
	{
		APawn* InstigatorPawn = InstigatedBy->GetPawn();

		if (InstigatorPawn != nullptr)
		{
			UHealthComponent* InstigatorHealth =
				InstigatorPawn->FindComponentByClass<UHealthComponent>();

			if (InstigatorHealth != nullptr)
			{
				InstigatorTeamId = InstigatorHealth->GetTeamId();
			}
		}
	}

	ApplyDamage(Damage, DamageCauser, InstigatorTeamId);
}

void UHealthComponent::ApplyDamage(const float DamageAmount, AActor* InstigatorActor, const int32 InstigatorTeamId)
{
	if (DamageAmount <= 0.f)
	{
		return;
	}

	if (bIsDead)
	{
		return;
	}

	const bool bValidTeamInfo = (InstigatorTeamId != -1);
	const bool bFriendly = bValidTeamInfo && (InstigatorTeamId == TeamId);

	if (!bAllowFriendlyFire && bFriendly)
	{
		UE_LOG(LogHealthComponent, Verbose,
		       TEXT("Damage blocked (friendly fire): TargetTeam=%d InstigatorTeam=%d"),
		       TeamId, InstigatorTeamId);
		return;
	}

	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);
	float Delta = CurrentHealth - OldHealth;

	UE_LOG(LogHealthComponent, Log,
	       TEXT("Damage: Owner=%s Amount=%.2f NewHealth=%.2f Instigator=%s Team=%d"),
	       GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
	       DamageAmount,
	       CurrentHealth,
	       InstigatorActor ? *InstigatorActor->GetName() : TEXT("None"),
	       InstigatorTeamId);

	OnDamaged.Broadcast(this, DamageAmount, InstigatorActor);
	OnHealthChanged.Broadcast(this, CurrentHealth, Delta, InstigatorActor);

	if (CurrentHealth <= 0.f && !bIsDead)
	{
		bIsDead = true;

		AController* KillerController = nullptr;
		AActor* DamageCauser = nullptr;

		ExtractKillerInfo(InstigatorActor, KillerController, DamageCauser);

		UE_LOG(LogHealthComponent, Log,
		       TEXT("Death: Owner=%s KillerCtrl=%s DamageCauser=%s"),
		       GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
		       KillerController ? *KillerController->GetName() : TEXT("None"),
		       DamageCauser ? *DamageCauser->GetName() : TEXT("None"));

		OnDeath.Broadcast(GetOwner(), KillerController, DamageCauser);
	}
}

void UHealthComponent::Heal(const float HealAmount, AActor* HealerActor)
{
	if (HealAmount <= 0.f)
	{
		return;
	}

	if (bIsDead)
	{
		return;
	}

	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.f, MaxHealth);
	float Delta = CurrentHealth - OldHealth;

	if (Delta <= 0.f)
	{
		return;
	}

	UE_LOG(LogHealthComponent, Log,
	       TEXT("Heal: Owner=%s Amount=%.2f NewHealth=%.2f Healer=%s"),
	       GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
	       Delta,
	       CurrentHealth,
	       HealerActor ? *HealerActor->GetName() : TEXT("None"));

	OnHealed.Broadcast(this, Delta, HealerActor);
	OnHealthChanged.Broadcast(this, CurrentHealth, Delta, HealerActor);

	if (CurrentHealth > 0.f && bIsDead)
	{
		bIsDead = false;
	}
}

void UHealthComponent::Kill(AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	float OldHealth = CurrentHealth;
	CurrentHealth = 0.f;
	float Delta = CurrentHealth - OldHealth;

	bIsDead = true;

	OnHealthChanged.Broadcast(this, CurrentHealth, Delta, InstigatorActor);

	AController* KillerController = nullptr;
	AActor* DamageCauser = nullptr;

	ExtractKillerInfo(InstigatorActor, KillerController, DamageCauser);

	OnDeath.Broadcast(GetOwner(), KillerController, DamageCauser);
}

void UHealthComponent::SetMaxHealth(const float NewMaxHealth, const bool bClampCurrent)
{
	MaxHealth = FMath::Max(NewMaxHealth, 0.f);

	if (bClampCurrent)
	{
		float OldHealth = CurrentHealth;
		CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);

		float Delta = CurrentHealth - OldHealth;

		if (!FMath::IsNearlyZero(Delta))
		{
			OnHealthChanged.Broadcast(this, CurrentHealth, Delta, nullptr);
		}
	}
}