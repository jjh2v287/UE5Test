// Copyright 2021 Marcin Swiderski. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class TickOptToolkit : ModuleRules
	{
		public TickOptToolkit(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

			PublicIncludePaths.AddRange(new string[] {
				Path.Combine(ModuleDirectory, "Public"),
				Path.Combine(ModuleDirectory, "Classes") });

			PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
				"CoreUObject",
				"Engine"
			});
		}
	}
}
