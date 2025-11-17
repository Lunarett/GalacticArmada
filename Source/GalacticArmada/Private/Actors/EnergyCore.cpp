#include "Actors/EnergyCore.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HealthComponent.h"

AEnergyCore::AEnergyCore()
{
	PrimaryActorTick.bCanEverTick = false;

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	RootComponent = CoreMesh;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void AEnergyCore::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent != nullptr)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AEnergyCore::HandleCoreDeath);
	}
}

void AEnergyCore::HandleCoreDeath(AActor* DeadActor, AController* Killer, AActor* DamageCauser)
{
	if (DeathEffect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DeathEffect,
			FVector(GetActorLocation() + EffectOffset),
			GetActorRotation(),
			FVector(FVector::One() * EffectScale)
		);
	}

	OnCoreDestroyed.Broadcast(this);

	Destroy();
}