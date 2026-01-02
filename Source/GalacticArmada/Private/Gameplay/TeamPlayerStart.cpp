#include "Gameplay/TeamPlayerStart.h"

FGenericTeamId ATeamPlayerStart::GetGenericTeamId() const
{
	return FGenericTeamId(TeamId);
}

uint8 ATeamPlayerStart::GetTeamId() const
{
	return TeamId;
}

bool ATeamPlayerStart::IsPlayerOnly() const
{
	return bIsPlayerOnly;
}