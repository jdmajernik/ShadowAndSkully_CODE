// Fill out your copyright notice in the Description page of Project Settings.


#include "MinotaurMechanics.h"
#include "TimerManager.h"
#include "SkullyMechanics.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DestructableObject.h"

DEFINE_LOG_CATEGORY(LogMinotaurDebug);
// Sets default values
AMinotaurMechanics::AMinotaurMechanics()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMinotaurMechanics::BeginPlay()
{
	Super::BeginPlay();

	PatrolWalkSpeed = PatrolSpeed;

	IgnoredActors.Add(this);
}

#pragma region
void AMinotaurMechanics::OnAttack()
{
	bReturnToPatrol = true;
	if(bStartChargeLatch)
	{
		//pauses before charging
		GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &AMinotaurMechanics::OnChargeWaitComplete, ChargeWait);
		bStartChargeLatch = false;
		bWaitingToCharge = true;
		StartCharge();
	}
	else
	{
		if (!bWaitingToCharge)
		{
			OnCharge();
		}
	}
}

void AMinotaurMechanics::OnChargeWaitComplete_Implementation()
{
	//Reset Variables
	bWaitingToCharge = false;
	bChargeHit = false;
	ChargeDeltaTime = 0;

	StartChargeLocation = GetActorLocation();

	//calculate the Charge Direction
	FVector ChargeDirection = Skully->GetActorLocation() - GetActorLocation();
	ChargeDirection.Normalize();//Gets a normalized vector for the direction of Skully
	ChargeDirection *= ChargeDistance;

	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Skully->GetActorLocation()));//Looks at skully/Charge direction

// -----------------
// | Hit Variables |
// -----------------
	FHitResult OutHit;


	FCollisionQueryParams ChargeHitParams;
	ChargeHitParams.AddIgnoredActors(IgnoredActors);

	//Scanning for all objects that block movement
	FCollisionObjectQueryParams HitCollisionTypes;
	HitCollisionTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);//Static Objects
	HitCollisionTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel1);//'Normal' Objects
	HitCollisionTypes.AddObjectTypesToQuery(ECC_Destructible);


	GetWorld()->SweepSingleByObjectType(
		OutHit,
		GetActorLocation(),
		ChargeDirection + GetActorLocation(),
		GetActorRotation().Quaternion(),
		HitCollisionTypes,
		FCollisionShape::MakeSphere(60), //The size of the capsule collider [static because of some intellisense bs]
		ChargeHitParams
	);

	

	if(bShowDebug)
	{
		DrawDebugLine(
			GetWorld(),
			GetActorLocation(),
			GetActorLocation() + ChargeDirection,
			FColor::Magenta,
			ChargeWait,
			-1,
			5
		);
	}

	if(OutHit.bBlockingHit)
	{
		ChargeLocation = OutHit.ImpactPoint;
		if(bShowDebug)
		{
			DrawDebugPoint(
				GetWorld(),
				OutHit.ImpactPoint,
				10,
				FColor::Emerald,
				false,
				5
			);
		}
		ChargeLocation.Z = GetActorLocation().Z;
	}
	else
	{
		ChargeLocation = ChargeDirection + GetActorLocation();
		ChargeLocation.Z = GetActorLocation().Z;
	}
}

void AMinotaurMechanics::OnCharge()
{
	static float ChargeEndTime = (FVector::Dist(GetActorLocation(), ChargeLocation) / ChargeSpeed);
	if(FVector::Dist(GetActorLocation(), ChargeLocation) < DistanceToChargeEnd || ChargeDeltaTime >= 1 || bChargeHit)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Done Charging!");
		CurrentState = EBadSoulStates::Bs_Freeze;
		

		// -------------------------------------
		// | Destructible Object Hit Variables |
		// -------------------------------------
		TArray<FHitResult> OutHits;

		FCollisionQueryParams ChargeHitParams;
		ChargeHitParams.AddIgnoredActors(IgnoredActors);

		//Scanning for all objects that block movement
		FCollisionObjectQueryParams HitCollisionTypes;
		HitCollisionTypes.AddObjectTypesToQuery(ECollisionChannel::ECC_Destructible);//Static Objects


		GetWorld()->SweepMultiByObjectType(
			OutHits,
			GetActorLocation(),
			GetActorForwardVector() + GetActorLocation(),
			GetActorRotation().Quaternion(),
			HitCollisionTypes,
			FCollisionShape::MakeSphere(DamageRadius), //The size of the capsule collider [static because of some intellisense bs]
			ChargeHitParams
		);

		for (auto Hit : OutHits)
		{
			auto DestObj = Cast<ADestructableObject>(Hit.Actor);
			if (DestObj)
			{
				DestObj->ExplodeObject();
				UE_LOG(LogMinotaurDebug, Warning, TEXT("Destroying Object"));
			}
		}
		OnChargeCompleted();//Calls the end charge animation changes and effects

		bStartChargeLatch = true;
		return;
	}

	//Charge
	//ChargeDeltaTime += ChargeEndTime*LoggedDeltaTime;
	ChargeDeltaTime += LoggedDeltaTime/3;
	AddMovementInput(UKismetMathLibrary::GetForwardVector(GetActorRotation()), ChargeSpeed);
	GetCharacterMovement()->MaxWalkSpeed = 4400;
	//FVector NextLocation = FMath::Lerp(StartChargeLocation, ChargeLocation, ChargeDeltaTime);
	GEngine->AddOnScreenDebugMessage(-1, LoggedDeltaTime, FColor::Red, "Charging!");
	//GEngine->AddOnScreenDebugMessage(-1, LoggedDeltaTime, FColor::Orange, NextLocation.ToString());
	GEngine->AddOnScreenDebugMessage(-1, LoggedDeltaTime, FColor::Cyan, FString::SanitizeFloat(ChargeDeltaTime));
	//SetActorLocation(NextLocation);
}

void AMinotaurMechanics::OnChargeCompleted_Implementation()
{
	GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &AMinotaurMechanics::OnChargeEndWaitCompleted, EndChargeWait);
}
#pragma endregion Charge Functions

void AMinotaurMechanics::OnChargeEndWaitCompleted()
{
	//Resets variables and returns to patrol state
	CurrentState = EBadSoulStates::Bs_Patrol;
	GetCharacterMovement()->MaxWalkSpeed = 300;
	bReturnToPatrol = false;
}

// Called every frame
void AMinotaurMechanics::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMinotaurMechanics::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

