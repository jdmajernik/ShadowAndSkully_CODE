// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#if WITH_EDITORONLY_DATA
	#include "LevelEditor.h"
	#include "ILevelViewport.h"
	#include "EditorViewportClient.h"
	#include "SceneView.h"
#endif


#include "DevTools.generated.h"

/**
 * 
 */



// -------------------
// |   UStructs      |
// -------------------

USTRUCT()
struct FWidgetHandler
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	void RefreshScreen();

	void ClearScreen(TSharedRef<SWidget> Widget);
	void DrawToScreen(TSharedRef<SWidget> Widget);

	FEditorViewportClient* VClient;
	FSceneView* View;
	TSharedPtr<ILevelViewport> EditorViewport = nullptr;

	TArray<TSharedRef<SWidget>> InstUIElements;
#endif

};

USTRUCT()
struct FActorHighlightData
{
	//Contains all the data needed for each highlighted actor (allows for multiple shapes/colors/etc for multiple actor types)
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Actor")
		TSubclassOf<AActor> HighlightedActorType;

	UPROPERTY(EditAnywhere, Category = "Radius")
		int RadiusMax = 240;
	UPROPERTY(EditAnywhere, Category = "Radius")
		int RadiusMin = 80;

	UPROPERTY(EditAnywhere, Category = "Line Style")
		FColor LineColor = FColor::White;
	UPROPERTY(EditAnywhere, Category = "Line Style")
		float LineThickness = 1.0;

	UPROPERTY(EditAnywhere, Category = "Shape")
		int NumSides = 4;
	UPROPERTY(EditAnywhere, Category = "Shape")
		int RotationOffset = 0;
		
};

// --------------------
// | SWidget Classes  |
// --------------------
class SNGonHighlight : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNGonHighlight) {}

	SLATE_ARGUMENT(FVector2D, ScreenPos)
	SLATE_ARGUMENT(int, sideLength)
	SLATE_ARGUMENT(FColor, LineColor)
	SLATE_ARGUMENT(float, LineThickness)
	SLATE_ARGUMENT(int, sides)
	SLATE_ARGUMENT(int, RotationOffset)

		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	FVector2D ScreenPos = FVector2D(-100, -100);
	int sideLength = 124;
	FColor LineColor = FColor::White;
	float LineThickness = 1.0;
	int sides = 0;
	int RotationOffset = 0;
};


UCLASS()
class SHADOWANDSKULLY_API UDevTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

};

UCLASS()
class SHADOWANDSKULLY_API AActorHighlighter : public AActor
{
	GENERATED_BODY()
public:
	AActorHighlighter();

	UFUNCTION(CallInEditor, Category = "Actor Highlighter")
		void Activate();
	UFUNCTION(CallInEditor, Category = "Actor Highlighter")
		void Deactivate();
protected:
	UPROPERTY(EditAnywhere, Category = "Actor Highlighter")
		TArray<FActorHighlightData> HighlightedActors;

	bool IsActive = false;
	FWidgetHandler WidgetHandler;

	UPROPERTY(VisibleAnywhere, Category = "Actor Highlighter")
	TArray<AActor*> TrackedActors;

	virtual void BeginPlay() override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	void DisplayUI();
	void FindScreenPos(TArray<FVector2D> &ScreenPos);
	void FindObjects();

public:
	virtual void Tick(float DeltaSeconds) override;
};

