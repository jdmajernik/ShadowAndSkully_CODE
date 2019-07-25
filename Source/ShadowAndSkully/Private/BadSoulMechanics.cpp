// Fill out your copyright notice in the Description page of Project Settings.


#include "BadSoulMechanics.h"
#include "Kismet/KismetMathLibrary.h"
#include "AIController.h"
#include "TimerManager.h"
#include "PuzzleElement.h"
#include "SkullyMechanics.h"
#include "BadSoulController.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ABadSoulMechanics::ABadSoulMechanics()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABadSoulMechanics::BeginPlay()
{
	Super::BeginPlay();


	GeneratePatrolDistance();//Finds a random patrol distance in the given range

	LastPosition = GetActorLocation();
	DistanceTravelled = PatrolDistance / 2;

	SightAngleRadians = (SightAngleDegrees * 2 * PI) / 360;

	IgnoredActors.Add(this);

	FTimerHandle UnusedHandle;
	GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ABadSoulMechanics::Awake, 0.5);

	if(!GetController())
	{
		SpawnDefaultController();
	}

	//TODO - REMOVE THIS. ONLY TEMPORARY UNTIL I DYNAMICALLY GENERATE
	PatrolDir = FVector(1, 0, 0);
	Destination = GetActorLocation();//Instantiates the Destination variable for use before finding the patrol route
}
void ABadSoulMechanics::Awake()
{
	TArray<AActor*> ShadowFind;
	(UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShadowMechanics::StaticClass() , ShadowFind));
	if (ShadowFind.Num()>0)
	{
		auto Shadow = Cast<AShadowMechanics>(ShadowFind[0]);
		if(Shadow)
		{
			SkullyController = Shadow->Skully;
			Skully = Cast <ASkullyMechanics>(Shadow->Skully->GetPawn());
			BadSoulController = Cast<ABadSoulController>(GetController());

			bIsAwake = true; //The awake function is called and the actor can tick

			return;
		}
		
	}
	FTimerHandle UnusedHandle;
	GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ABadSoulMechanics::Awake, 0.5);
}



#pragma region
void ABadSoulMechanics::GeneratePatrolDistance()
{
	PatrolDistance = FMath::FRandRange(MinPatrolDistance, MaxPatrolDistance);
}

void ABadSoulMechanics::PatrolTick()
{
	GetCharacterMovement()->MaxWalkSpeed = PatrolWalkSpeed; //resets walk speed1
	if (!bPausePatrol)
	{
		if (bReturnToPatrol)
		{
			if (FVector::Dist(GetActorLocation(), LastPosition) < ReturnToPatrolDist)
			{
				//Returns to the last logged patrol state upon reaching the destination
				bReturnToPatrol = false;
			}
			else if (BadSoulController)
			{
				//Returns to the last logged position if returning from an 'attack' state
				BadSoulController->MoveToLocation(LastPosition);
			}
			else
			{
				BadSoulController = Cast<ABadSoulController>(GetController());
			}
		}
		else
		{
			//Checks for Skully in its sight range
			if (Skully)
			{
				if (FVector::Dist(GetActorLocation(), Skully->GetActorLocation()) <= SightRange)
				{
					auto Forward = UKismetMathLibrary::GetForwardVector(GetActorRotation());
					auto NormSkullyOffset = Skully->GetActorLocation() - GetActorLocation();
					NormSkullyOffset.Normalize();

					auto DotSightRadius = 1 - (SightAngleDegrees / 90);

					if (FVector::DotProduct(Forward, NormSkullyOffset) >= DotSightRadius)
					{
						FHitResult OutHit;

						FCollisionObjectQueryParams SkullySearchParams;
						SkullySearchParams.AddObjectTypesToQuery(ECC_WorldStatic);
						SkullySearchParams.AddObjectTypesToQuery(ECC_WorldDynamic);
						SkullySearchParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);

						FCollisionQueryParams IgnoredHitActors;
						IgnoredHitActors.AddIgnoredActor(this);
						IgnoredHitActors.AddIgnoredActor(Skully);

						GetWorld()->SweepSingleByObjectType(
							OutHit,
							GetActorLocation(),
							Skully->GetActorLocation(),
							GetActorRotation().Quaternion(),
							SkullySearchParams,
							FCollisionShape::MakeSphere(30),
							IgnoredHitActors
						);
						//If there is no wall blocking Line-of-sight
						if (!OutHit.bBlockingHit)
						{
							//Sets appropriate state if within range of skully
							CurrentState = EBadSoulStates::Bs_Attack;
							Skully->CurrentState = ESkullyStates::Sk_Flee;
							Skully->OnSetFlee(this);
						}
					}
				}
				if (bShowDebug)
				{
					DrawDebugLine(
						GetWorld(),
						GetActorLocation(),
						UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetActorLocation(),
						FColor::Green
					);
					DrawDebugLine(
						GetWorld(),
						GetActorLocation(),
						(UKismetMathLibrary::GetForwardVector(GetActorRotation()) * SightRange) + GetActorLocation(),
						FColor::Orange
					);
					auto Forward = UKismetMathLibrary::GetForwardVector(GetActorRotation());
					auto NormSkullyOffset = Skully->GetActorLocation() - GetActorLocation();
					NormSkullyOffset.Normalize();

					if (bShowDebug && GEngine)
					{
						GEngine->AddOnScreenDebugMessage(
							-1,
							LoggedDeltaTime,
							FColor::Cyan,
							FString::SanitizeFloat(FVector::DotProduct(Forward, NormSkullyOffset))
						);
						GEngine->AddOnScreenDebugMessage(
							-1,
							LoggedDeltaTime,
							FColor::Cyan,
							FString::SanitizeFloat(1 - (SightAngleDegrees / 90))
						);
					}
				}
			}

			if (bCheckPatrol)
			{
				
				bCheckPatrol = false;

				FTimerHandle CheckPatrolHandle;
				GetWorld()->GetTimerManager().SetTimer(CheckPatrolHandle, this, &ABadSoulMechanics::FindWalls, PatrolUpdateTime);
			}

			auto const Distance = FVector::Distance(LastPosition, GetActorLocation());
			auto MoveDir = (UKismetMathLibrary::GetForwardVector(GetActorRotation()) * 200) + GetActorLocation();

			//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Blue, FString("EnemyTick"));


			//Destination = (PatrolDir * 200) + GetActorLocation();
			//auto BadSoulController = Cast<AAIController>(this->GetController());
			if(BadSoulController)
			{
				BadSoulController->MoveToLocation(Destination);
			}
			DistanceTravelled += Distance;
			LastPosition = GetActorLocation();
		}
	}
	else
	{
		PatrolPause();
	}
}
void ABadSoulMechanics::PatrolPause()
{
	//An overridable function for actions during a paused patrol route
	bPausePatrol = false;
}
void ABadSoulMechanics::FindWalls()
{
	//Resets Variables
	bCheckPatrol = true;
	bIsWallAhead = false;

	if (bShowDebug)
	{
		//Shows check radius
		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			WallCheckRadius,
			16,
			FColor::Red,
			false,
			PatrolUpdateTime
		);
	}
	//
	TArray<FHitResult> StaticItemHits;

	FCollisionQueryParams PatrolHitParams;
	PatrolHitParams.AddIgnoredActors(IgnoredActors);

	//Scanning for all objects that block movement
	FCollisionObjectQueryParams HitCollisionTypes;
	HitCollisionTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);//Static Objects
	HitCollisionTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel1);//'Normal' Objects
	HitCollisionTypes.AddObjectTypesToQuery(ECC_Destructible);


	XOffsetWalls.Empty();
	YOffsetWalls.Empty();


	TArray<FVector> OpenDirections;
	FHitResult OutHit;

	FVector WallAheadLoc;


	//Checks the Four directions around the player for walls
	for (int Direction = 0; Direction <= 1; Direction++)
	{
		auto IsX = static_cast<bool>(Direction);
		//The bool determines the direction, the offset dir determines the orientation
		for (int OffsetOrientation = -1; OffsetOrientation <= 1; OffsetOrientation += 2)
		{
			//Generates a Movement vector to raytrace against
			FVector CheckDir;
			if(IsX == 1)
			{
				CheckDir = FVector(OffsetOrientation, 0, 0);
			}
			else
			{
				CheckDir = FVector(0, OffsetOrientation, 0);
			}
			
			FVector EndPoint = GetActorLocation() + (CheckDir * WallCheckRadius);

			GetWorld()->LineTraceSingleByObjectType(
				OutHit,
				GetActorLocation(),
				EndPoint,
				HitCollisionTypes,
				PatrolHitParams
			);
				
	
			//If there is no collision (the way is open), adds to open Directions array
			if (!OutHit.bBlockingHit)
			{
				OpenDirections.Add(CheckDir);
			}
			else
			{
				//If there IS a wall there, logs it to check against
				//Checks if the Hit item is valid (Ignores puzzle elements and both the player and skully)
				if (!Cast<APuzzleThinker>(OutHit.Actor) && !Cast<ASkullyMechanics>(OutHit.Actor) && !Cast<AShadowMechanics>(OutHit.Actor))
				{
					//Checks if there is a wall ahead
					auto WallOffset = GetActorLocation() - OutHit.ImpactPoint;
					WallOffset.Normalize();
					if (FVector::DotProduct(WallOffset, PatrolDir) < -0.9)
					{
						bIsWallAhead = true;
						WallAheadLoc = OutHit.ImpactPoint;
					}
					if (IsX)
					{
						XOffsetWalls.Add(OutHit.ImpactPoint);
					}
					else
					{
						YOffsetWalls.Add(OutHit.ImpactPoint);
					}
				}

				if (bShowDebug)
				{
					DrawDebugPoint(GetWorld(), OutHit.ImpactPoint, 15, FColor::Red, false, PatrolUpdateTime);
				}
			}
	
					
				
		}
	}

	//Turns around if hits patrol distance
	if (DistanceTravelled >= PatrolDistance)
	{
		GEngine->AddOnScreenDebugMessage(-1, PatrolUpdateTime, FColor::Red, "Turning Around");
		//Turn Around, bright eyes
		PatrolDir = PatrolDir * -1;
		DistanceTravelled = 0;
		bPausePatrol = true; //Pause the patrol when at the end of a route
	}

	//if there is a wall ahead, checks against the distance to the wall aheadS
	if (bIsWallAhead && FVector::Dist(GetActorLocation(), WallAheadLoc) < DistToWall)
	{
		//If surrounded in a U junction, reset patrol and turn around
		if(YOffsetWalls.Num() == 1 && XOffsetWalls.Num() == 1)
		{

			//Compares the Open directions against the Current Move Direction
			for (auto OpenDirection : OpenDirections)
			{
				if ((OpenDirection  * -1) != PatrolDir)
				{
					PatrolDir = OpenDirection;
					Destination = (PatrolDir * 200) + GetActorLocation();
					bPausePatrol = true;
					break; //DON'T Forget!!!
				}
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, PatrolUpdateTime, FColor::Red, "Turning Around");
			//If the AI is close enough to the wall, turn around
			if (FVector::Dist(GetActorLocation(), WallAheadLoc) < DistToWall)
			{
				PatrolDir = PatrolDir * -1;
				DistanceTravelled = 0;
			}
		}
	}
	
	//Averages the distance between walls and tries to patrol there
	else if (XOffsetWalls.Num() >= 2 && PatrolDir.Y != 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, PatrolUpdateTime, FColor::Green, "Moving Forward");
		float AverageXPos = (XOffsetWalls[0].X + XOffsetWalls[1].X) / 2;
		FVector UpdatedLocation = FVector(AverageXPos, GetActorLocation().Y, GetActorLocation().Z);
		Destination = (PatrolDir * 200) + UpdatedLocation;

	}
	else if (YOffsetWalls.Num() >= 2 && PatrolDir.X != 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, PatrolUpdateTime, FColor::Green, "Moving Forward");
		float AverageYPos = (YOffsetWalls[0].Y + YOffsetWalls[1].Y) / 2;
		FVector UpdatedLocation = FVector(GetActorLocation().X, AverageYPos, GetActorLocation().Z);
		Destination = (PatrolDir * 200) + UpdatedLocation;

	}
	else
	{
		//If there is nothing in the way, keep going straight
		Destination = (PatrolDir * 200) + GetActorLocation();
	}
}
#pragma  endregion Patrol Functions

#pragma region 
void ABadSoulMechanics::OnAttack()
{
	bReturnToPatrol = true;

	GetCharacterMovement()->MaxWalkSpeed = AttackWalkSpeed;

	FTimerHandle UnusedHandle;
	GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ABadSoulMechanics::StopChase, StopChasingTime);
	
	//check controller and skully references
	if (BadSoulController && Skully)
	{
		//If valid, move to skully
		BadSoulController->MoveToActor(Skully);
		if (FVector::Dist(GetActorLocation(), Skully->GetActorLocation()) < AttackDist)
		{
			FHitResult OutHit;

			FCollisionObjectQueryParams CollisionParams;
			CollisionParams.AddObjectTypesToQuery(ECC_WorldStatic);
			CollisionParams.AddObjectTypesToQuery(ECC_Destructible);
			CollisionParams.AddObjectTypesToQuery(ECC_Pawn);
			CollisionParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);

			FCollisionQueryParams IgnoreParams;
			IgnoreParams.AddIgnoredActors(IgnoredActors);

			GetWorld()->SweepSingleByObjectType(
				OutHit,
				GetActorLocation(),
				Skully->GetActorLocation(),
				GetActorRotation().Quaternion(),
				CollisionParams,
				FCollisionShape::MakeSphere(15),
				IgnoreParams
			);
			if (Cast<ASkullyMechanics>(OutHit.Actor))
			{
				//sets Skully's state to dead
				Skully->CurrentState = ESkullyStates::Sk_Dead;
				//CurrentState = EBadSoulStates::Bs_Possessed;//Freezes bad soul
			}
		}
	}
	else
	{
		//Resets Reference Variables
		auto const Shadow = Cast<AShadowMechanics>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (Shadow)
		{
			SkullyController = Shadow->Skully;
			Skully = Cast <ASkullyMechanics>(Shadow->Skully->GetPawn());
			BadSoulController = Cast<ABadSoulController>(GetController());
		}
	}
}
void ABadSoulMechanics::StopChase()
{
	if(Skully)
	{
		//Resets the current state to patrol
		//if(FVector::Dist(GetActorLocation(), Skully->GetActorLocation()) > SightRange)
		{
			CurrentState = EBadSoulStates::Bs_Patrol;
		}
	}
}
#pragma endregion Attack Functions

// Called every frame
void ABadSoulMechanics::Tick(float DeltaTime)
{

	Super::Tick(DeltaTime);

	if (bIsAwake)
	{

		LoggedDeltaTime = DeltaTime;

		switch (CurrentState)
		{
		case EBadSoulStates::Bs_Patrol:
			PatrolTick();
			break;
		case  EBadSoulStates::Bs_Attack:
			OnAttack();
			break;
		case EBadSoulStates::Bs_Possessed:
			break;
		case EBadSoulStates::Bs_Freeze:
			//Similar to 'possessed' without skully attacking it
			break;
		}
	}
}

void ABadSoulMechanics::Destroyed()
{

	Super::Destroyed();
}

// Called to bind functionality to input
void ABadSoulMechanics::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


