// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UKNPC : ModuleRules
{
	public UKNPC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"NavigationSystem",
			"AIModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"UKNPC",
			"UKNPC/Variant_Platforming",
			"UKNPC/Variant_Platforming/Animation",
			"UKNPC/Variant_Combat",
			"UKNPC/Variant_Combat/AI",
			"UKNPC/Variant_Combat/Animation",
			"UKNPC/Variant_Combat/Gameplay",
			"UKNPC/Variant_Combat/Interfaces",
			"UKNPC/Variant_Combat/UI",
			"UKNPC/Variant_SideScrolling",
			"UKNPC/Variant_SideScrolling/AI",
			"UKNPC/Variant_SideScrolling/Gameplay",
			"UKNPC/Variant_SideScrolling/Interfaces",
			"UKNPC/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
