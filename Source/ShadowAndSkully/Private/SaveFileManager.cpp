// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveFileManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "CheckpointMechanics.h"
#include "PuzzleElement.h"
#include "EngineUtils.h"
#include "BufferArchive.h"
#include "MemoryReader.h"
#include "BadSoulMechanics.h"
#include "DynaimicCameraController.h"
#include "SkullyMechanics.h"

DEFINE_LOG_CATEGORY(LogCheckpointDebug);

FTransform USaveFileManager::GetPlayerSpawnPoint(AActor* Controller)
{
	auto const ThisMap = Controller->GetWorld()->GetMapName();
	
	if(PlayerSpawnPoints.Contains(ThisMap))
	{
		return PlayerSpawnPoints[ThisMap];
	}
	else
	{
		//If the save point doesn't exist, Set it to the player's location
		auto PlayerStart = Controller->GetTransform();

		PlayerSpawnPoints.Add(ThisMap, PlayerStart);
		return PlayerStart;
	}
}

FTransform USaveFileManager::SetPlayerSpawnPoint(AActor* Controller, FTransform NewLocation)
{
	auto const ThisMap = Controller->GetWorld()->GetMapName();

	if(PlayerSpawnPoints.Contains(ThisMap))
	{
		PlayerSpawnPoints[ThisMap] = NewLocation;
	}
	else
	{
		PlayerSpawnPoints.Add(ThisMap, NewLocation);
	}

	return PlayerSpawnPoints[ThisMap];
}

void USaveFileManager::LoadGame(AActor* Controller)
{
	UE_LOG(LogCheckpointDebug, Display, TEXT("HelloWorld"));
	FString Message;
	int DisplayCount = -1;

	if (bShowDebug && GEngine)
	if (bShowDebug && GEngine)
	{
		Message = ("------------------\nCurrent Map  ->  ");
		Message.Append(Controller->GetWorld()->GetMapName());
		Message.Append("\n------------------");

		GEngine->AddOnScreenDebugMessage(--DisplayCount, DebugDisplayTime, FColor::Green, Message);

		Message = ("Found Map Saves\n\n");
		for (auto MapSave : MapSaveData)
		{
			Message.Append("-> ");
			Message.Append(MapSave.Key);
			Message.Append("\n");
		}
		Message.Append("\n-----------------------------");
		GEngine->AddOnScreenDebugMessage(--DisplayCount, DebugDisplayTime, FColor::Yellow, Message);
	}

	//Sanitize on read
	if (MapSaveData.Contains(Controller->GetWorld()->GetMapName()))
	{
		//Debug Items
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(--DisplayCount, DebugDisplayTime, FColor::Green, "-----------------------------\nThere is data for this map!\n-----------------------------");

			Message = "Found - ";
			Message.Append(FString::FromInt(MapSaveData[Controller->GetWorld()->GetMapName()].ObjectData.Num()));
			Message.Append(" Items!");

			GEngine->AddOnScreenDebugMessage(--DisplayCount, DebugDisplayTime, FColor::Green, Message);
		}


		//Loads in Actors
		PreloadObjects(Controller);

		//Assigns Data to them
		LoadActorData(Controller);
	}
	else if(bShowDebug)
	{
		GEngine->AddOnScreenDebugMessage(--DisplayCount, DebugDisplayTime, FColor::Red, "There are no items in the Save Data");
	}
}

void USaveFileManager::SaveGame(AActor* Controller, FString SlotName)
{
	GEngine->AddOnScreenDebugMessage(-1, DebugDisplayTime, FColor::Green, "Trying to save to -> " + SlotName);
	
	
	FActorSpawnData ActorData;

	TArray<AActor*> AllFoundActors;

	TArray<AActor*> TempFoundActors;

	//This finds all actors

	UGameplayStatics::GetAllActorsOfClass(Controller->GetWorld(), ACheckpointMechanics::StaticClass(), TempFoundActors);
	AllFoundActors += TempFoundActors;

	UGameplayStatics::GetAllActorsOfClass(Controller->GetWorld(), APuzzleElement::StaticClass(), TempFoundActors);
	AllFoundActors += TempFoundActors;

	UGameplayStatics::GetAllActorsOfClass(Controller->GetWorld(), AShadowMechanics::StaticClass(), TempFoundActors);
	AllFoundActors += TempFoundActors;

	UGameplayStatics::GetAllActorsOfClass(Controller->GetWorld(), ABadSoulMechanics::StaticClass(), TempFoundActors);
	AllFoundActors += TempFoundActors;

	UGameplayStatics::GetAllActorsOfClass(Controller->GetWorld(), ADynamicCameraController::StaticClass(), TempFoundActors);
	AllFoundActors += TempFoundActors;


//	UGameplayStatics::GetAllActorsOfClass(Controller->GetWorld(), AActor::StaticClass(), AllFoundActors);

	if(!MapSaveData.Contains(Controller->GetWorld()->GetMapName()))
	{
		MapSaveData.Emplace(Controller->GetWorld()->GetMapName());
	}

	if (AllFoundActors.Num() > 0)
	{
		TArray<FActorSpawnData>& MapActorData = MapSaveData[Controller->GetWorld()->GetMapName()].ObjectData;
		MapActorData.Empty();
		for (auto Actor : AllFoundActors)
		{

			//Finds Each Object in the Scene and saves it to this object
			ObjectSaver(Actor, ActorData);


			MapActorData.Emplace(ActorData);
		}



	}
	//Saves the data
	if (UGameplayStatics::SaveGameToSlot(this, SlotName, 0))
	{
		//USaveFileManager* CheckSave = Cast<USaveFileManager>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));

		//GEngine->AddOnScreenDebugMessage(-1, DebugDisplayTime, FColor::Cyan, FString::FromInt(CheckSave->MapSaveData[Controller->GetWorld()->GetMapName()].ObjectData.Num()));

		//for (auto DebugData : CheckSave->MapSaveData[Controller->GetWorld()->GetMapName()].ObjectData)
		//{
		//	if (DebugData.Equals(ActorData))
		//	{
		//		GEngine->AddOnScreenDebugMessage(-1, DebugDisplayTime, FColor::Cyan, DebugData.ActorName.ToString());
		//	}
		//}

	}

	//ThisMapData.CheckpointStatus = CheckpointData;
	//ThisMapData.PuzzleState = PuzzleData;
}

void USaveFileManager::SetSaveName(FString NewFileName)
{
	FString TrimmedName = "";

	//Iterates through all elements in the string and adds it to TrimmedName
	for(int i = 0; i < (NewFileName.Len() > MaxSaveFileNameSize ? MaxSaveFileNameSize : NewFileName.Len()); i++)
	{
		TrimmedName += NewFileName[i];
	}

	SaveFileName = TrimmedName;
}

FString USaveFileManager::GetSaveName()
{
	return SaveFileName;
}

void USaveFileManager::PreloadObjects(AActor* WorldActor)
{
	if (MapSaveData.Contains(WorldActor->GetWorld()->GetMapName()))
	{
		TArray<AActor*> AllFoundActors;
		TArray<AActor*> TempFoundActors;
		TMap<FString, AActor*> FoundEnemyNames;
		
		UGameplayStatics::GetAllActorsOfClass(WorldActor->GetWorld(), ACheckpointMechanics::StaticClass(), TempFoundActors);
		AllFoundActors += TempFoundActors;

		UGameplayStatics::GetAllActorsOfClass(WorldActor->GetWorld(), APuzzleElement::StaticClass(), TempFoundActors);
		AllFoundActors += TempFoundActors;

		UGameplayStatics::GetAllActorsOfClass(WorldActor->GetWorld(), AShadowMechanics::StaticClass(), TempFoundActors);
		AllFoundActors += TempFoundActors;

		UGameplayStatics::GetAllActorsOfClass(WorldActor->GetWorld(), ABadSoulMechanics::StaticClass(), TempFoundActors);
		AllFoundActors += TempFoundActors;

		UGameplayStatics::GetAllActorsOfClass(WorldActor->GetWorld(), ADynamicCameraController::StaticClass(), TempFoundActors);
		AllFoundActors += TempFoundActors;
		
		for(auto Actor : AllFoundActors)
		{
			Actor->Destroy();
			//FoundEnemyNames.Add(Enemy->GetName(), Enemy);
		}

		for (auto LoadData : MapSaveData[WorldActor->GetWorld()->GetMapName()].ObjectData)
		{
			//Spawns In all valid Actor classes
			if (LoadData.ActorClass)
			{
				UE_LOG(LogCheckpointDebug, Display, TEXT("Loading In actor %s"), *(LoadData.ActorName).ToString());
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.Name = LoadData.ActorName;

				if(FoundEnemyNames.Contains((LoadData.ActorName).ToString()))
				{
					FoundEnemyNames.Remove(LoadData.ActorName.ToString());
				}

				AActor* NewActor;

				if(Cast<AShadowMechanics>(LoadData.ActorClass) && PlayerSpawnPoints.Contains(WorldActor->GetWorld()->GetMapName()))
				{
					//if the actor is the player, set the transform to the spawn point instead
					NewActor = WorldActor->GetWorld()->SpawnActor<AActor>(LoadData.ActorClass, PlayerSpawnPoints[WorldActor->GetWorld()->GetMapName()], SpawnParameters);
				}
				else
				{
					NewActor = WorldActor->GetWorld()->SpawnActor<AActor>(LoadData.ActorClass, LoadData.ActorTransform, SpawnParameters);
					NewActor->SetActorScale3D(LoadData.ActorScale);
				}

				PreloadedActors.Add(NewActor);
			}
		}

		for(auto RemainingEnemies : FoundEnemyNames)
		{
			//Destroys non-saved enemies
			auto PawnEnemy = Cast<ACharacter>(RemainingEnemies.Value);
			if(PawnEnemy)
			{
				//Destroys the controller to stop crashes
				PawnEnemy->GetController()->Destroy();
			}
			RemainingEnemies.Value->Destroy();
		}
	}
}

void USaveFileManager::LoadActorData(AActor* WorldActor)
{
	if (MapSaveData.Contains(WorldActor->GetWorld()->GetMapName()))
	{
		auto SaveObjectData = MapSaveData[WorldActor->GetWorld()->GetMapName()].ObjectData;
		//This loads all of the data to the actors
		for (int i = 0; i < SaveObjectData.Num(); i++)
		{
			//Checks if the Data is not NULL
			if (SaveObjectData[i].ActorClass)
			{
				//tries to access actor
				if (PreloadedActors[i])
				{
					UE_LOG(LogCheckpointDebug, Display, TEXT("Loading in Object Data"));
					ObjectLoader(PreloadedActors[i], SaveObjectData[i].ActorData);
				}
			}
			else
			{
				UE_LOG(LogCheckpointDebug, Warning, TEXT("Accessed NULL in Actor Data"));
			}
		}
	}
}

void USaveFileManager::ObjectSaver(UObject* SavedObject, FActorSpawnData& SpawnData)
{
	SpawnData.ActorClass = SavedObject->GetClass();
	SpawnData.ActorName = FName(*SavedObject->GetName());
	
	if (Cast<AActor>(SavedObject))
	{
		SpawnData.ActorTransform = Cast<AActor>(SavedObject)->GetTransform();
		SpawnData.ActorScale = Cast<AActor>(SavedObject)->GetActorScale();
	}

	//Writes UPROPERTY variables to memory
	if(SavedObject)
	{
		FMemoryWriter Writer(SpawnData.ActorData, true);

		FObjectSaveArchive MyArchive(Writer);

		SavedObject->Serialize(MyArchive);
	}
	
}

void USaveFileManager::ObjectLoader(UObject* LoadObject, UPARAM(ref) TArray<uint8>& ObjectData)
{
	//Loads UPROPERTY variables to memory
	if(ObjectData.Num()>0)
	{

		FMemoryReader Reader(ObjectData, true);

		FObjectSaveArchive MyArchive(Reader);

		LoadObject->Serialize(MyArchive);
	}

}
