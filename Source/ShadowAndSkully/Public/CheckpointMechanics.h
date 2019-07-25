// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"

#include "CheckpointMechanics.generated.h"


UCLASS()
class SHADOWANDSKULLY_API ACheckpointMechanics : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACheckpointMechanics();

	UPROPERTY(BlueprintReadWrite, SaveGame)
		bool bIsDestroyed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Checkpoint LevelDesign")
		FVector CollisionScale = FVector(64, 64, 128);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
