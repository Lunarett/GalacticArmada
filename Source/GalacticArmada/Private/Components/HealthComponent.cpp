#include "Components/HealthComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"

UHealthComponent::UHealthComponent()
{
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    Health = DefaultHealth;

    if (AActor* Owner = GetOwner())
    {
        Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
        Owner->OnTakePointDamage.AddDynamic(this, &UHealthComponent::HandleTakePointDamage);
        Owner->OnTakeRadialDamage.AddDynamic(this, &UHealthComponent::HandleTakeRadialDamage);
    }
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (Damage <= 0.0f || Health <= 0.0f)
    {
        return;
    }

    Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);

    OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

    if (Health <= 0.0f)
    {
        OnDeath.Broadcast(InstigatedBy, DamageCauser);
    }
}

void UHealthComponent::HandleTakePointDamage(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser)
{
    HandleTakeAnyDamage(DamagedActor, Damage, DamageType, InstigatedBy, DamageCauser);
}

void UHealthComponent::HandleTakeRadialDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo, class AController* InstigatedBy, AActor* DamageCauser)
{
    HandleTakeAnyDamage(DamagedActor, Damage, DamageType, InstigatedBy, DamageCauser);
}
