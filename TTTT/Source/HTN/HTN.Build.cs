// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

using UnrealBuildTool;

public class HTN : ModuleRules
{
	public HTN(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "GameplayTasks",
                "AIModule",
                "GameplayTags"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "NavigationSystem"
			}
        );
	}
}
