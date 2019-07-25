// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
class APuzzleThinker;

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "SkullyController.h"

#include "ShadowMechanics.generated.h"


UCLASS()
class SHADOWANDSKULLY_API AShadowMechanics : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AShadowMechanics();

	UFUNCTION()
		void FindGoals();

	UFUNCTION()
		void SortGoals();

	UPROPERTY(VisibleAnywhere, Category = "Camera System")
		TArray<APuzzleThinker*> Goals;
	UPROPERTY(EditAnywhere, Category = "Player Data")
		ASkullyController* Skully;
	UPROPERTY(EditDefaultsOnly, Category = "Camera System")
		float MinZoom;
	UPROPERTY(EditDefaultsOnly, Category = "Camera System")
		float MaxZoom;
	UPROPERTY(EditDefaultsOnly, Category = "Camera System")
		float MaxDistance = 800;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Camera System")
		UCameraComponent* Camera;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
};
