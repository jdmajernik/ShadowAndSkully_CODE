// Fill out your copyright notice in the Description page of Project Settings.


#include "DevTools.h"

#include "ModuleManager.h"
#include "EngineUtils.h"

/*
 * ------------------------Widget Handler Struct---------------------------------
 */
#if WITH_EDITORONLY_DATA
void FWidgetHandler::RefreshScreen()
{
	
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));

	EditorViewport = LevelEditor.GetFirstActiveViewport();

	VClient = static_cast<FEditorViewportClient*>(EditorViewport->GetActiveViewport()->GetClient());

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		VClient->Viewport,
		VClient->GetScene(),
		VClient->EngineShowFlags
	));

	View = VClient->CalcSceneView(&ViewFamily);


}

void FWidgetHandler::ClearScreen(TSharedRef<SWidget> Widget)
{
	RefreshScreen();
	EditorViewport->RemoveOverlayWidget(Widget);
}

void FWidgetHandler::DrawToScreen(TSharedRef<SWidget> Widget)
{
	RefreshScreen();
	EditorViewport->AddOverlayWidget(Widget);
}
#endif
/*
 * ------------------------NGon Highlight Widget---------------------------------
 */

void SNGonHighlight::Construct(const FArguments& InArgs)
{
	ScreenPos = InArgs._ScreenPos;
	sideLength = InArgs._sideLength;
	LineColor = InArgs._LineColor;
	LineThickness = InArgs._LineThickness;
	sides = InArgs._sides;
	RotationOffset = InArgs._RotationOffset;
	
}

int32 SNGonHighlight::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	auto const radius = float(sideLength) / 2;
	TArray<FVector2D> Polygon;

	float const Theta = 360 / sides;
	float const RotOffset = (Theta / 2) + RotationOffset; //Rotates the shape to look correct

	float const DegToRad = PI / 360;

	for(int i = 0; i<=sides; i++)
	{
		const float GenTheta = (RotOffset + (Theta * i)) * DegToRad; //the generated theta for this point
		const float GenX = (cos(GenTheta) * radius) + ScreenPos.X;
		const float GenY = (sin(GenTheta) * radius) + ScreenPos.Y;

		Polygon.Add(FVector2D(GenX, GenY));
	}

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Polygon, ESlateDrawEffect::None, LineColor, true, LineThickness);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}



/*
 * ------------------------ACTOR HIGHLIGHT CONTROLLER---------------------------------
 */
AActorHighlighter::AActorHighlighter()
{
	if (WITH_EDITOR)
	{
		//this keeps it in the editor only (Deactivates for builds)
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
	}
}
void AActorHighlighter::Activate()
{
	FindObjects();
//	IsActive = true;
}

void AActorHighlighter::Deactivate()
{
	IsActive = false;
}

void AActorHighlighter::BeginPlay()
{
	Super::BeginPlay();
}

bool AActorHighlighter::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AActorHighlighter::DisplayUI()
{
	TArray<FVector2D> ScreenPos;
	FindScreenPos(ScreenPos);
}

void AActorHighlighter::FindScreenPos(TArray<FVector2D> &ScreenPos)
{
#if WITH_EDITORONLY_DATA
	ScreenPos.Empty();
	for (AActor* TrackedActor : TrackedActors)
	{
		FVector2D FoundPos;
		WidgetHandler.View->WorldToPixel(TrackedActor->GetActorLocation(), FoundPos);
		ScreenPos.Add(FoundPos);
	}
#endif
}

void AActorHighlighter::FindObjects()
{
	TrackedActors.Empty();
	for (FActorHighlightData ActorData : HighlightedActors)
	{
		auto const ClassType = (*ActorData.HighlightedActorType);

		for (TActorIterator<AActor> TrackedActorItr(GetWorld()); TrackedActorItr; ++TrackedActorItr)
		{
			TrackedActors.Add(*TrackedActorItr);

			//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, TrackedActorItr->GetParentActor()->GetName());
			if(TrackedActorItr->StaticClass() == ClassType)
			{
				
			}
		}
		
	}
}

void AActorHighlighter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(IsActive)
	{
		DisplayUI();
	}
}

