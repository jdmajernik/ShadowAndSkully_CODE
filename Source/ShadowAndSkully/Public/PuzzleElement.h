// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//#include "LevelEditor.h"
#include "DevTools.h"
#include "ShadowMechanics.h"

#include "PuzzleElement.generated.h"

class APuzzleThinker;

UCLASS()
class SHADOWANDSKULLY_API APuzzleElement : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APuzzleElement();

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadWrite, Category = "Puzzle Data")
		bool State = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void UpdateUI();



public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

//--------------------
UCLASS()
class SHADOWANDSKULLY_API APuzzleDoer : public APuzzleElement
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APuzzleDoer();

	//Lever Activation
	UFUNCTION(BlueprintNativeEvent)
		void ActivateLever(bool NewState);

	UPROPERTY(VisibleAnywhere, SaveGame, Category = "Puzzle Data")
		TArray<APuzzleThinker*> LinkedThinkers;

	
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};


UCLASS()
class SHADOWANDSKULLY_API APuzzleThinker : public APuzzleElement
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APuzzleThinker();

	UPROPERTY(EditAnywhere, SaveGame, Category = "Puzzle Data")
		TArray<APuzzleDoer*> ActivateItems;

	UPROPERTY(EditAnywhere, Category = "Puzzle Connection Visualizer")
		FColor LineColor = FColor::Red;//The color of the Debug Lines
	UPROPERTY(EditAnywhere, Category = "Puzzle Connection Visualizer")
		float ArrowSize = 5000;//The Size of the Arrow displayed
	UPROPERTY(EditAnywhere, Category = "Puzzle Connection Visualizer")
		float LineThickness = 8; //The overall thickness of the lines
	UPROPERTY(EditAnywhere, Category = "Puzzle Connection Visualizer")
		float NullPointCounterScale = 100; //The overall thickness of the lines
	UPROPERTY(EditAnywhere, Category = "Puzzle Connection Visualizer")
		float NullPointDrawSize = 30; //The overall thickness of the lines
	UPROPERTY(EditAnywhere, Category = "Puzzle Connection Visualizer")
		float DrawPriority = 1; //The overall thickness of the lines

	virtual	void OnSelectedInEditor(UObject* Object);

	virtual void UseItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Puzzle Connection Visualizer")
		bool bSelected = false;

	bool ShouldTickIfViewportsOnly() const override;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};


UCLASS()
class SHADOWANDSKULLY_API ALever : public APuzzleThinker
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALever();


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		void UseItem() override;
protected:
	// Called when the game starts or when spawned
	
	virtual void BeginPlay() override;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

UCLASS()
class SHADOWANDSKULLY_API ASoulPickup : public APuzzleThinker
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoulPickup();

	//UPROPERTY(VisibleAnywhere, Category = "Puzzle Data")
		//bool State = false;

	UFUNCTION(BlueprintNativeEvent)
		void UseItem() override;
protected:
	// Called when the game starts or when spawned

	virtual void BeginPlay() override;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

//EMPTY CLASS TO CHECK AGAINST IN CASTING
UCLASS()
class SHADOWANDSKULLY_API APressurePlate : public APuzzleThinker
{
	GENERATED_BODY()
};

//Single pressure plate
UCLASS()
class SHADOWANDSKULLY_API ALatchedPressurePlate : public APressurePlate
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
		void UseItem() override;
};

//Two paired pressure plates
UCLASS()
class SHADOWANDSKULLY_API APairedPressurePlate : public APressurePlate
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APairedPressurePlate();


	UPROPERTY(VisibleAnywhere, Category = "Puzzle Data")
		bool ItemState = false;

	AShadowMechanics* Player;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Puzzle Data")
		AActor* OverlapedActor;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		void UseItem() override;
	UFUNCTION(BlueprintNativeEvent)
		void StepOffPlate();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Puzzle Data")
		void CheckProperties();
#if WITH_EDITORONLY_DATA
	void PostEditChangeProperty(FPropertyChangedEvent& e) override;
#endif
	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadWrite, Category = "Puzzle Data")
		APairedPressurePlate* MasterPressurePlate = nullptr;
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Puzzle Data")
		APairedPressurePlate* SlavePressurePlate = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "Puzzle Data")
		bool bWaitingOnPair = false;

	UFUNCTION(BlueprintCallable, Category = "Puzzle Data")
		void ResetSkullyTarget();

	void OnSelectedInEditor(UObject* Object) override;
protected:
	// Called when the game starts or when spawned

	virtual void BeginPlay() override;




public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void Activate();
};

UCLASS()
class SHADOWANDSKULLY_API ASoulRecepticle : public ALever
{
	//Empty class to cast to (and use for future reference)
	GENERATED_BODY()
};

