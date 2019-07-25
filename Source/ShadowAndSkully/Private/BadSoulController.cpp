// Fill out your copyright notice in the Description page of Project Settings.


#include "BadSoulController.h"
#include "Kismet/GameplayStatics.h"
#include "BadSoulMechanics.h"
#include "TimerManager.h"


void ABadSoulController::OnPawnSpawn(APawn* PawnReference)
{
	Possess(PawnReference);
}
