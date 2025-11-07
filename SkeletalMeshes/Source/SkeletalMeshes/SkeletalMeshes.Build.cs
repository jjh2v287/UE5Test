// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SkeletalMeshes : ModuleRules
{
	public SkeletalMeshes(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SkeletalMeshes",
			"SkeletalMeshes/Variant_Platforming",
			"SkeletalMeshes/Variant_Platforming/Animation",
			"SkeletalMeshes/Variant_Combat",
			"SkeletalMeshes/Variant_Combat/AI",
			"SkeletalMeshes/Variant_Combat/Animation",
			"SkeletalMeshes/Variant_Combat/Gameplay",
			"SkeletalMeshes/Variant_Combat/Interfaces",
			"SkeletalMeshes/Variant_Combat/UI",
			"SkeletalMeshes/Variant_SideScrolling",
			"SkeletalMeshes/Variant_SideScrolling/AI",
			"SkeletalMeshes/Variant_SideScrolling/Gameplay",
			"SkeletalMeshes/Variant_SideScrolling/Interfaces",
			"SkeletalMeshes/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
