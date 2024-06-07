#include "Actors/ProjectileBase.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileBase::AProjectileBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // Initialize Collision Component
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    RootComponent = CollisionComponent;
    CollisionComponent->InitSphereRadius(30.0f);
    CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectileBase::OnOverlapBegin);

    // Initialize Projectile Movement Component
    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovementComponent->InitialSpeed = 175000.0f;
    ProjectileMovementComponent->MaxSpeed = 250000.0f;
    ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
}

void AProjectileBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Check Null and Ignore Self
    if (!OtherActor || !OtherComp || OtherActor == GetOwner()) return;

    // Add Damage
    UGameplayStatics::ApplyPointDamage(OtherActor, Damage, GetActorLocation(), SweepResult, GetInstigatorController(), this, nullptr);

    // Spawn Impact Effects
    if (ImpactEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, GetActorLocation());
    }

    // Destroy Projectile Actor
    Destroy();
}
