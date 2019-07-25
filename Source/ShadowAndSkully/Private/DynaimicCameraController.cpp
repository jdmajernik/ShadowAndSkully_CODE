// Fill out your copyright notice in the Description page of Project Settings.


#include "DynaimicCameraController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "ShadowMechanics.h"
#include "SkullyMechanics.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ADynamicCameraController::ADynamicCameraController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MainCamera = CreateDefaultSubobject<UCameraComponent>(FName("Camera"));
	RootComponent = MainCamera;

	//Creates camera on the singleton
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADynamicCameraController::StaticClass(), FoundCameras);
	for(auto Camera : FoundCameras)
	{
		if(Camera != this)
		{
			//Destroys other cameras if they exist
			Camera->Destroy();
		}
	}
}

// Called when the game starts or when spawned
void ADynamicCameraController::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* player = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	bFindCameraComponentWhenViewTarget = true;

	AdditionalGoals.Empty();

	player->SetViewTarget(this);
	MainCamera->Activate(false);

	for (TActorIterator<AShadowMechanics> ShadowItr(GetWorld()); ShadowItr; ++ShadowItr)
	{
		Shadow = *ShadowItr; //Adds in every Puzzle Thinker in the level
	}
	for (TActorIterator<ASkullyMechanics> SkullyItr(GetWorld()); SkullyItr; ++SkullyItr)
	{
		Skully = *SkullyItr; //Adds in every Puzzle Thinker in the level
	}




	FTimerHandle UnusedHandle;
	GetWorldTimerManager().SetTimer(UnusedHandle, this, &ADynamicCameraController::StartAsyncTask, 0.1);

}


void ADynamicCameraController::OnAsyncFinish(FCameraData Data)
{
	//Updates camera data
	LastData = Data;
}

void ADynamicCameraController::FindMoveDirection()
{
	//Get The world point where the camera is looking
	FHitResult outHit;
	GetWorld()->LineTraceSingleByChannel(
		outHit,
		GetActorLocation(),
		(UKismetMathLibrary::GetForwardVector(GetActorRotation()) * 5000) + GetActorLocation(),
		ECollisionChannel::ECC_GameTraceChannel3
	);

	AsyncCalcPoint = LastData.TargetPoint;
	CenterPoint = outHit.ImpactPoint;

	const FVector Dist = LastData.TargetPoint - CenterPoint;
	const FVector CalcTarget = GetActorLocation() + Dist;

	if (FVector::Dist(CenterPoint, AsyncCalcPoint) > 100)
	{

		auto Loc = GetActorLocation();
		//Sets the target point with relation to the camera (i.e. floating in the air)
		TargetPoint = FVector(CalcTarget.X, CalcTarget.Y, Loc.Z);

		//scales the speed based on distance to the target [up to double CameraSpeed]
		//Also scales based on resolution (TODO Scale for found resolution)
		const auto moveX = (FMath::Clamp((FMath::Abs(Loc.X - TargetPoint.X) / 100), float(0), CameraSpeed * 4));
		const auto moveY = (FMath::Clamp((FMath::Abs(Loc.Y - TargetPoint.Y) / 200), float(0), (CameraSpeed * 2)));

		//Calculates the Scaled camera move speed
		DeltaMoveSpeed = FVector(moveX, moveY, CameraSpeed);
		bMoveCamera = true;
	}
	else
	{
		//Stops camera movement if within acceptance radius
		bMoveCamera = false;
	}



}

void ADynamicCameraController::FindFurthestScreenPoint()
{
	APlayerController* player = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	int32 ScreenX, ScreenY;
	player->GetViewportSize(ScreenX, ScreenY);

	auto ClosestItems = LastData.GoalLocations;

	//Center of the viewport
	const FVector2D CenterScreen((float(ScreenX) / 2), (float(ScreenY) / 2));

	//screen point that is the furthest from the center
	FVector2D LargestPoint = CenterScreen;

	//current screen point to check against;
	FVector2D ScreenPoint;

	//Checking the screen points of all current goal objects

	for (int i = 0; i< ClosestItems.Num(); i++)
	{
		player->ProjectWorldLocationToScreen(ClosestItems[i], ScreenPoint);

		if (FVector2D::Distance(ScreenPoint, FVector2D((float(ScreenX) / 2), (float(ScreenY) / 2))) > FVector2D::Distance(LargestPoint, FVector2D((float(ScreenX) / 2), (float(ScreenY) / 2))))
		{
			LargestPoint = ScreenPoint;
			FurtherstWorldPoint = ClosestItems[i];
		}
	}
	if (Shadow && player)
	{
		player->ProjectWorldLocationToScreen(Shadow->GetActorLocation(), ScreenPoint);

		if (FVector2D::Distance(ScreenPoint, CenterScreen) > FVector2D::Distance(LargestPoint, CenterScreen))
		{
			LargestPoint = ScreenPoint;
			FurtherstWorldPoint = Shadow->GetActorLocation();
		}
	}

	if (Skully && player)
	{
		player->ProjectWorldLocationToScreen(Skully->GetActorLocation(), ScreenPoint);

		if (FVector2D::Distance(ScreenPoint, CenterScreen) > FVector2D::Distance(LargestPoint, CenterScreen))
		{
			LargestPoint = ScreenPoint;
			FurtherstWorldPoint = Skully->GetActorLocation();
		}
	}

	//Debug
	if (bShowDebug)
	{
		DrawDebugPoint(
			GetWorld(),
			FurtherstWorldPoint,
			3,
			FColor::Orange);
		DrawDebugLine(
			GetWorld(),
			CenterPoint,
			FurtherstWorldPoint,
			FColor::Orange,
			false,
			-1,
			0,
			4);
	}


	FurthestPoint = LargestPoint;
}



void ADynamicCameraController::StartAsyncTask()
{
	TArray<FVector> Goals;
	for (TActorIterator<APuzzleThinker> ThinkerItr(GetWorld()); ThinkerItr; ++ThinkerItr)
	{
		Goals.Add(ThinkerItr->GetActorLocation()); //Adds in every Puzzle Thinker in the level
	}

	bRunTask = true;
	GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Blue, "Starting Async Function in Controller");

	(new FAutoDeleteAsyncTask<FCheckGoalsInFocus>(Goals, Shadow, Skully, this))->StartBackgroundTask();
}

void ADynamicCameraController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	bRunTask = false;
}

// Called every frame
void ADynamicCameraController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector MoveToPoint;
	FVector ZoomToPoint;

	//Calculates the Data captured in the Async task
	FindMoveDirection();
	FindFurthestScreenPoint();

	if (bMoveCamera)
	{
		//Lerps based on a 2d move vector
		auto const Loc = GetActorLocation();
		MoveToPoint = FMath::Lerp(GetActorLocation(), TargetPoint, (DeltaTime * DeltaMoveSpeed));
	}

	APlayerController* player = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	int32 ScreenX, ScreenY;
	player->GetViewportSize(ScreenX, ScreenY);

	ScreenCenter = FVector2D(ScreenX, ScreenY)/2;
;
	const int Border = 40;

	auto const ZoomSpeedScale = (ScreenX + ScreenY) / 25;
	auto const UnClampedSpeed = FVector2D::Distance(FurthestPoint, ScreenCenter) / ZoomSpeedScale;
	auto const ZoomSpeed = FMath::Clamp(UnClampedSpeed, float(0.5), float(25));

	auto const XDistFromScreenCenter = FMath::Abs(FurthestPoint.X - (ScreenX / 2));
	auto const YDistFromScreenCenter = FMath::Abs(FurthestPoint.Y - (ScreenY / 2));

	if((XDistFromScreenCenter > ((ScreenX/2) - Border) ||
		YDistFromScreenCenter > ((ScreenY / 2) - Border))
		&& FVector::Dist(GetActorLocation(), CenterPoint) < MaxZoomOutDist)
	{
		//Zooms out if the Furthest point is outside the screen
		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, "I need to Zoom out");
			GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Green, FString::SanitizeFloat(FVector::Dist(GetActorLocation(), CenterPoint)));
		}

		ZoomToPoint = (UKismetMathLibrary::GetForwardVector(GetActorRotation()) * (-ZoomSpeed)) + GetActorLocation();
		bZoomCamera = true;
	}

	else if ((XDistFromScreenCenter < ((ScreenX/2) - (Border * 3)) ||
		YDistFromScreenCenter < ((ScreenY / 2) - (Border * 3)) )
		&& FVector::Dist(GetActorLocation(), CenterPoint) > MaxZoomInDist)
	{
		//zooms in if the furthest point is within a set distance from the screen

		if (bShowDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, "I need to Zoom in");
			
		}
		
		ZoomToPoint = (UKismetMathLibrary::GetForwardVector(GetActorRotation()) * ZoomSpeed) + GetActorLocation();
		bZoomCamera = true;
	}

	else
	{
		bZoomCamera = false;
	}

	if(bZoomCamera && FVector::Dist(GetActorLocation(), ZoomToPoint) <= ZoomIgnoreBuffer)
	{
		//If the zoom is too small, do not zoom
		bZoomCamera = false;
	}

	//Isolating variables
	{
		//The total Movement vector
		FVector UpdatedLocation;

		if (bMoveCamera && bZoomCamera)
		{
			//This Whole thing lerps between zooming and moving the camera based on a even scale
			//I get an even scale by finding the average of all Movement and Zoom vectors (net result is more reliable IMHO)

			//gets the distance to the respective movement points
			auto const Loc = GetActorLocation();
			auto DistToMove = FVector::Dist(Loc, MoveToPoint);
			auto DistToZoom = FVector::Dist(Loc, ZoomToPoint);

			//Updates Deltas
			DeltaMove = (DeltaMove + DistToMove) / 2;
			DeltaZoom = (DeltaZoom + DistToZoom) / 2;

			//Makes sure we don't divide by zero
			DistToMove = DeltaMove == 0 ? DistToMove : (DistToMove + DeltaMove) / DeltaMove;
			DistToZoom = DeltaZoom == 0 ? DistToZoom : (DistToZoom + DeltaZoom) / DeltaZoom;

			//Zoom can get sticky, so this unlocks it
			if (DistToZoom == 0) { DistToZoom = 0.1; DistToMove = 0.9; }


			//Finds the scale of movement
			auto const ZoomScale = DistToMove + DistToZoom;

			if (bShowDebug && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Blue, FString::SanitizeFloat(DistToMove / ZoomScale));
				GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Blue, FString::SanitizeFloat(DistToZoom / ZoomScale));
			}

			//Basically lerps between the two vectors based on distance
			UpdatedLocation = (((MoveToPoint * DistToMove) + (ZoomToPoint * DistToZoom)) / ZoomScale);
		}
		else if (bMoveCamera)
		{
			UpdatedLocation = MoveToPoint;
		}
		else if (bZoomCamera)
		{
			UpdatedLocation = ZoomToPoint;
		}

		//Prevent movement to zero
		if (bMoveCamera || bZoomCamera)
		{
			if (UGameplayStatics::GetGlobalTimeDilation(GetWorld()) < 0.1)
			{
				//If the game is paused, halts camera movement
				UpdatedLocation = GetActorLocation();
			}
			//Sanitizes some of the smaller movements throwing an exception (For some reason)
			if (FVector::Dist(GetActorLocation(), UpdatedLocation) > 0.1)
			{
				SetActorLocation(UpdatedLocation);
			}
		}

	}

	//Debugging stuff
	if (bShowDebug && GEngine)
	{
		auto const ScreenDist = FVector2D::Distance(FurthestPoint, FVector2D(ScreenX, ScreenY));
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime, bMoveCamera ? FColor::Green : FColor::Red, bMoveCamera ? "Move Camera -True" : "Move Camera - False");
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime, bZoomCamera ? FColor::Green : FColor::Red, bZoomCamera ? "Zoom Camera - True" : "Zoom Camera - False");
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Yellow, "Furthest Point = " + FurthestPoint.ToString());
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Yellow, FVector2D(ScreenX, ScreenY).ToString());
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Orange, FString::SanitizeFloat( ScreenDist));

		GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, FString::SanitizeFloat(FVector::Dist(GetActorLocation(), TargetPoint)));

		auto const SkullyPos = Skully->GetActorLocation();
		auto const ShadowPos = Shadow->GetActorLocation();

		DrawDebugLine(
			GetWorld(),
			SkullyPos,
			ShadowPos,
			FColor::Blue
		);

		DrawDebugPoint(
			GetWorld(),
			AsyncCalcPoint,
			10,
			FColor::Red
		);
		DrawDebugPoint(
			GetWorld(),
			TargetPoint,
			10,
			FColor::Red
		);

		DrawDebugLine(
			GetWorld(),
			AsyncCalcPoint,
			CenterPoint,
			FColor::Green,
			false,
			-1,
			0,
			3
		);
		DrawDebugPoint(
			GetWorld(),
			CenterPoint,
			10,
			FColor::Green
		);
		DrawDebugSphere(
			GetWorld(),
			Shadow->GetActorLocation(),
			GoalDist,
			30,
			FColor::Black
		);

		DrawDebugLine(
			GetWorld(),
			GetActorLocation(),
			TargetPoint,
			FColor::Yellow
		);
	}

	
}

void ADynamicCameraController::Destroyed()
{
	bRunTask = false;
}

