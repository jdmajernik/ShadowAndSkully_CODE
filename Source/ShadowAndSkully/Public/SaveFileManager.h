// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BufferArchive.h"
#include "ObjectAndNameAsStringProxyArchive.h"
#include "GameFramework/GameState.h"
#include "SaveFileManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCheckpointDebug, Log, All);

class APuzzleElement;
class ACheckpointMechanics;


/**
 * 
 */

USTRUCT()
struct FActorSpawnData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(VisibleAnywhere, SaveGame)
		UClass* ActorClass;

	UPROPERTY(VisibleAnywhere, SaveGame)
		FName ActorName;


	UPROPERTY(VisibleAnywhere, SaveGame)
		FTransform ActorTransform;

	UPROPERTY(VisibleAnywhere, SaveGame)
		FVector ActorScale = FVector(1,1,1);

	UPROPERTY(VisibleAnywhere, SaveGame)
		TArray<uint8> ActorData;

	FActorSpawnData()
	{
		ActorClass = nullptr;
	}

	bool Equals(FActorSpawnData Comparator)
	{
		if(ActorName.IsEqual(Comparator.ActorName) && ActorClass->GetName().Equals(Comparator.ActorClass->GetName()))
		{
			return true;
		}
		return false;
	}
};

USTRUCT(BlueprintType)
struct FMapSaveData
{
	//This Class lets me nest items in by map
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(VisibleAnywhere, SaveGame)
	TArray<FActorSpawnData> ObjectData;
	
};



struct FObjectSaveArchive : public FObjectAndNameAsStringProxyArchive
{
	FObjectSaveArchive(FArchive& InInnerArchive)
		:FObjectAndNameAsStringProxyArchive(InInnerArchive, false)
	{
		ArIsSaveGame = true;
		ArNoDelta = true;
	}
};

UCLASS()
class SHADOWANDSKULLY_API USaveFileManager : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		bool bShowDebug = false;
	UPROPERTY(EditAnywhere)
		int32 MaxSaveFileNameSize = 12;

	UPROPERTY(VisibleAnywhere)
	TMap<FString, FTransform> PlayerSpawnPoints; //The transform of the last checkpoint in the tagged map (non-repeating key)

	UPROPERTY(VisibleAnywhere, SaveGame)
	TMap<FString, FMapSaveData> MapSaveData; //All of the stored actor data for any map that this saves to

	TArray<AActor*> PreloadedActors; //Temporary array of spawned actors before serializing their data




	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame)
		FString LastMapName;

	UFUNCTION(BlueprintPure)
		FTransform GetPlayerSpawnPoint(AActor* Controller);
	UFUNCTION(BlueprintCallable)
		FTransform SetPlayerSpawnPoint(AActor* Controller, FTransform NewLocation);

	UFUNCTION(BlueprintCallable)
		void LoadGame(AActor* Controller);
	UFUNCTION(BlueprintCallable)
		void SaveGame(AActor* Controller, FString SlotName);

	UFUNCTION(BlueprintCallable)
		void SetSaveName(FString NewFileName);
	UFUNCTION(BlueprintCallable)
		FString GetSaveName();

protected:
	void PreloadObjects(AActor* WorldActor);
	void LoadActorData(AActor* WorldActor);

	void ObjectSaver(UObject* SavedObject, FActorSpawnData& SpawnData);
	void ObjectLoader(UObject* LoadObject, UPARAM(ref) TArray<uint8>& ObjectData);

	UPROPERTY(VisibleAnywhere, SaveGame)
		FString SaveFileName;

	UPROPERTY()
		float DebugDisplayTime = 15;
};
