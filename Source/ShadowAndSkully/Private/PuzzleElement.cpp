///This Holds all of the Puzzle Element behaviors for the entire game 


#include "Public/PuzzleElement.h"
#include "Engine/Engine.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

#pragma region

#pragma region
// Sets default values
APuzzleElement::APuzzleElement()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//WidgetHandler = new FWidgetHandler();


}

// Called when the game starts or when spawned
void APuzzleElement::BeginPlay()
{
	Super::BeginPlay();
	
}

void APuzzleElement::UpdateUI()
{
	//Empty function. Used to update the UI elements every tick
#if WITH_EDITORONLY_DATA
	//WidgetHandler->ClearScreen(SelfHighlighter);
	//WidgetHandler->DrawToScreen(SelfHighlighter);
#endif
}



// Called every frame
void APuzzleElement::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

#pragma endregion Puzzle Element Functions


#pragma region 
APuzzleThinker::APuzzleThinker()
{
	PrimaryActorTick.bCanEverTick = true;
#if WITH_EDITORONLY_DATA
	PrimaryActorTick.bStartWithTickEnabled = true;
	USelection::SelectObjectEvent.AddUObject(this, &APuzzleThinker::OnSelectedInEditor);
#endif
}

void APuzzleThinker::OnSelectedInEditor(UObject* Object)
{
#if WITH_EDITORONLY_DATA
	if (Object == this || ActivateItems.Contains(Object))
	{
		bSelected = true;
	}
	else
	{
		bSelected = false;
	}
#endif
}

bool APuzzleThinker::ShouldTickIfViewportsOnly() const
{
	return true;
}


void APuzzleThinker::UseItem()
{
	
}



void APuzzleThinker::BeginPlay()
{
	Super::BeginPlay();

	//Updates the LinkedThinker list for all attached Doers
	for(auto Doer : ActivateItems)
	{
		if (Doer)
		{
			if (!Doer->LinkedThinkers.Contains(this))
			{
				Doer->LinkedThinkers.Add(this);
			}
		}
	}

#if WITH_EDITORONLY_DATA
	bSelected = false;
#endif

}
void APuzzleThinker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_EDITORONLY_DATA
	if(bSelected)
	{
		

		int NullItemCount = 0;
		for (auto Thinker : ActivateItems)
		{
			if (Thinker)
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					GetActorLocation(),
					Thinker->GetActorLocation(),
					ArrowSize,
					LineColor,
					false,
					-1,
					DrawPriority,
					LineThickness
					);
			}
			else
			{
				NullItemCount++;

				auto DrawPoint = GetActorLocation() + FVector(0, 0, (NullItemCount) * NullPointCounterScale);
				

				DrawDebugPoint(
					GetWorld(),
					DrawPoint,
					NullPointDrawSize,
					FColor::Red,
					false,
					-1,
					DrawPriority
				);

				
			}

		}
		if(NullItemCount > 0)
		{
			FString Message = "There are - ";
			Message.AppendInt(NullItemCount);
			Message += " - NULL Activation Items";

			GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, Message);
		}
	}
#endif
}
#pragma  endregion Puzzle Thinker Functions


#pragma region
APuzzleDoer::APuzzleDoer()
{
	PrimaryActorTick.bCanEverTick = true;
	if(State)
	{
		ActivateLever(State);
	}
}

void APuzzleDoer::BeginPlay()
{
	Super::BeginPlay();

	//Trims the Linked Thinkers list to remove null refs and outdated refs
	for(auto Thinker : LinkedThinkers)
	{
		if(Thinker)
		{
			if(!Thinker->ActivateItems.Contains(this))
			{
				LinkedThinkers.Remove(Thinker);
			}
		}
		else
		{
			LinkedThinkers.Remove(Thinker);
		}
	}
}

void APuzzleDoer::ActivateLever_Implementation(bool NewState)
{

}

void APuzzleDoer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
#pragma endregion Puzzle Doer Functions

#pragma endregion Parent Classes


#pragma region
ALever::ALever()
{

	PrimaryActorTick.bCanEverTick = true;

}
void ALever::UseItem_Implementation()
{
	State = !State;
	for (APuzzleDoer* obj : ActivateItems)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, obj->GetName());
		if (obj != nullptr)
		{
			obj->ActivateLever(State);
		}
	}
}

void ALever::BeginPlay()
{
	Super::BeginPlay();
}

void ALever::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
#pragma endregion Puzzle Lever Functions


#pragma region
ASoulPickup::ASoulPickup()
{

	PrimaryActorTick.bCanEverTick = true;

}
void ASoulPickup::UseItem_Implementation()
{
	for (auto Thinker : ActivateItems)
	{
		if (Thinker != nullptr)
		{
			Thinker->ActivateLever(true);
		}
	}
}

void ASoulPickup::BeginPlay()
{
	Super::BeginPlay();
}

void ASoulPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
#pragma endregion Soul Pickup Functions


#pragma region
// ---------Latched Pressure Plate --------------------
void ALatchedPressurePlate::UseItem_Implementation()
{
	State = !State;
	for(auto Item : ActivateItems)
	{
		if(Item)
		{
			Item->ActivateLever(State);
		}
	}
}

//-------------Paired Pressure Plate----------------

APairedPressurePlate::APairedPressurePlate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void APairedPressurePlate::CheckProperties()
{
	//Updates master and slave properties accordingly
	if (MasterPressurePlate == this)
	{
		MasterPressurePlate = nullptr;
	}
	if (SlavePressurePlate == this)
	{
		SlavePressurePlate = nullptr;
	}

	if (MasterPressurePlate != nullptr)
	{
		if (SlavePressurePlate != nullptr)
		{
			SlavePressurePlate = nullptr;
		}

		if (MasterPressurePlate->SlavePressurePlate != this)
		{
			MasterPressurePlate = nullptr;
		}
	}
}
#if WITH_EDITORONLY_DATA
void APairedPressurePlate::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	FName PropName;
	if(e.Property != nullptr)
	{
		PropName = e.Property->GetFName();
	}
	else
	{
		PropName = NAME_None;
	}

	if(PropName == GET_MEMBER_NAME_CHECKED(APairedPressurePlate, SlavePressurePlate))
	{
		//sets the slave's master to this
		if (SlavePressurePlate != this)
		{
			SlavePressurePlate->MasterPressurePlate = this;
		}
	}
	Super::PostEditChangeProperty(e);
}
#endif

void APairedPressurePlate::ResetSkullyTarget()
{

	Player = Cast<AShadowMechanics>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (Player != nullptr)
	{
		Player->Skully->TargetGoal = Player;
	}
}

void APairedPressurePlate::BeginPlay()
{
	Super::BeginPlay();

	Player = Cast<AShadowMechanics>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	CheckProperties();
	
}
void APairedPressurePlate::OnSelectedInEditor(UObject* Object)
{
#if WITH_EDITORONLY_DATA
	if (Object == this || ActivateItems.Contains(Object) || Object == SlavePressurePlate)
	{
		bSelected = true;
	}
	else
	{
		bSelected = false;
	}
#endif
}

void APairedPressurePlate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if(WITH_EDITORONLY_DATA)
	if(bSelected)
	{
		if(SlavePressurePlate)
		{
			//Only Calls on MASTER
			DrawDebugLine(
				GetWorld(),
				GetActorLocation(),
				SlavePressurePlate->GetActorLocation(),
				LineColor,
				false,
				-1,
				DrawPriority,
				LineThickness
			);
		}
	}
#endif
}

void APairedPressurePlate::UseItem_Implementation()
{
	Player = Cast<AShadowMechanics>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));//Re-finds player instance to update for spawned in player
	ItemState = true;

		//If this is a slave pressure plate
	if (MasterPressurePlate != nullptr)
	{
		if (Player != nullptr && OverlapedActor == Player)
		{
			Player->Skully->TargetGoal = MasterPressurePlate;
		}
		if (MasterPressurePlate->ItemState)
		{
			MasterPressurePlate->Activate();
		}

	}
		//if this is a master pressure plate
	else if (SlavePressurePlate != nullptr)
	{
		Player = Cast<AShadowMechanics>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (Player != nullptr && OverlapedActor == Player)
		{
			Player->Skully->TargetGoal = SlavePressurePlate;
		}
		if(SlavePressurePlate->ItemState)
		{
			Activate();
		}
	}


}
void APairedPressurePlate::StepOffPlate_Implementation()
{
	ItemState = false;
}

void APairedPressurePlate::Activate()
{
	//calls the default use item
	State = !State;
	for (APuzzleDoer* OBJ : ActivateItems)
	{
		if (OBJ != nullptr)
		{
			OBJ->ActivateLever(State);
		}
	}
}
#pragma endregion Pressure Plates Functions


