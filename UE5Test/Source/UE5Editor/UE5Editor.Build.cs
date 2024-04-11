// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UE5Editor : ModuleRules
{
	public UE5Editor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OverridePackageType = PackageOverrideType.EngineDeveloper;
		PublicIncludePaths.AddRange(
			new string[] {
				"UE5Editor"
			}
			);
				
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"AnimGraph",
				"AnimGraphRuntime",
				"UE5Test"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UE5Test",
				"InputCore",
				"EditorFramework",
				"EditorStyle",
				"UnrealEd",
				"LevelEditor",
				"AnimGraph",
				"AnimGraphRuntime",
				"BlueprintGraph",
				"InteractiveToolsFramework",
				"EditorInteractiveToolsFramework"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
