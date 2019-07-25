// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class ShadowAndSkully : ModuleRules
{
	public ShadowAndSkully(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //PrivateIncludePaths.Add("LoadingScreen/Private");

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "MoviePlayer", "Engine", "InputCore", "UMG", "Slate", "SlateCore","NavigationSystem"  });
        if (Target.bBuildEditor == true)
        {
            PublicDependencyModuleNames.AddRange(new string[] {"UnrealEd"});
        }
        
        PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
