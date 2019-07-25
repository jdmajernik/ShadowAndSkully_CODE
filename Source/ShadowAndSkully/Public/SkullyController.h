// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SkullyController.generated.h"

/**
 * 
 */
UCLASS()
class SHADOWANDSKULLY_API ASkullyController : public AAIController
{
	GENERATED_BODY()
	void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "AI Data")
		AActor* TargetGoal;
};
