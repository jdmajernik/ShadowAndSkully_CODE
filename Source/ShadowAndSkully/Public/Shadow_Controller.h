// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShadowMechanics.h"
#include "SkullyMechanics.h"
#include "SaveFileManager.h"
#include "Shadow_Controller.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class SHADOWANDSKULLY_API AShadow_Controller : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
		AActor* SkullyActorRef;
	UPROPERTY(BlueprintReadWrite)
		AActor* ShadowActorRef;
	UPROPERTY(BlueprintReadWrite)
		ASkullyMechanics* SkullyRef;
	UPROPERTY(BlueprintReadWrite)
		AShadowMechanics* ShadowRef;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
		USaveFileManager* SaveData;
};
