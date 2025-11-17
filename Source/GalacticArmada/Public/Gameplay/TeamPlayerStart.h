#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TeamPlayerStart.generated.h"

UCLASS()
class GALACTICARMADA_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Team")
	bool bIsPlayerOnly = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team")
	int32 TeamID;

public:
	FORCEINLINE int32 GetTeamID() const { return TeamID; }
	FORCEINLINE bool GetIsPlayerOnly() const { return bIsPlayerOnly; }
};
