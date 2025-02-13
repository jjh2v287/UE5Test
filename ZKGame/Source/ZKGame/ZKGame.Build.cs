// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ZKGame : ModuleRules
{
	public ZKGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] 
		{	
			"ZKGame",
			"ZKGame/Core",
			"ZKGame/Component",
			"ZKGame/Ability",
		});
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			
			// Gameplay Abilities
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			
			// GameFeatures
			"GameFeatures",
			"ModularGameplay",
			
			//
			"StateTreeModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "GameplayCameras" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
