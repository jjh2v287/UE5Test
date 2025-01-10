// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ZoneDetector : ModuleRules
{
	public ZoneDetector(ReadOnlyTargetRules Target) : base(Target)
	{
		// PrivateDependencyModuleNames.AddRange(new string[] { "BSPUtils" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
