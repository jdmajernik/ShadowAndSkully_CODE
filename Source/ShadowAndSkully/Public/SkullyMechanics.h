// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SkullyController.h"
#include "SkullyMechanics.generated.h"

UENUM(BlueprintType)
enum class ESkullyStates : uint8
{
	Sk_Default		UMETA(DisplayName = "Default"),
	Sk_Follow		UMETA(DisplayName = "Follow"),
	Sk_Wait			UMETA(DisplayName = "Wait"),
	Sk_Attack		UMETA(DisplayName = "Attack"),
	Sk_Activate		UMETA(DisplayName = "Activate"),
	Sk_Flee			UMETA(DisplayName = "Flee"),
	Sk_Dead			UMETA(DisplayName = "Dead"),
	Sk_Default_Wait	UMETA(DisplayName = "Pressure Plate Wait")
};

class APuzzleElement;
class APuzzleDoer;
class APuzzleThinker;

UCLASS()
class SHADOWANDSKULLY_API ASkullyMechanics : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASkullyMechanics();

	


//-------- GLOBAL VARIABLES --------
// Used across most/all functions

		//Object References
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skully AI")
		ASkullyController* SkullyController;//This Actor's controller
	UPROPERTY()
		ACharacter* Shadow;//Main Character Reference

	//TODO Migrate to BlueprintReadOnly for sanitization
	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadWrite, Category = "Skully AI")
		ESkullyStates CurrentState;//Current AI State

		//Boolean
	UPROPERTY()
		bool bCanTick = true;//Halts tick functions if needed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skully AI")
		bool bShowDebug = false;//Shows Debug Functionality

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadWrite, Category = "Skully AI")
		int Souls = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skully AI")
		FVector LookAtTarget;

	UPROPERTY(VisibleAnywhere, Category = "Skully AI")
		TArray<APuzzleThinker*> ThinkerMasterList; //The list of all thinkers in the level

	//Distance Variables

	//The distance shadow has to be to revert to the Default state
	UPROPERTY(EditAnywhere, Category = "Skully AI")
		float RevertToDefaultDist = 1200;
	//The Normal Movement Speed for Skully
	UPROPERTY(EditAnywhere, Category = "Skully AI")
		float NormalMoveSpeed = 600; 

	//The distance to the generated Thinker that skully will auto-activate it
	UPROPERTY(EditAnywhere, Category = "Skully AI|Default")
		float AutoActivationDistance = 500;
	//The distance to the generated Thinker that skully will auto-activate it
	UPROPERTY(EditAnywhere, Category = "Skully AI|Default")
		float GoalSearchRadius = 1500;
	//The rate (in Seconds) at which the AI's Goals are refreshed
	UPROPERTY(EditAnywhere, Category = "Skully AI|Default")
		float GoalRefreshRate = 1;
	//The Time in Seconds that the AI will wait before attempting to activate an item
	UPROPERTY(EditAnywhere, Category = "Skully AI|Default")
		float AutoActivationCooldown = 1.5;
	//The distance from Shadow which Skully will wait on a pressure plate
	UPROPERTY(EditAnywhere, Category = "Skully AI|Default")
		float PressurePlateWaitDistance = 500;

	//The AI move distance to the Shadow Actor
	UPROPERTY(EditAnywhere, Category = "Skully AI|Follow")
		float DistToShadow = 100;
	//The maximum path distance that Skully can take to move to Shadow (Stops AI from traversing map to get to 'closer' point)
	UPROPERTY(EditAnywhere, Category = "Skully AI|Follow")
		float MaxPathDistToSkully = 2000;


	//The Range that Skully checks items
	UPROPERTY(EditAnywhere, Category = "Skully AI|Activate")
		float ItemCheckRange = 1200;
	//The AI move distance to Items (Such as pressure plates)
	UPROPERTY(EditAnywhere, Category = "Skully AI|Activate")
		float DistToItem = 1;
	//The distance with which skully can Activate Items
	UPROPERTY(EditAnywhere, Category = "Skully AI|Activate")
		float ItemActivationDistance = 175;
	//Multiplier to modify check distance to pressure plates
	UPROPERTY(EditAnywhere, Category = "Skully AI|Activate")
		float PressurePlateMultiplier = 2.5;

	//The distance with which skully can see possessed enemies
	UPROPERTY(EditAnywhere, Category = "Skully AI|Attack")
		float EnemyDetectionRange = 800;
	//The distance with which skully can attack possessed enemies
	UPROPERTY(EditAnywhere, Category = "Skully AI|Attack")
		float AttackRadius = 150;


	//Navigation-related variables
	//Allows me to change variables in one place and in-editor

	UPROPERTY(EditAnywhere, Category = "Skully AI|Navigation Variables")
		float SkullyAgentHeight = 150;
	UPROPERTY(EditAnywhere, Category = "Skully AI|Navigation Variables")
		float SkullyAgentRadius = 35;
	UPROPERTY(EditAnywhere, Category = "Skully AI|Navigation Variables")
		bool bCanSkullyFly = false;
	UPROPERTY(EditAnywhere, Category = "Skully AI|Navigation Variables")
		bool bCanSkullyWalk = true;


	void OnSetFlee(AActor* EnemyToFlee);

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Delayed BeginPlay call that lets all other variables to instantiate
	UFUNCTION(BlueprintNativeEvent)
		void Awake();//Waits a small amount of time after BeginPlay for all objects to be instantiated

	UFUNCTION(BlueprintImplementableEvent)
		void OnDeathActivate();

	UFUNCTION()
		bool CheckForStateChange(ESkullyStates NewState);//Checks if there is a change in Skully's AI state


//-------- DEFAULT VARIABLES --------

	bool bCheckGoal = true;//Latch to check that the goal is still correct
	bool bFindingGoalPaths = false;
	bool bFindingThinkerPaths = false;
	bool bSeekingThinker = false;

	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Default")
	int GoalsFound = 0;
	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Default")
	int GoalPaths = 0;
	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Default")
	int GoalThinkersFound = 0;
	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Default")
	int GoalThinkerPaths = 0;

	//Pathfinding distance to the next goal object, prevents non-existent paths from being counted as valid
	UPROPERTY(EditAnywhere, Category = "Skully AI|Default")
	float MinDistanceToGoal = 250;

	//The Next goals to look for in the level
	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Default")
	TArray<APuzzleDoer*> NextGoals; 

	//the thinkers that activate the next goal item
	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Default")
	TArray<APuzzleThinker*> NextGoalThinkers;

	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Default")
		TMap<APuzzleThinker*, float> FoundItemThinkers;

	APuzzleThinker* AutoActivatedThinker;

	//The goals that are being found through a path
	UPROPERTY()
	TMap<uint32, APuzzleDoer*> PathedGoals;

	//The thinkers that are being found through a path
	UPROPERTY()
	TMap<uint32, APuzzleThinker*> PathedGoalThinkers;

	//The tick function called by the default state
	UFUNCTION()
		void DefaultTick();

	//Finds the goal that the player is moving toward
	void FindNextGoal();
	//The bound function to the path found async method
	void OnNextGoalFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr);
	//The bound function to the path found to the linked goal thinkers in async method
	void OnThinkerPathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr);
	//Listens for all paths to goals to be found
	void GoalPathListener();
	//Listens for all paths to Linked thinkers to be found
	void ThinkerPathListener();
	
	

	

//-------- FOLLOW VARIABLES --------
	//Latch that waits for the Async path find to respond
	UPROPERTY()
		bool bCheckingPathToShadow = false; 
	UPROPERTY()
		bool bMovePathValid = true;
	UPROPERTY()
		bool bShouldCheckForPath = true;

	//The Time (in seconds) to wait between path checks
	UPROPERTY(EditAnywhere, Category = "Skully AI|Follow")
		float CheckForPathDelay = 0.3;


	//Moves to the target goal, called on tick
	UFUNCTION()
		void SimpleMoveToTarget(); 

	void StartFindingPath(); //The bound path generator that is fired on timer set by CheckForPathDelay
	void OnTargetPathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr); //Listens for the async path to the Target goal to be found


//-------- WAIT VARIABLES --------	
	//Functions//
	UFUNCTION()
		void SkullyWait();//Simply stops movement

//-------- ACTIVATE VARIABLES --------
	//Variables//
	bool bFindItemLatch = true;
	bool bPathItemLatch = true;

	//The amount of Puzzle Thinkers to find paths to listen for
	int FoundItemCount = 0;
	//The amount of completed Paths to Puzzle Thinkers
	int PathsFound = 0;

	//Modifies the items found by the float value (HIGHER -> MORE LIKELY TO FIND)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Skully AI|Activate")
		TMap<TSubclassOf<APuzzleElement>, float> FoundItemPriority;

	float ShortestThinkerPathDist = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Skully AI|Activate")
		FVector ActivationStartPoint;

	//The closest puzzle thinker
	APuzzleElement* ClosestElement;
	//The Closest goal to where the player is heading
	APuzzleDoer* ClosestGoal; 

	//Stored valid paths to found elements that will be sorted based on FoundItemPriority
	TMap<APuzzleElement*, FNavPathSharedPtr> FoundItemPaths;

	//Logs the element and the path ID for retrieval in path found function
	TMap<uint32, APuzzleElement*> PathedElements;

	//The chosen path that Skully will use to look at
	FNavPathSharedPtr BestItemPath;

	//Functions//
	UFUNCTION()
		void ActivateItem();

	void FindPuzzleItems(); //Finds all the puzzle items in a set distance and finds the shortest viable path back to one
	void FindItemPathsListener();//Listens for the Async functions to finish up
	void OnItemPathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr);//Plots a path to a found item and sends data back

	UFUNCTION(BlueprintImplementableEvent)
		void OnFoundNothing();

//-------- ATTACK VARIABLES --------
	UFUNCTION()
		void AttackEnemy(); //Searches for Enemy Actors



//-------- FLEE VARIABLES ---------

	bool bFindingFleePaths = true;
	bool bFleePathSetupLatch = true;
	UPROPERTY()
		bool bFleePathStart = true;

	//The delay speed (in seconds) between flee path find calls
	UPROPERTY(EditAnywhere, Category = "Skully AI|Flee")
		float FindFleePathDelay = 0.2;
	//The Movement Speed with which to set skully while in 'flee' mode
	UPROPERTY(EditAnywhere, Category = "Skully AI|Flee")
		float FleeSpeed = 600;
	//The Radius with which to check flee paths
	UPROPERTY(EditAnywhere, Category = "Skully AI|Flee")
		float FleePathRadius = 500;

	//The MAX time (in seconds) Skully Should flee an enemy
	UPROPERTY(EditAnywhere, Category = "Skully AI|Flee")
		float FLEE_PATH_TIMER_MAX = 3;
	//The MIN time (in seconds) Skully Should flee an enemy
	UPROPERTY(EditAnywhere, Category = "Skully AI|Flee")
		float FLEE_PATH_TIMER_MIN = 0.5;
	//The minimum distance for the Max flee time
	UPROPERTY(EditAnywhere, Category = "Skully AI|Flee")
		float FleeTimerDistanceModifier = 300;
	//The time (in Seconds) Skully should flee from the enemy -> GENERATED
	UPROPERTY(VisibleAnywhere, Category = "Skully AI|Flee")
		float FleeTimer = 0;

	//The Longest distance from the enemy for skully to flee
	float BestFleePathDist = 0;

	//The Number of paths to find in the 180 arc away from the enemy
	UPROPERTY(EditAnywhere, Category = "Skully AI|Flee")
		int NumberOfFleePaths = 9; 

	int TotalFleePaths = 0;
	int FleePathsFound = 0;

	UPROPERTY()
		FVector BestFleePathLocation;
	
	FNavPathSharedPtr BestFleePathPtr;


	UPROPERTY()
		TMap<int32, FVector> FleePathDir;

	UPROPERTY()
		AActor* Enemy;

	void OnFlee();
	void FindFleePath();
	void OnFleePathFound(uint32 aPathId, ENavigationQueryResult::Type aResultType, FNavPathSharedPtr aNavPtr);;
	void FleePathListener();
	void OnFleeTimer();
	void OnFleeEnd();

	
	



	//Messy solution of sorts, but this will reset latches in the event of a state change
	void ResetVars();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
