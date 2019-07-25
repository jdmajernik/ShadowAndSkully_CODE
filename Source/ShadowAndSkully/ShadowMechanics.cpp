// Fill out your copyright notice in the Description page of Project Settings.


#include "ShadowMechanics.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "PuzzleElement.h"

// Sets default values
AShadowMechanics::AShadowMechanics()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
}

// Called when the game starts or when spawned
void AShadowMechanics::BeginPlay()
{
	Super::BeginPlay();

	Goals.Empty();
	

	//Finds all Thinker objects in the level
	for (TActorIterator<APuzzleThinker> ThinkerItr(GetWorld()); ThinkerItr; ++ThinkerItr)
	{
		Goals.Add(*ThinkerItr); //Adds in every Puzzle Thinker in the level
	}
	for(TActorIterator<ASkullyController> SkullyItr(GetWorld()); SkullyItr; ++SkullyItr)
	{
		Skully = *SkullyItr;
	}

	SortGoals();
}


// Called every frame
void AShadowMechanics::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AShadowMechanics::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AShadowMechanics::FindGoals()
{
	TArray<APuzzleThinker*> ClosestItems;
	for(int i = 0; i<5; i++)
	{
		float Dist = FVector::Dist(GetActorLocation(), Goals[i]->GetActorLocation());
		if(Dist < MaxDistance)
		{
			ClosestItems.Add(Goals[i]);
		}
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);


	FVector TopRight, BottomRight, TopLeft, BottomLeft;

	FVector DumpVec;

	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);

	PlayerController->DeprojectScreenPositionToWorld(0,0,TopLeft,DumpVec);
	PlayerController->DeprojectScreenPositionToWorld(ViewportSize.X, 0, TopRight, DumpVec);
	PlayerController->DeprojectScreenPositionToWorld(0, ViewportSize.Y, BottomLeft, DumpVec);
	PlayerController->DeprojectScreenPositionToWorld(ViewportSize.X, ViewportSize.Y, BottomRight, DumpVec);
}

void AShadowMechanics::SortGoals()
{
	//Sorts every 'thinker' in the level based on distance to shadow
	FVector thisLoc = GetActorLocation();
	Goals.StableSort([thisLoc](APuzzleThinker& LHS, APuzzleThinker& RHS)
		{
			const float FirstDist = FVector::Dist(LHS.GetActorLocation(), thisLoc);
			const float SecondDist = FVector::Dist(RHS.GetActorLocation(), thisLoc);

			return FirstDist < SecondDist;
		});
}
