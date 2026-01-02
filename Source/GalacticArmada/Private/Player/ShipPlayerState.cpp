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