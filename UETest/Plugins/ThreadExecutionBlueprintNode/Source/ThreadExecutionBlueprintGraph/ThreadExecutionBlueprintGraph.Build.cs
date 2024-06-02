using UnrealBuildTool;

public class ThreadExecutionBlueprintGraph : ModuleRules
{
    public ThreadExecutionBlueprintGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "KismetCompiler",
                "UnrealEd",
                "BlueprintGraph",
                "GraphEditor",
            }
        );
    }
}