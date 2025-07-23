// Copyright (C) 2021-2025 Space Raccoon Game Studio. All rights reserved.

using UnrealBuildTool;

public class NPC_Optimizator : ModuleRules
{
	public NPC_Optimizator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"AIModule",
				"NavigationSystem",
				"HairStrandsCore",
				"Niagara"
			}
			);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd"
				}
			);
		}
    }
}
