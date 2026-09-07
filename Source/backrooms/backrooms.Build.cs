// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class backrooms : ModuleRules
{
	public backrooms(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory);
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTags",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
            "UMG",
            "DeveloperSettings",
            "AudioMixer"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "UMGEditor", "AssetTools", "AssetRegistry", "KismetCompiler" });
        }

        // GAS is intentionally not included in the vertical-slice framework.
		// Add GameplayAbilities only after the design requires a larger ability/effect model.
	}
}
