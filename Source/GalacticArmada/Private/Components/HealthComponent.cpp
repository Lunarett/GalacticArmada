// HealthComponent.cpp

#include "Components/HealthComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogHealthComponent, Log, All);

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

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

bool UHealthComponent::TryGetTeamFromActorOrController(const AActor* Actor, FGenericTeamId& OutTeam)
{
	if (!Actor)
	{
		return false;
	}

	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor))
	{
		OutTeam = TeamAgent->GetGenericTeamId();
		return true;
	}

	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const AController* C = Pawn->GetController())
		{
			if (const IGenericTeamAgentInterface* ControllerAgent = Cast<IGenericTeamAgentInterface>(C))
			{
				OutTeam = ControllerAgent->GetGenericTeamId();
				return true;
			}
		}
	}

	return false;
}

bool UHealthComponent::TryResolveInstigatorTeam(AActor* InstigatorActor, FGenericTeamId& OutTeam)
{
	// Most common: projectile/weapon actor owned by pawn/controller
	if (TryGetTeamFromActorOrController(InstigatorActor, OutTeam))
	{
		return true;
	}

	if (InstigatorActor)
	{
		if (TryGetTeamFromActorOrController(InstigatorActor->GetOwner(), OutTeam))
		{
			return true;
		}
	}

	return false;
}

bool UHealthComponent::ShouldBlockDamageAsFriendlyFire(AActor* InstigatorActor) const
{
	if (bAllowFriendlyFire)
	{
		return false;
	}

	FGenericTeamId TargetTeam = FGenericTeamId::NoTeam;
	if (!TryGetTeamFromActorOrController(GetOwner(), TargetTeam) || TargetTeam == FGenericTeamId::NoTeam)
	{
		return false;
	}

	FGenericTeamId InstigatorTeam = FGenericTeamId::NoTeam;
	if (!TryResolveInstigatorTeam(InstigatorActor, InstigatorTeam) || InstigatorTeam == FGenericTeamId::NoTeam)
	{
		return false;
	}

	return TargetTeam == InstigatorTeam;
}

void UHealthComponent::HandleTakeAnyDamage(
	AActor* DamagedActor,
	float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser
)
{
	if (Damage <= 0.f || bIsDead)
	{
		return;
	}

	// Prefer causer (weapon/projectile), fall back to controller
	AActor* InstigatorActor = DamageCauser ? DamageCauser : Cast<AActor>(InstigatedBy);
	ApplyDamage(Damage, InstigatorActor);
}

void UHealthComponent::ApplyDamage(float DamageAmount, AActor* InstigatorActor)
{
	if (DamageAmount <= 0.f || bIsDead)
	{
		return;
	}

	if (ShouldBlockDamageAsFriendlyFire(InstigatorActor))
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);
	const float Delta = CurrentHealth - OldHealth;

	OnDamaged.Broadcast(this, DamageAmount, InstigatorActor);
	OnHealthChanged.Broadcast(this, CurrentHealth, Delta, InstigatorActor);

	if (CurrentHealth <= 0.f && !bIsDead)
	{
		bIsDead = true;

		AController* KillerController = nullptr;
		AActor* DamageCauser = nullptr;
		ExtractKillerInfo(InstigatorActor, KillerController, DamageCauser);

		OnDeath.Broadcast(GetOwner(), KillerController, DamageCauser);
	}
}

void UHealthComponent::Heal(float HealAmount, AActor* HealerActor)
{
	if (HealAmount <= 0.f || bIsDead)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.f, MaxHealth);
	const float Delta = CurrentHealth - OldHealth;

	if (Delta <= 0.f)
	{
		return;
	}

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

	const float OldHealth = CurrentHealth;
	CurrentHealth = 0.f;
	const float Delta = CurrentHealth - OldHealth;

	bIsDead = true;

	OnHealthChanged.Broadcast(this, CurrentHealth, Delta, InstigatorActor);

	AController* KillerController = nullptr;
	AActor* DamageCauser = nullptr;
	ExtractKillerInfo(InstigatorActor, KillerController, DamageCauser);

	OnDeath.Broadcast(GetOwner(), KillerController, DamageCauser);
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bClampCurrent)
{
	MaxHealth = FMath::Max(NewMaxHealth, 0.f);

	if (bClampCurrent)
	{
		const float OldHealth = CurrentHealth;
		CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);
		const float Delta = CurrentHealth - OldHealth;

		if (!FMath::IsNearlyZero(Delta))
		{
			OnHealthChanged.Broadcast(this, CurrentHealth, Delta, nullptr);
		}
	}
}

void UHealthComponent::ExtractKillerInfo(AActor* InstigatorActor, AController*& OutController, AActor*& OutDamageCauser)
{
	OutController = nullptr;
	OutDamageCauser = InstigatorActor;

	if (!InstigatorActor)
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