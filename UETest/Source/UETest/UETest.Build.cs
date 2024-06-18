// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UETest : ModuleRules
{
	public UETest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AnimGraphRuntime",
			"UMG",
			
			"AudioExtensions",
			"DeveloperSettings",
			// "MetasoundEngined",
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"AnimGraphRuntime", "AIModule",
		});
	}
}
