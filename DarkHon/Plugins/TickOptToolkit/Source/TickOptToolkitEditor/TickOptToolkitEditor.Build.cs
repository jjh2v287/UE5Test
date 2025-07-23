// Copyright 2021 Marcin Swiderski. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class TickOptToolkitEditor : ModuleRules
	{
		public TickOptToolkitEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

			PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
				"CoreUObject",
				"Engine",
			});
			
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd",
				"EditorStyle",
				"PropertyEditor",
				"InputCore",
				"SlateCore",
				"Slate",
				"GraphEditor",
				"TickOptToolkit",
			});
		}
	}
}
