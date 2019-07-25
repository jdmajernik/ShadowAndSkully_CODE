// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PuzzleElement.h"
#include "MazeNode.generated.h"

/**
 * 
 */
UCLASS()
class SHADOWANDSKULLY_API AMazeNode : public APuzzleDoer
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, SaveGame, EditAnywhere, Category = "Maze Node")
		FRotator DefaultState;
	UPROPERTY(BlueprintReadWrite, SaveGame, EditAnywhere, Category = "Maze Node")
		FRotator ActiveState;

	void BeginPlay() override;
protected:
	void ActivateLever_Implementation(bool NewState) override;
	void Tick(float DeltaSeconds) override;
};
