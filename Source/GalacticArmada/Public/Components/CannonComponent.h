#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CannonComponent.generated.h"

UENUM(BlueprintType)
enum class ECannonFireMode : uint8
{
    All UMETA(DisplayName = "All"),
    Sequential UMETA(DisplayName = "Sequential")
};

USTRUCT(BlueprintType)
struct FCannonFireProperties
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
    bool Enabled;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
    bool IsAutomaticFire;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
    float FireRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
    ECannonFireMode CannonFireMode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
    TArray<FName> FireLocationSocketNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
    TSubclassOf<class AProjectileBase> ProjectileClass;

    FCannonFireProperties()
        : Enabled(true),
          IsAutomaticFire(false),
          FireRate(1.0f),
          CannonFireMode(ECannonFireMode::All),
          ProjectileClass(nullptr)
    {
    }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GALACTICARMADA_API UCannonComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCannonComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
    TArray<FCannonFireProperties> CannonFirePropertiesArray;

    UPROPERTY(BlueprintReadWrite, Category = "Cannon")
    USkeletalMeshComponent* OwnerSkeletalMeshComponent;

    UFUNCTION(BlueprintCallable, Category = "Cannon")
    void BeginCannonFire(int32 CannonIndex);

    UFUNCTION(BlueprintCallable, Category = "Cannon")
    void EndCannonFire(int32 CannonIndex);

private:
    void FireAllCannons(const FCannonFireProperties& CannonFireProps) const;
    void FireSequentialCannon(const FCannonFireProperties& CannonFireProps, int32& CurrentCannonIndex) const;
    void AutomaticFire(int32 CannonIndex);
    void StartAutomaticFire(int32 CannonIndex);
    void StopAutomaticFire(int32 CannonIndex);

    TArray<int32> SequentialCannonIndices;
    TArray<FTimerHandle> AutomaticFireTimers;
};