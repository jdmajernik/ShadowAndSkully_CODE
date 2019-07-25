// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "Runnable.h"
#include "PuzzleElement.h"
#include "Kismet/GameplayStatics.h"


#include "DynaimicCameraController.generated.h"

//set up empty class for declaration
class FCheckGoalsInFocus;

USTRUCT()
struct FCameraData
{
	GENERATED_BODY()
		FVector TargetPoint = FVector::ZeroVector;
	TArray<FVector> GoalLocations;
};

DECLARE_DELEGATE_OneParam(FAsyncFinish, FCameraData);

UCLASS()
class SHADOWANDSKULLY_API ADynamicCameraController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADynamicCameraController();

	UPROPERTY(VisibleAnywhere, Category = "Dynamic Camera")
		UCameraComponent* MainCamera;

	UPROPERTY(VisibleAnywhere, Category = "Dynamic Camera")
		FVector CenterPoint;
	UPROPERTY(VisibleAnywhere, Category = "Dynamic Camera")
		FVector TargetPoint;
	UPROPERTY(VisibleAnywhere, Category = "Dynamic Camera")
		FVector AsyncCalcPoint;
	UPROPERTY(EditAnywhere, Category = "Dynamic Camera")
		float CameraSpeed = 0.85;
	UPROPERTY(EditAnywhere, Category = "Dynamic Camera")
		float GoalDist = 1500;
	UPROPERTY(EditAnywhere, Category = "Dynamic Camera")
		float ZoomIgnoreBuffer = 0.1;//The Maximum amount of distance between the camera and the ZoomToPoint that the camera will flat-out ignore

	UPROPERTY(EditAnywhere, Category = "Dynamic Camera")
		float MaxZoomInDist = 1200;
	UPROPERTY(EditAnywhere, Category = "Dynamic Camera")
		float MaxZoomOutDist = 3200;

	UPROPERTY(VisibleAnywhere, Category = "Dynamic Camera")
		AActor* Skully;
	UPROPERTY(VisibleAnywhere, Category = "Dynamic Camera")
		AActor* Shadow;

	UPROPERTY(BlueprintReadOnly)
		FVector2D FurthestPoint = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
		FVector2D ScreenCenter;

	UPROPERTY()
		TArray<FVector> CuratedGoals;
	UPROPERTY(BlueprintReadWrite)
		TArray<FVector> AdditionalGoals;

	UPROPERTY(EditAnywhere, Category = "Dynamic Camera")
		bool bShowDebug = true;



	UPROPERTY()
		bool bRunTask = true;
	UPROPERTY()
		bool bMoveCamera;
	UPROPERTY()
		bool bZoomCamera;
	UPROPERTY()
		FVector DeltaMoveSpeed = FVector::ZeroVector;
	FVector FurtherstWorldPoint;

	UFUNCTION()
		void OnAsyncFinish(FCameraData Data);

	void FindMoveDirection();
	void FindFurthestScreenPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void StartAsyncTask();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
		float DeltaMove;
	UPROPERTY()
		float DeltaZoom;
	FCameraData LastData;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void Destroyed() override;
};



class FCheckGoalsInFocus : public FNonAbandonableTask
{
	friend  class FAutoDeleteAsyncTask<FCheckGoalsInFocus>;

public:
	FCheckGoalsInFocus(TArray<FVector>NewGoals, AActor* NewShadow, AActor* NewSkully, ADynamicCameraController* NewController)
		:Goals(NewGoals),
		Skully(NewSkully),
		Shadow(NewShadow),
		Controller(NewController)
	{}

protected:
	TArray<FVector> Goals;
	AActor* Skully;
	AActor* Shadow;
	ADynamicCameraController* Controller;

	float TickSpeed = 0.05;
	FCameraData Data;
	FAsyncFinish OnAsyncFinish;
	
	void GenerateTarget(TArray<FVector> ClosestItems)
	{

		auto TargetPos = FVector::ZeroVector;
		auto ItemOffset = 1 + ClosestItems.Num();
		if (Shadow && Skully)
		{
			TargetPos += ((Shadow->GetActorLocation() * (ItemOffset * 2)) + (Skully->GetActorLocation() * ItemOffset));
		}

		for (auto Goal : ClosestItems)
		{
			TargetPos += Goal;
		}
		//Divides the calculated target positions by the total number of calculations to even out the target point
		TargetPos = TargetPos / (ClosestItems.Num() + Controller->AdditionalGoals.Num() + (ItemOffset * 3));

		//Send the generated target point to the controller
		Data.TargetPoint = TargetPos;
	}


	void DoWork()
	{
		TArray<FVector> ClosestItems;

		//Main Function loop
		while (Controller->bRunTask)
		{
			ClosestItems.Empty();
			for (int i = 0; i < 5; i++)
			{
				if (Shadow != nullptr && Goals.Num()>0 && i< Goals.Num())
				{
					//Keeps on throwing errors. Keeps game from crashing outright
					auto const Dist = FVector::Dist(Shadow->GetActorLocation(), Goals[i]);
					if (Dist < Controller->GoalDist)
					{
						ClosestItems.Add(Goals[i]);
					}
				}
			}
			for (auto AddGoal : Controller->AdditionalGoals)
			{
				ClosestItems.Add(AddGoal);
			}
			//Controller->CuratedGoals = (ClosestItems);
			//finds the target camera location
			
			GenerateTarget(ClosestItems);
			
			Data.GoalLocations = ClosestItems;

			//attempting to offload calculations back into main thread
			const FVector2D LargestPoint = Controller->FurthestPoint;

			//Checks if the furthest point is within the screen border

			FPlatformProcess::Sleep(TickSpeed);

			if (OnAsyncFinish.IsBound())
			{
				OnAsyncFinish.Execute(Data);
			}
			else
			{
				//Binds the calculation functions to AsyncFinish
				if (Controller)
				{
					OnAsyncFinish.BindUObject(Controller, &ADynamicCameraController::OnAsyncFinish);
				}
			}
			
		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCheckGoalsInFocus, STATGROUP_ThreadPoolAsyncTasks);
	}
};
