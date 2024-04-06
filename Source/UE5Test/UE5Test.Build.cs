// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UE5Test : ModuleRules
{
	public UE5Test(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimGraph",
			"BlueprintGraph"
		});

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AnimGraphRuntime" });
	}
}
