// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

using UnrealBuildTool;

public class HTNEditor : ModuleRules
{
	public HTNEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"AssetTools",
				"Projects",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"SourceControl",
				"EditorStyle",
                "AIGraph",
                "GraphEditor",
                "Kismet",
                "KismetWidgets",
                "GameplayTasks",
                "AIModule",
                "ToolMenus",
                "PropertyEditor",
                "HTN"
			}
		);
	}
}
