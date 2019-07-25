// Fill out your copyright notice in the Description page of Project Settings.


#include "SkullyController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "SkullyMechanics.h"

void ASkullyController::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundControllers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASkullyController::StaticClass(), FoundControllers);
	if(FoundControllers.Num()>1)
	{
		Destroy();
	}
	else
	{
		TArray<AActor*> FoundSkullys;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASkullyMechanics::StaticClass(), FoundSkullys);

		auto Skully = Cast<ASkullyMechanics>(FoundSkullys[0]);
		if(Skully)
		{
			Possess(Skully);
		}
	}
}
