// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"
#include "AIController.h"

#include "BadSoulMechanics.generated.h"

class ASkullyController;
class ASkullyMechanics;
class ABadSoulController;

UENUM(BlueprintType)
enum class EBadSoulStates : uint8
{
	Bs_Patrol      UMETA(DisplayName = "Patrol"),
	Bs_Possessed   UMETA(DisplayName = "Possessed"),
	Bs_Attack	   UMETA(DisplayName = "Attack"),
	Bs_Freeze	   UMETA(DisplayName = "Paused"), 
	Bs_Idle	       UMETA(DisplayName = "Idle"),
	Bs_Dead        UMETA(DisplayName = "Dead")
};

UCLASS()
class SHADOWANDSKULLY_API ABadSoulMechanics : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABadSoulMechanics();


// ----- Public AI Variables -----
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Bad Soul AI")
		EBadSoulStates CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bad Soul AI")
		bool bShowDebug = true;

	UPROPERTY(VisibleAnywhere, Category = "Bad Soul AI|Patrol")
		float PatrolDistance = 800;//The total distance that the AI should patrol
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Patrol")
		float MinPatrolDistance = 800;//The minimum generated patrol distance
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Patrol")
		float MaxPatrolDistance = 1200;//The maximum generated patrol distance
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Patrol")
		float WallCheckRadius = 300; //The radius with which to check for walls surrounding the AI
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Patrol")
		float DistToWall = 250; //The radius with which to check for walls surrounding the AI
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Patrol")
		float ReturnToPatrolDist = 50; //The radius with which to check for walls surrounding the AI

	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Attack")
		float AttackDist = 150; //The radius with which to check for walls surrounding the AI
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Attack")
		float SightRange = 500; //The radius with which to check for walls surrounding the AI
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Attack")
		float SightAngleDegrees = 30; //The radius with which to check for walls surrounding the AI

	float SightAngleRadians;//The converted Sight angle to radians for use in code

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void Awake();

	float LoggedDeltaTime = 0;//Logging delta time for debug display reasons

	UPROPERTY()
		bool bIsAwake = false;

	UPROPERTY()
	ASkullyController* SkullyController;//Skully Controller Reference
	UPROPERTY()
	ASkullyMechanics* Skully;//Skully Object Reference
	UPROPERTY(VisibleAnywhere, Category = "Bad Soul AI")
	ABadSoulController* BadSoulController;//This object's controller
	

//--------PATROL VARIABLES--------

	UPROPERTY()
		bool bCheckPatrol = true; //Patrol path generation latch
	UPROPERTY()
		bool bPausePatrol = true; //
	UPROPERTY()
		bool bIsWallAhead = false;//Checks if there is a wall ahead of the AI
	UPROPERTY()
		bool bReturnToPatrol = false;

	UPROPERTY()
		FVector LastPosition; //The position of the AI on the last Patrol Function Call


	UPROPERTY(VisibleAnywhere, SaveGame, Category = "Bad Soul AI|Patrol")
		FVector PatrolDir; //Directional vector that determines the best direction to take while patrolling
	UPROPERTY(VisibleAnywhere, Category = "Bad Soul AI|Patrol")
		FVector Destination; //The destination for this instance of the patrol route

	UPROPERTY(VisibleAnywhere, Category = "Bad Soul AI|Patrol")
		TArray<FVector> XOffsetWalls;
	UPROPERTY(VisibleAnywhere, Category = "Bad Soul AI|Patrol")
		TArray<FVector> YOffsetWalls;

	UPROPERTY()
		TArray<AActor*>IgnoredActors;//The actors to ignore for hitscans (For child classes)

	UPROPERTY()
		float DistanceTravelled = 0; //The total Distance travelled in this patrol route
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Patrol")
		float PatrolUpdateTime = 0.15; //The time (in seconds) that the patrol path should be updated

	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Patrol")
		float PatrolWalkSpeed= 450; //The max walking speed of the patrol state

	void GeneratePatrolDistance(); //Randomly generates patrol distance (can be called during state switches to mix up the game a bit)
	void PatrolTick(); //The Primary patrol function called on tick
	void FindWalls(); //Finds the walls around the AI
	virtual void PatrolPause();//The function to call when the patrol is paused

//--------ATTACK VARIABLES--------

	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Attack")
		float StopChasingTime = 2; //The time (in seconds) that the Bad soul will chase Skully
	UPROPERTY(EditAnywhere, Category = "Bad Soul AI|Attack")
		float AttackWalkSpeed = 1000; //The max walking speed of the attack state

	virtual void OnAttack();
	void StopChase();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void Destroyed() override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};

