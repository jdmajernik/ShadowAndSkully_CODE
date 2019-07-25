// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UserWidget.h"
#include "MoviePlayer.h"
#include "Border.h"

#include "SaS_GameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartLoadingDelegate);

UCLASS()
class SHADOWANDSKULLY_API USaS_GameInstance : public UGameInstance
{
	GENERATED_BODY()
		

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
		TSubclassOf<class UUserWidget> wLoadingScreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
		UUserWidget* LoadingWidgetRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
		TArray<FText> LoadingTips;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
		float MinLoadTime = 3;//The minimum amount of time to display the load screen

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
		float MaxHelpScreen;//The minimum amount of time to display the load screen

	UPROPERTY(BlueprintAssignable, Category = "Loading")
		FStartLoadingDelegate LoadingHandler;

protected:
	TSharedPtr<class SWidget> WidgetFromClass;

	void Init() override
	{

		Super::Init();

		GetMoviePlayer()->OnPrepareLoadingScreen().AddUObject(this, &USaS_GameInstance::BeginLoadingScreen);
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USaS_GameInstance::EndLoadingScreen);
		
	}

	void BeginLoadingScreen()
	{
		if (!IsRunningDedicatedServer() && IsMoviePlayerEnabled())
		{
			FLoadingScreenAttributes LoadingScreen;

			LoadingScreen.MinimumLoadingScreenDisplayTime = 3;
			LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;

			LoadingWidgetRef = CreateWidget(this, wLoadingScreen);
			LoadingScreen.WidgetLoadingScreen = LoadingWidgetRef->TakeWidget();


			GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);

			if (LoadingHandler.IsBound())
			{
				LoadingHandler.Broadcast();
			}
		}
	}
	void EndLoadingScreen(UWorld* InWorld)
	{
	}
	void Shutdown() override
	{
		GetMoviePlayer()->OnPrepareLoadingScreen().RemoveAll(this);

		Super::Shutdown();
	}
};
