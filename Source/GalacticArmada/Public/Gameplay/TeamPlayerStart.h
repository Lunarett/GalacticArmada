#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "GenericTeamAgentInterface.h"
#include "TeamPlayerStart.generated.h"

/**
 * PlayerStart that is associated with a Generic Team ID.
 *
 * Designers place these in the map to define spawn locations per team.
 * - TeamId: which team this start belongs to (0..254). 255 is reserved for NoTeam.
 * - bIsPlayerOnly: if true, only human players may spawn here (AI will skip it).
 */
UCLASS()
class GALACTICARMADA_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	/** Returns the start's team as a Generic Team ID. */
	FGenericTeamId GetGenericTeamId() const;

	/** Returns the raw team id used in the editor (0..254). */
	uint8 GetTeamId() const;

	/** True if this start may be used only by human players. */
	bool IsPlayerOnly() const;

protected:
	/** If true, AI spawning will skip this start. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Team")
	bool bIsPlayerOnly = false;

	/**
	 * Team identifier for this start.
	 * Keep within 0..254 (255 is reserved for NoTeam).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Team", meta=(ClampMin="0", ClampMax="254"))
	uint8 TeamId = 0;
};