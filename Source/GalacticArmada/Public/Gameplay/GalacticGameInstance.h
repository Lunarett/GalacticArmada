#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GalacticGameInstance.generated.h"

class AShipPawn;

UCLASS(Abstract)
class GALACTICARMADA_API UGalacticGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	TSubclassOf<AShipPawn> DefaultPlayerShipClass;

public:
	UFUNCTION(BlueprintCallable, Category = "Player")
	TSubclassOf<AShipPawn> GetDefaultPlayerShipClass() const { return DefaultPlayerShipClass; }

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetDefaultPlayerShipClass(const TSubclassOf<AShipPawn> NewClass) { DefaultPlayerShipClass = NewClass; }
};