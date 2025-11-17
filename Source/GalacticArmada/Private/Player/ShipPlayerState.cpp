// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/ShipPlayerState.h"

void AShipPlayerState::AddKill()
{
	Kills++;
}

void AShipPlayerState::AddDeath()
{
	Deaths++;
}

void AShipPlayerState::SetTeamID(const int32 InTeamID)
{
	TeamID = InTeamID;
}