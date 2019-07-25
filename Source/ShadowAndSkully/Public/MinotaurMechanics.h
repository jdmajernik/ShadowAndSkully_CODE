// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BadSoulMechanics.h"
#include "Engine/Engine.h"
#include "MinotaurMechanics.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMinotaurDebug, Log, All);

UCLASS()
class SHADOWANDSKULLY_API AMinotaurMechanics : public ABadSoulMechanics
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMinotaurMechanics();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	

	FTimerHandle UnusedHandle;
//-------- Charging Variables --------

	UPROPERTY(VisibleAnywhere, Category = "Minotaur AI")
		bool bStartChargeLatch = true;//The Latch for the charge variable setup
	UPROPERTY(VisibleAnywhere, Category = "Minotaur AI")
		bool bWaitingToCharge = true;//The timer latch for the Minotaur's charge charge-up
	UPROPERTY(VisibleAnywhere, Category = "Minotaur AI")
		bool bChargeHit = true;//If the minotaur hits anything during charge, stops charge
	UPROPERTY(EditAnywhere, Category = "Minotaur AI")
		float PatrolSpeed = 300;//The speed with which the minotaur charges at the player
	

	UPROPERTY(EditAnywhere, Category = "Minotaur AI|Charge")
		float ChargeWait = 1;//The wait time (in seconds) before charging
	UPROPERTY(EditAnywhere, Category = "Minotaur AI|Charge")
		float EndChargeWait = 2;//The wait time (in seconds) after charging, before resuming patrol function
	UPROPERTY(EditAnywhere, Category = "Minotaur AI|Charge")
		float ChargeSpeed = 800;//The speed with which the minotaur charges at the player
	UPROPERTY(EditAnywhere, Category = "Minotaur AI|Charge")
		float ChargeDistance = 1500;//The distance to check the charge direction
	UPROPERTY(EditAnywhere, Category = "Minotaur AI|Charge")
		float DistanceToChargeEnd = 100;//The distance to the charge location to stop charging
	UPROPERTY(EditAnywhere, Category = "Minotaur AI|Charge")
		float DamageRadius = 400;//The radius that the charge damages destructible items

	UPROPERTY(EditAnywhere, Category = "Minotaur AI|Charge")
		TSubclassOf<AActor> DestructibleBlueprintType;//The radius that the charge damages destructible items

	UPROPERTY()
		float ChargeDeltaTime = 0;//Delta time for lerping to charge location

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minotaur AI|Charge")
		FVector ChargeLocation;
	UPROPERTY(VisibleAnywhere, Category = "Minotaur AI|Charge")
		FVector StartChargeLocation;

	void OnAttack() override;

	UFUNCTION(BlueprintImplementableEvent)
		void StartCharge();
	UFUNCTION(BlueprintNativeEvent)
		void OnChargeWaitComplete();
	void OnCharge();
	UFUNCTION(BlueprintNativeEvent)
		void OnChargeCompleted();
	void OnChargeEndWaitCompleted();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
		FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override
	{
		if(Other->GetComponentsCollisionResponseToChannel(ECC_WorldStatic) || Other->GetComponentsCollisionResponseToChannel(ECC_WorldStatic))
		{
			bChargeHit = true;
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, Other->GetName());
		}
		
	}

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
