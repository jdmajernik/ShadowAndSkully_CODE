/**
	John Majernik 
	Skully Mechanics

	This controlls most, if not all, Skully AI functions. It operates primarily through a tick-based state system.
	Most of the relevant variables can be modified in-editor to better tweak the AI to match the gameplay.
*/
#include "SkullyMechanics.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"
#include "Runtime/NavigationSystem/Public/NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "PuzzleElement.h"
#include "BadSoulMechanics.h"


#pragma region
// Sets default values
ASkullyMechanics::ASkullyMechanics()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASkullyMechanics::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle UnusedHandle;
	GetWorldTimerManager().SetTimer(UnusedHandle, this, &ASkullyMechanics::Awake, 0.2);

	//safe to call since player spawns skully
	CurrentState = ESkullyStates::Sk_Default;
	bCanTick = false;
}

void ASkullyMechanics::Awake_Implementation()
{
	//Waits until game is set up to start pulling variables 


	SkullyController = Cast<ASkullyController>(GetController());

	//Sets Shadow as the target goal
	SkullyController->TargetGoal = GetWorld()->GetFirstPlayerController()->GetPawn();

	Shadow = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	//Populates a master list of every Thinker element in the level
	ThinkerMasterList.Empty();
	for (TActorIterator<APuzzleThinker> ThinkerItr(GetWorld()); ThinkerItr; ++ThinkerItr)
	{
		ThinkerMasterList.Add(*ThinkerItr);
	}
	

	bCanTick = true;
}
#pragma endregion Begin Functions






#pragma region
void ASkullyMechanics::DefaultTick()
{
	static FTimerHandle GoalHandle;
	if(bCheckGoal)
	{
		//Sets a timer to start finding close goal objects (doesn't need to be continuously updated)
		GetWorld()->GetTimerManager().SetTimer(GoalHandle, this, &ASkullyMechanics::FindNextGoal, GoalRefreshRate);
		bCheckGoal = false;
	}
	
	if(bFindingGoalPaths)
	{
		GoalPathListener();
	}
	else if (bFindingThinkerPaths)
	{
		ThinkerPathListener();
	}
	if (NextGoalThinkers.Num() > 0)
	{
		//Finds Distance to the Next GoalThinker
		for(auto TargetedThinker : NextGoalThinkers)
		{
			if(FVector::Dist(GetActorLocation(), TargetedThinker->GetActorLocation()) < AutoActivationDistance)
			{
				//Checks if the pressure plate is outside the wait distance or if it's not a pressure plate
				if (!Cast<APressurePlate>(TargetedThinker) || FVector::Dist(GetActorLocation(), Shadow->GetActorLocation()) <= PressurePlateWaitDistance)
				{
					UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

					auto ThinkerPath = NavSys->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), TargetedThinker->GetActorLocation());

					if (ThinkerPath)
					{
						//Checks that the path itself is within the activation distance
						if (ThinkerPath->GetPathLength() < AutoActivationDistance)
						{
							//Any Thinkers with activation parameters should go HERE
							if (Cast<ASoulRecepticle>(TargetedThinker) && Souls <= 0)
							{
								//Do Nothing if skully cannot activate receptacle 
								break;
							}

							//Sets the look-at point to the thinker, this is so the player knows which item skully is targeting
							LookAtTarget = TargetedThinker->GetActorLocation();

							//Sets goal variables
							SkullyController->TargetGoal = TargetedThinker;
							AutoActivatedThinker = TargetedThinker;

							bSeekingThinker = true;

							//Cleans out the Look for array since a Thinker has been found
							//NextGoalThinkers.Empty();
							break;
						}
					}
				}
			}
		}
	}


	//Moves to Target
	if(Cast<APuzzleElement>(SkullyController -> TargetGoal))
	{
		//Target is puzzle element and needs smaller Acceptance Radius
		SkullyController->MoveToActor(SkullyController->TargetGoal, DistToItem);
		
	}
	else
	{
		//Target is not a puzzle object, calls simple move to for existing distance calculations
		SimpleMoveToTarget();
	}

	if(bSeekingThinker && FVector::Dist(GetActorLocation(), SkullyController->TargetGoal->GetActorLocation()) < ItemActivationDistance)
	{
		if (!Cast<APressurePlate>(AutoActivatedThinker))
		{
			//Activates Item and resets target to shadow if its not a pressure plate
			if(Cast<ASoulRecepticle>(AutoActivatedThinker) && Souls == 0)
			{
				return;
			}
			else if(Cast<ASoulRecepticle>(AutoActivatedThinker) && !AutoActivatedThinker->State)
			{
				Souls--;
			}
			AutoActivatedThinker->UseItem();
			bSeekingThinker = false;
			SkullyController->TargetGoal = Shadow;

			BestItemPath = nullptr;

			NextGoalThinkers.Empty();

			bCheckGoal = false;

			FTimerHandle UnusuedHandle;
			GetWorld()->GetTimerManager().SetTimer(UnusuedHandle, this, &ASkullyMechanics::ResetVars, AutoActivationCooldown);
		}
		else
		{
			if(FVector::Dist(GetActorLocation(), SkullyController->TargetGoal->GetActorLocation()) < 10)
			{
				//If standing on a pressure plate, wait
				CurrentState = ESkullyStates::Sk_Default_Wait;
			}
		}
	}
}

void ASkullyMechanics::FindNextGoal()
{
	//Resets variables
	GoalsFound = 0;
	GoalPaths = 0;

	GoalThinkersFound = 0;
	GoalThinkerPaths = 0;

	NextGoals.Empty();
	PathedGoalThinkers.Empty();
	NextGoalThinkers.Empty();
	FoundItemThinkers.Empty();
	
//I'm basically copy-pasting these, but why wouldn't I, if they're duplicates (of sorts)

//	------------------------
//	| NAVIGATION VARIABLES |
//	------------------------

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = NavSys->MainNavData;


	FNavAgentProperties NavAgentProperties;
	NavAgentProperties.AgentHeight = SkullyAgentHeight;
	NavAgentProperties.AgentRadius = SkullyAgentRadius;
	NavAgentProperties.bCanFly = bCanSkullyFly;
	NavAgentProperties.bCanWalk = bCanSkullyWalk;

	FPathFindingQuery NavParams;
	NavParams.StartLocation = GetActorLocation();
	NavParams.QueryFilter = UNavigationQueryFilter::GetQueryFilter<UNavigationQueryFilter>(*NavData);

	FNavPathQueryDelegate NavDelegate;
	NavDelegate.BindUObject(this, &ASkullyMechanics::OnNextGoalFound); //Triggers OnItemPathFound when path is found


//	--------------------------
//	| SPHERE TRACE VARIABLES |
//	--------------------------
	TArray<FHitResult> FoundGoalItems;//All hit actors

	//Looking for every kind of goal,regardless of Collision Type (because Skully isn't directly interacting with the goal)
	FCollisionObjectQueryParams FindItemTypes;//Channels to Search for
	FindItemTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);//Static Objects
	FindItemTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel1);//'Normal' Objects
	FindItemTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel2);//'Spirit' Objects

	FCollisionQueryParams FindItemParams;//Ignored Actors
	FindItemParams.AddIgnoredActor(this);
	FindItemParams.AddIgnoredActor(Shadow);

	//Finds all goal objects in the specified radius
	GetWorld()->SweepMultiByObjectType(
		FoundGoalItems,
		GetActorLocation(),
		GetActorForwardVector() + GetActorLocation(),
		GetActorRotation().Quaternion(),
		FindItemTypes,
		FCollisionShape::MakeSphere(GoalSearchRadius),
		FindItemParams
	);

	for(auto FoundGoalHit : FoundGoalItems)
	{
		auto FoundGoal = Cast<APuzzleDoer>(FoundGoalHit.Actor);

		if(FoundGoal != nullptr)
		{
			NavParams.EndLocation = FoundGoal->GetActorLocation();

			GoalsFound++;
			PathedGoals.Add(
				NavSys->FindPathAsync(NavAgentProperties, NavParams, NavDelegate), //The Navigation path ID
				FoundGoal); //The Goal Actor reference
		}
	}

	//Sets Latch for GoalPathListener
	bFindingGoalPaths = true;
	bFindingThinkerPaths = true;
}

void ASkullyMechanics::GoalPathListener()
{
	if (GoalPaths < GoalsFound)
	{
		//await all paths to finish their async functions
		return;
	}

	

	//Sets up AsyncPath to the Thinkers to check if auto-activation would be valid
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = NavSys->MainNavData;


	FNavAgentProperties NavAgentProperties;
	NavAgentProperties.AgentHeight = SkullyAgentHeight;
	NavAgentProperties.AgentRadius = SkullyAgentRadius;
	NavAgentProperties.bCanFly = bCanSkullyFly;
	NavAgentProperties.bCanWalk = bCanSkullyWalk;

	FPathFindingQuery NavParams;
	NavParams.StartLocation = GetActorLocation();
	NavParams.QueryFilter = UNavigationQueryFilter::GetQueryFilter<UNavigationQueryFilter>(*NavData);

	FNavPathQueryDelegate NavDelegate;
	NavDelegate.BindUObject(this, &ASkullyMechanics::OnThinkerPathFound); //Triggers OnItemPathFound when path is found

	if (NextGoals.Num() > 0)
	{
		for (auto Goal : NextGoals)
		{
			for(auto Thinker : Goal->LinkedThinkers)
			{
				if (Thinker)//Check if valid
				{
					//goes through each thinker and finds the ones associated with this goal
					if (!NextGoalThinkers.Contains(Thinker))
					{
						UE_LOG(LogScript, Warning, TEXT("Starting Path Check for linked Thinker"));

						NavParams.EndLocation = Thinker->GetActorLocation();

						//Finds a path to the object and logs the path ID in a map with an object reference
						PathedGoalThinkers.Add(NavSys->FindPathAsync(NavAgentProperties, NavParams, NavDelegate), Thinker);
						GoalThinkerPaths++;

						auto PairedPressureThinker = Cast<APairedPressurePlate>(Thinker);
						if (PairedPressureThinker)
						{
							//Finds either the master or slave plate for the pressure plate
							auto PairedPlate = PairedPressureThinker->MasterPressurePlate ? PairedPressureThinker->MasterPressurePlate : PairedPressureThinker->SlavePressurePlate;

							NavParams.EndLocation = PairedPlate->GetActorLocation();

							//adds the linked pressure plate to the list of activation items
							PathedGoalThinkers.Add(
								NavSys->FindPathAsync(NavAgentProperties, NavParams, NavDelegate),
								PairedPlate
							);
							GoalThinkerPaths++;
						}
					}
				}
			}
		}
	}
	else
	{
		//Triggers function reset
		bFindingThinkerPaths = false;
	}

	bFindingGoalPaths = false;
}
	
	

void ASkullyMechanics::ThinkerPathListener()
{
	if (GoalThinkerPaths < GoalThinkersFound)
	{
		//await return of paths
		return;
	}

	TMap<APuzzleThinker*, float> ModifiedItemDistances;
	for(auto FoundElement : FoundItemThinkers)
	{
		auto PathLength = FoundElement.Value;
		for(auto priority : FoundItemPriority)
		{
			//Removes div by zero error
			if(FoundElement.Key->IsA(priority.Key) && priority.Value != 0)
			{
				//Modifies the path length based on priority
				PathLength = PathLength / priority.Value;
			}
		}

		ModifiedItemDistances.Add(FoundElement.Key, PathLength);
	}
	

	//Sorts the array based on the modified Distance
	ModifiedItemDistances.ValueSort([](const float& LHS, const float& RHS) {return LHS < RHS; });


	ModifiedItemDistances.GenerateKeyArray(NextGoalThinkers);

	//Resets Latches
	bCheckGoal = true; //Waits until async tasks are done to start new ones
	bFindingGoalPaths = false;
	bFindingThinkerPaths = false;

}


void ASkullyMechanics::OnNextGoalFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr)
{
	GoalPaths++;
	
	if(aResultType == ENavigationQueryResult::Success)
	{
		if(FVector::Dist(aNavPtr->GetDestinationLocation(), PathedGoals[aPathId]->GetActorLocation()) <= MinDistanceToGoal)
		{
			//If the path is Valid (Doesn't end at a wall or something)
			if(!PathedGoals[aPathId]->State)
			{
				//If the goal has yet to be activated, add it to the list
				NextGoals.Add(PathedGoals[aPathId]);
				
			}
		}
	}
}

void ASkullyMechanics::OnThinkerPathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType,
	FNavPathSharedPtr aNavPtr)
{
	UE_LOG(LogScript, Warning, TEXT("Found a path to a thinker"));
	GoalThinkersFound++;
	if (aResultType == ENavigationQueryResult::Success)
	{
		if ((FVector::Dist(aNavPtr->GetDestinationLocation(), PathedGoalThinkers[aPathId]->GetActorLocation()) <= ItemActivationDistance) && !FoundItemThinkers.Contains(PathedGoalThinkers[aPathId]))
		{
			//Checks if the path distance is valid and, if it is, adds it to the list
			FoundItemThinkers.Add(PathedGoalThinkers[aPathId], aNavPtr->GetLength());
		}
	}
}
#pragma endregion Default Functions

//Wait function (it's just the one, no region needed)
void ASkullyMechanics::SkullyWait()
{
	//Stops movement and waits for State Change
	bCanTick = false;
	

	//Sets the look at target to the Controller target
	if(SkullyController && SkullyController->TargetGoal)
	{
		SkullyController->StopMovement();
		LookAtTarget = SkullyController->TargetGoal->GetActorLocation();
	}
}

#pragma region 
void ASkullyMechanics::ActivateItem()
{
	//Sets up paths
	if(bFindItemLatch)
	{
		FindPuzzleItems();
	}
	//Listens for them to complete
	else if(bPathItemLatch)
	{
		if(GEngine && bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Orange, "Waiting on Paths");
		}
		FindItemPathsListener();
	}
	//Moves to the closest Thinker with a path to it
	else
	{
		if (!ClosestElement)
		{
			//If no paths were found, exits Activate state
			CurrentState = ESkullyStates::Sk_Default;

			bFindItemLatch = true;
		}
		else
		{
			//Updates Skully to look at the path to the puzzle element
			if (BestItemPath && FVector::Dist(GetActorLocation(), ClosestElement->GetActorLocation()) < ItemActivationDistance && BestItemPath->GetPathPoints().Num() > 0)
			{
				LookAtTarget = BestItemPath->GetPathPoints()[1].Location;
			}

			if (FVector::Dist(ClosestElement->GetActorLocation(), GetActorLocation()) < ItemActivationDistance && !Cast<APressurePlate>(ClosestElement))
			{
				
				auto ItemThinker = Cast<APuzzleThinker>(ClosestElement);
				//Activates the Item if close enough
				ItemThinker->UseItem();

				if(Cast<ASoulPickup>(ClosestElement))
				{
					//if the item is a soul, add one to 'souls' upon activation
					Souls++;
				}
				else if(Cast<ASoulRecepticle>(ClosestElement) && ClosestElement->State)
				{
					Souls--;
				}

				//Resets variables Post-activation
				SkullyController->TargetGoal = Shadow;
				CurrentState = ESkullyStates::Sk_Default;
				bFindItemLatch = true;

				BestItemPath = nullptr;

				if (GEngine && bShowDebug)
				{
					GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Green, "Activating Item!");
				}
			}
			else
			{
				//Disregards 'Activation' if the element is a pressure plate


				//Refreshes path continuously
				SkullyController->MoveToLocation(ClosestElement->GetActorLocation(), DistToItem);

				
				if (GEngine && bShowDebug)
				{
					auto const DebugDist = FVector::Dist(GetActorLocation(), ClosestElement->GetActorLocation());
					GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Orange, ClosestElement->GetName());
					GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Orange, FString::SanitizeFloat(DebugDist));
				}
			}
		}

	}
}

void ASkullyMechanics::FindPuzzleItems()
{
//Finds all Puzzle Thinkers in a set radius

	bFindItemLatch = false; //locks this to only happen once

	//Resets variables
	FoundItemCount = 0;
	PathsFound = 0;
	ShortestThinkerPathDist = 0;
	bPathItemLatch = true;

	ClosestElement = nullptr;
	PathedElements.Empty(); //Empties directory before population
	FoundItemPaths.Empty();

//	------------------------
//	| NAVIGATION VARIABLES |
//	------------------------

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = NavSys->MainNavData;


	FNavAgentProperties NavAgentProperties;
	NavAgentProperties.AgentHeight = SkullyAgentHeight;
	NavAgentProperties.AgentRadius = SkullyAgentRadius;
	NavAgentProperties.bCanFly = bCanSkullyFly;
	NavAgentProperties.bCanWalk = bCanSkullyWalk;


	FPathFindingQuery NavParams;
	NavParams.StartLocation = ActivationStartPoint;
	NavParams.QueryFilter = UNavigationQueryFilter::GetQueryFilter<UNavigationQueryFilter>(*NavData);

	FNavPathQueryDelegate NavDelegate;
	NavDelegate.BindUObject(this, &ASkullyMechanics::OnItemPathFound); //Triggers OnItemPathFound when path is found


//	--------------------------
//	| SPHERE TRACE VARIABLES |
//	--------------------------
	TArray<FHitResult> FoundPuzzleItems;//All hit actors

	FCollisionObjectQueryParams FindItemTypes;//Channels to Search for
	FindItemTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
	FindItemTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel1);//'Normal' Objects

	FCollisionQueryParams FindItemParams;//Ignored Actors
	FindItemParams.AddIgnoredActor(this);
	FindItemParams.AddIgnoredActor(Shadow);

	//Finds items in a radius
	GetWorld()->SweepMultiByObjectType(
		FoundPuzzleItems,
		GetActorLocation(),
		GetActorForwardVector()+ GetActorLocation(),
		GetActorRotation().Quaternion(),
		FindItemTypes,
		FCollisionShape::MakeSphere(ItemCheckRange),
		FindItemParams
	);

	if (bShowDebug)
	{
		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			ItemCheckRange,
			10,
			FColor::Orange,
			false,
			5);
	}

	

	for(auto FoundItem : FoundPuzzleItems)
	{
		//Finds all of the Thinkers
		auto PuzzleItem = Cast<APuzzleThinker>(FoundItem.Actor);

		if(PuzzleItem != nullptr)
		{
			if (bShowDebug)
			{
				DrawDebugPoint(
					GetWorld(),
					FoundItem.ImpactPoint,
					10,
					FColor::Green,
					false,
					5
				);
			}
			//If the item is an actor
			FoundItemCount++; //The number of paths generated to check against

			NavParams.EndLocation = FoundItem.ImpactPoint;
			PathedElements.Add(NavSys->FindPathAsync(NavAgentProperties, NavParams, NavDelegate), PuzzleItem); //Adds in the item reference to a directory with the Path ID as the key
		}
		else
		{
			if (bShowDebug)
			{
				DrawDebugPoint(
					GetWorld(),
					FoundItem.ImpactPoint,
					10,
					FColor::Red,
					false,
					5
				);
			}
		}
		

	}


}

void ASkullyMechanics::OnItemPathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr)
{
	//Increments Path count
	PathsFound++;

	//DebugDraw variables
	bool bIsFirstTime = true;
	FNavPathPoint LastPoint;

	if(aResultType == ENavigationQueryResult::Success)
	{

		for(auto Point : aNavPtr->GetPathPoints())
		{
			//Debug Function to Display the paths
			if(bShowDebug)
			{
				if(Point.Location != aNavPtr->GetPathPointLocation(aNavPtr->GetPathPoints().Num() - 1).Position)
				//If this isn't the last point
				DrawDebugPoint(
					GetWorld(),
					Point.Location,
					10,
					FColor::Orange,
					false,
					10
				);
				else
				{
					if (FVector::Dist(aNavPtr->GetEndLocation(), PathedElements[aPathId]->GetActorLocation()) < ItemActivationDistance)
					{
						DrawDebugPoint(
							GetWorld(),
							Point.Location,
							10,
							FColor::Green,
							false,
							10
						);
					}
					else
					{
						DrawDebugPoint(
							GetWorld(),
							Point.Location,
							10,
							FColor::Red,
							false,
							10
						);
					}
				}
				if(!bIsFirstTime)
				{
					DrawDebugLine(
						GetWorld(),
						Point.Location,
						LastPoint.Location,
						FColor::Green,
						false,
						10,
						0,
						10
					);
				}

				LastPoint = Point;
			}
		}

		//Checks to see if the endpoint is valid (within activation distance)
		if (FVector::Dist(aNavPtr->GetEndLocation(), PathedElements[aPathId]->GetActorLocation()) < (150))
		{
			if (Cast<ASoulRecepticle>(PathedElements[aPathId]) && Souls <= 0)
			{
				//Ignore Soul receptacles if Skully has no souls
				UE_LOG(LogScript, Warning, TEXT("Found a soul recipticle, but I don't have any souls"));
				return;
			}


			//Only logs the path if skully can activate the item
			FoundItemPaths.Add(PathedElements[aPathId], aNavPtr);
		}
	}
}


void ASkullyMechanics::FindItemPathsListener()
{
	if(PathsFound<FoundItemCount)
	{
		//waits for paths to find without disrupting main thread (rendering the whole 'async' thing moot)
		return;
	}
	else
	{ 
		TMap<APuzzleElement*, float> ModifiedItemDistances;
		for(auto ElementPath : FoundItemPaths)
		{
			auto PathLength = ElementPath.Value->GetLength();
			for(auto priority : FoundItemPriority)
			{
				//Removes div by zero error
				if(ElementPath.Key->IsA(priority.Key) && priority.Value != 0)
				{
					//Modifies the path length based on priority
					PathLength = PathLength / priority.Value;
				}
			}

			ModifiedItemDistances.Add(ElementPath.Key, PathLength);
		}

		//Sorts the array based on the modified Distance
		ModifiedItemDistances.ValueSort([](const float& LHS, const float& RHS) {return LHS < RHS; });

		TArray<APuzzleElement*> SortedItems;
		ModifiedItemDistances.GenerateKeyArray(SortedItems);

		if (SortedItems.Num() > 0)
		{
			ClosestElement = SortedItems[0];
			BestItemPath = FoundItemPaths[ClosestElement];
		}
		else
		{
			ClosestElement = nullptr;
			OnFoundNothing();
		}

		if (ClosestElement != nullptr)
		{
			//Moves towards target (if it exists)
			SkullyController->MoveToLocation(ClosestElement->GetActorLocation(), DistToItem);
			SkullyController->TargetGoal = ClosestElement;
		}

		//Continues if all paths are found
		bPathItemLatch = false;
	}
}
#pragma  endregion Activate Functions

#pragma region
void ASkullyMechanics::SimpleMoveToTarget()
{
	if (bMovePathValid)
	{
		//Moves to the Controller's Target actor
		if (Cast<APuzzleElement>(SkullyController->TargetGoal))
		{
			SkullyController->MoveToActor(SkullyController->TargetGoal, DistToItem);
		}
		else
		{
			SkullyController->MoveToActor(SkullyController->TargetGoal, DistToShadow);
		}
	}
	//Asynchronously checks the path to shadow. Because the frame rate drops when doing it in this thread were a bitch and a half
	if (!bCheckingPathToShadow && SkullyController->TargetGoal)
	{
		bCheckingPathToShadow = true;

		

		//Stops movement if the path is too long

	}
	if(bShouldCheckForPath)
	{
		//Latches the timer
		bShouldCheckForPath = false;

		//Finds paths at a set interval by CheckForPathDelay
		FTimerHandle UnusedHandle;
		GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ASkullyMechanics::StartFindingPath, CheckForPathDelay);
	}
}

void ASkullyMechanics::StartFindingPath()
{
	//Resets latch
	bShouldCheckForPath = true;

	//Finds the navigation system
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = NavSys->MainNavData;

	FNavAgentProperties NavAgentProperties;
	NavAgentProperties.AgentHeight = SkullyAgentHeight;
	NavAgentProperties.AgentRadius = SkullyAgentRadius;
	NavAgentProperties.bCanFly = bCanSkullyFly;
	NavAgentProperties.bCanWalk = bCanSkullyWalk;

	FPathFindingQuery NavParams;
	NavParams.StartLocation = GetActorLocation();
	NavParams.EndLocation = SkullyController->TargetGoal->GetActorLocation();
	NavParams.QueryFilter = UNavigationQueryFilter::GetQueryFilter<UNavigationQueryFilter>(*NavData);

	FNavPathQueryDelegate NavDelegate;
	NavDelegate.BindUObject(this, &ASkullyMechanics::OnTargetPathFound);


	NavSys->FindPathAsync(
		NavAgentProperties,
		NavParams,
		NavDelegate,
		EPathFindingMode::Regular);
}

void ASkullyMechanics::OnTargetPathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr)
{
	bCheckingPathToShadow = false;

	float PathLength = 0;
	bool bIsFirstTime = true;
	FNavPathPoint LastPoint;

	if (aResultType == ENavigationQueryResult::Success || aResultType == ENavigationQueryResult::Fail)
	{
		//Draws Debug Path
		for (auto Point : aNavPtr->GetPathPoints())
		{
			//Debug Function to Display the paths
			if (bShowDebug)
			{
				if (Point.Location != aNavPtr->GetPathPointLocation(aNavPtr->GetPathPoints().Num() - 1).Position)
					//If this isn't the last point
					DrawDebugPoint(
						GetWorld(),
						Point.Location,
						10,
						FColor::Purple,
						false,
						CheckForPathDelay
					);
				else
				{
					if (SkullyController && FVector::Dist(aNavPtr->GetEndLocation(), SkullyController->TargetGoal->GetActorLocation()) < MaxPathDistToSkully)
					{
						DrawDebugPoint(
							GetWorld(),
							Point.Location,
							10,
							FColor::Green,
							false,
							CheckForPathDelay
						);
					}
					else
					{
						DrawDebugPoint(
							GetWorld(),
							Point.Location,
							10,
							FColor::Red,
							false,
							CheckForPathDelay
						);
					}
				}
				if (!bIsFirstTime)
				{
					DrawDebugLine(
						GetWorld(),
						Point.Location,
						LastPoint.Location,
						FColor::Purple,
						false,
						CheckForPathDelay,
						0,
						10
					);
				}
				bIsFirstTime = false;
				LastPoint = Point;
			}
		}

		if (aNavPtr->GetLength() > MaxPathDistToSkully)
		{
			//Stop moving that way
			SkullyController->StopMovement();
			bMovePathValid = false;

			//Tells Skully to wait
			CurrentState = ESkullyStates::Sk_Wait;
		}
		else
		{
			if (aNavPtr->GetPathPoints().Num() > 0)
			{
				//sets the LookAt point to the next path node
				LookAtTarget = aNavPtr->GetPathPoints()[1].Location;
			}
			bMovePathValid = true;
		}
	}
}
#pragma endregion Follow Functions

#pragma region
void ASkullyMechanics::AttackEnemy()
{
	if(SkullyController)
	{
		if (!Cast<ABadSoulMechanics>(SkullyController->TargetGoal))
		{
			for(TActorIterator<ABadSoulMechanics> EnemyItr(GetWorld()); EnemyItr; ++EnemyItr)
			{
				if(EnemyItr->CurrentState == EBadSoulStates::Bs_Possessed)
				{
					//Sets the target as the bad soul that is possessed
					SkullyController->TargetGoal = *EnemyItr;
					break;
				}
			}
			
		}
	}
	SimpleMoveToTarget();

	//Looks for possessed enemies and attacks them if in range

//	--------------------------
//	| SPHERE TRACE VARIABLES |
//	--------------------------
	TArray<FHitResult> FoundEnemies;//All hit enemies

	FCollisionObjectQueryParams FindItemTypes;//Channels to Search for
	FindItemTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);//Just looking for enemies

	FCollisionQueryParams FindItemParams;//Ignored Actors
	FindItemParams.AddIgnoredActor(this);
	FindItemParams.AddIgnoredActor(Shadow);

	//Finds items in a radius
	GetWorld()->SweepMultiByObjectType(
		FoundEnemies,
		GetActorLocation(),
		GetActorForwardVector() + GetActorLocation(),
		GetActorRotation().Quaternion(),
		FindItemTypes,
		FCollisionShape::MakeSphere(AttackRadius),
		FindItemParams
	);

	//Checks Hit items for enemies that are possessed
	for(auto FoundEnemy : FoundEnemies)
	{
		auto CastEnemy = Cast<ABadSoulMechanics>(FoundEnemy.Actor);

		if(CastEnemy != nullptr)
		{
			if(CastEnemy->CurrentState == EBadSoulStates::Bs_Possessed)
			{
				//The 'attack' Part of the code, can insert anims, etc here
				CastEnemy->Destroy();
				if(SkullyController && Shadow)
				{
					SkullyController->TargetGoal = Shadow;
				}
				CurrentState = ESkullyStates::Sk_Default;
			}
		}
	}
}
#pragma endregion Attack Functions

#pragma region 
void ASkullyMechanics::OnFlee()
{
	if (Enemy)
	{
		if(bFleePathStart)
		{
			//Sets latch
			bFleePathStart = false;

			//Finds a distance-Based time to flee the enemy (the shorter the distance, the more the timer)
			FleeTimer = FMath::Clamp((FleeTimerDistanceModifier / FVector::Dist(GetActorLocation(), Enemy->GetActorLocation())) * FLEE_PATH_TIMER_MAX, FLEE_PATH_TIMER_MIN, FLEE_PATH_TIMER_MAX);

			//Sets Timer to revert to default
			FTimerHandle UnusedHandle;
			GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ASkullyMechanics::OnFleeEnd, FleeTimer);
		}
		//Finds the Flee Path with the furthest distance to the Enemy
		GetCharacterMovement()->MaxWalkSpeed = FleeSpeed;
		if (bFindingFleePaths)
		{
			if (bFleePathSetupLatch)
			{
				FindFleePath();
			}
			else
			{
				FleePathListener();
			}
		}
		else
		{
			//Move to path
			if(SkullyController)
			{
				SkullyController->MoveToLocation(BestFleePathLocation);

				if (BestFleePathPtr)
				{
					//Sets skully to look at the next point in the best flee path
					LookAtTarget = BestFleePathPtr->GetPathPoints()[1].Location;
				}
			}
		}
	}
}

void ASkullyMechanics::FindFleePath()
{
	FleePathDir.Empty();
	BestFleePathLocation = FVector::ZeroVector;
	bFleePathSetupLatch = false;

	//	------------------------
//	| NAVIGATION VARIABLES |
//	------------------------

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = NavSys->MainNavData;


	FNavAgentProperties NavAgentProperties;
	NavAgentProperties.AgentHeight = SkullyAgentHeight;
	NavAgentProperties.AgentRadius = SkullyAgentRadius;
	NavAgentProperties.bCanFly = bCanSkullyFly;
	NavAgentProperties.bCanWalk = bCanSkullyWalk;


	FPathFindingQuery NavParams;
	NavParams.StartLocation = Enemy->GetActorLocation();//Finds the Furthest path from the enemy
	NavParams.QueryFilter = UNavigationQueryFilter::GetQueryFilter<UNavigationQueryFilter>(*NavData);

	FNavPathQueryDelegate NavDelegate;
	NavDelegate.BindUObject(this, &ASkullyMechanics::OnFleePathFound); //Triggers OnItemPathFound when path is found

	for(int i = 0; i < NumberOfFleePaths; i++)
	{
		//The even distribution of flee path angles
		const float ThetaOffset = (i * (NumberOfFleePaths / 180)) - 90;

		float AngleAwayFromEnemy;
		if(Enemy)
		{
			auto NormalizedVectorToSkully = (Enemy->GetActorLocation() - this->GetActorLocation());
			NormalizedVectorToSkully.Normalize();

			//Finds the angle (in Rads) away from the enemy
			AngleAwayFromEnemy = FMath::Atan2(NormalizedVectorToSkully.X, NormalizedVectorToSkully.Y);
			//Converts to Degrees
			AngleAwayFromEnemy = FMath::RadiansToDegrees(AngleAwayFromEnemy);
		}
		else
		{
			AngleAwayFromEnemy = GetActorRotation().Euler().Z;
		}

		//Finds the angle with which to find the next flee path
		float theta = AngleAwayFromEnemy + ThetaOffset;
		theta = (theta * 2 * PI) / 360; //Convert to rads
		
		FVector FleeDirection = FVector(cosf(theta), sinf(theta), 0);
		FVector FleePoint = (FleeDirection * FleePathRadius) + GetActorLocation();

		if(bShowDebug)
		{
			DrawDebugPoint(
				GetWorld(),
				FleePoint,
				15,
				FColor::Magenta,
				false,
				5
			);

		}

		NavParams.EndLocation = FleePoint;
		TotalFleePaths++;

		FleePathDir.Add(
			NavSys->FindPathAsync(NavAgentProperties, NavParams, NavDelegate),
			FleePoint
		);

	}
}
void ASkullyMechanics::OnFleePathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr)
{
	//Finds the furthest path from the enemy (path starts at Enemy)

	FleePathsFound++;

	FNavPathPoint LastPoint;
	bool bIsFirstTime = true;

	if(aResultType == ENavigationQueryResult::Success && FVector::Dist(aNavPtr->GetEndLocation(), FleePathDir[aPathId]) < MinDistanceToGoal)
	{

		//Draws the Debug Paths
		for (auto Point : aNavPtr->GetPathPoints())
		{
			//Debug Function to Display the paths
			if (bShowDebug)
			{
				if (Point.Location != aNavPtr->GetPathPointLocation(aNavPtr->GetPathPoints().Num() - 1).Position)
					//If this isn't the last point
					DrawDebugPoint(
						GetWorld(),
						Point.Location,
						10,
						FColor::Yellow,
						false,
						FindFleePathDelay
					);
				else
				{
					DrawDebugPoint(
						GetWorld(),
						Point.Location,
						10,
						FColor::Green,
						false,
						FindFleePathDelay
					);

				}
				if (!bIsFirstTime)
				{
					DrawDebugLine(
						GetWorld(),
						Point.Location,
						LastPoint.Location,
						FColor::Yellow,
						false,
						FindFleePathDelay,
						0,
						10
					);
				}
				bIsFirstTime = false;
				LastPoint = Point;
			}
		}

		//Sets the flee Path variables
		if(aNavPtr->GetLength() > BestFleePathDist)
		{
			BestFleePathDist = aNavPtr->GetLength();
			BestFleePathLocation = FleePathDir[aPathId];//Sets the best flee path for skully to follow
			BestFleePathPtr = aNavPtr;
		}
	}
}
void ASkullyMechanics::FleePathListener()
{
	if(FleePathsFound >= TotalFleePaths)
	{
		bFindingFleePaths = false;
		bFleePathSetupLatch = true;

		FTimerHandle UnusedHandle;
		GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ASkullyMechanics::OnFleeTimer, FindFleePathDelay);
	}
}

void ASkullyMechanics::OnFleeTimer()
{
	bFindingFleePaths = true;
}

void ASkullyMechanics::OnFleeEnd()
{

	//Resets variables
	bFleePathStart = true;
	FleeTimer = 0;
	CurrentState = ESkullyStates::Sk_Default;
}

void ASkullyMechanics::OnSetFlee(AActor* EnemyToFlee)
{
	//Called by enemy, sets itself as a refernce to flee away from

	Enemy = EnemyToFlee;//Sets the enemy instance to check against
}

#pragma endregion Flee Functions	


// Called every frame
void ASkullyMechanics::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Initializes Skully controller. Not efficient, but it catches those annoying few times that the if-else block below fails (don't ask)
	SkullyController = Cast<ASkullyController>(GetController());

	//Checks that the Controller Reference is valid
	if (SkullyController)
	{
		//Switches back to Default if the player is too far away
		if (Shadow != nullptr)
		{
			//If Shadow moves too far away, switches back to Default state
			if (GEngine && bShowDebug)
			{
				GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Blue, FString::SanitizeFloat(FVector::Dist(GetActorLocation(), Shadow->GetActorLocation())));
			}
			if (FVector::Dist(GetActorLocation(), Shadow->GetActorLocation()) > RevertToDefaultDist && CurrentState != ESkullyStates::Sk_Default)
			{
				CurrentState = ESkullyStates::Sk_Default;
				bCanTick = true;
			}
		}

		if (CheckForStateChange(CurrentState))
		{
			//If the state changes, reset variables
			ResetVars();
		}

		if (bCanTick)
		{
			switch (CurrentState)
			{
			case ESkullyStates::Sk_Default:
				DefaultTick();
				break;
			case ESkullyStates::Sk_Wait:
				SkullyWait();
				break;
			case ESkullyStates::Sk_Activate:
				ActivateItem();
				break;
			case ESkullyStates::Sk_Follow:
				//JUST follows shadow
				SkullyController->TargetGoal = Shadow;
				SimpleMoveToTarget();
				break;
			case ESkullyStates::Sk_Attack:
				AttackEnemy();
				break;
			case ESkullyStates::Sk_Flee:
				OnFlee();
				break;
			case ESkullyStates::Sk_Dead:
				OnDeathActivate();
				break;
			case ESkullyStates::Sk_Default_Wait:
				if(Shadow)
				{
					if(FVector::Dist(GetActorLocation(), Shadow->GetActorLocation()) > PressurePlateWaitDistance)
					{
						//Custom distance for Pressure plate activation
						//Resets to default state if the player walks too far away
						CurrentState = ESkullyStates::Sk_Default;
					}
				}
				break;
			}
		}
	}
}

#pragma region 
bool ASkullyMechanics::CheckForStateChange(ESkullyStates NewState)
{
	//Checks to see if skully's state has changed since last call
	static ESkullyStates PrevState;
	if (PrevState == NewState)
	{
		//the state has not changed
		return false;
	}

	//Updates previous state
	PrevState = NewState;
	return true;
}


void ASkullyMechanics::ResetVars()
{
	//Resets various latches and variables on State change
	//It's way simpler to do this here than individually bind and check functions on exit or something

	//Global Items
	bCanTick = true;
	GetCharacterMovement()->MaxWalkSpeed = NormalMoveSpeed;

	//Activate Items
	bFindItemLatch = true;

	//Follow
	bShouldCheckForPath = true;
	bMovePathValid = true;

	//Default
	bCheckGoal = true;
	bFindingGoalPaths = false;
	bFindingThinkerPaths = false;
	bSeekingThinker = false;

	GoalsFound = 0;
	GoalPaths = 0;
	GoalThinkersFound = 0;
	GoalThinkerPaths = 0;

	//Flee
	bFleePathStart = true;

}
#pragma endregion Utility Functions

// Called to bind functionality to input
void ASkullyMechanics::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}