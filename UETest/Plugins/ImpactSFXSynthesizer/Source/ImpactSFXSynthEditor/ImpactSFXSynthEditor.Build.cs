// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

using UnrealBuildTool;

public class ImpactSFXSynthEditor : ModuleRules
{
    public ImpactSFXSynthEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd",
            "ImpactSFXSynth",
            "SignalProcessing",
            "ContentBrowser"
        });
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AudioEditor",
                "AudioExtensions",
                "AudioMixer",
                "Core",
                "CoreUObject",
                "CurveEditor",
                "Engine",
                "EditorFramework",
                "EditorWidgets",
                "GameProjectGeneration",
                "InputCore",
                "PropertyEditor",
                "SequenceRecorder",
                "Slate",
                "SlateCore",
                "ToolWidgets",
                "ToolMenus",
                "Json", 
                "JsonUtilities", 
                "EditorScriptingUtilities"
            }
        );
        
        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "AssetTools",
                "AssetRegistry"
            });

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
                "AssetTools",
                "AssetRegistry"
            });

        PublicDefinitions.AddRange(
            new string[] {
                "BUILDING_STATIC"
            }
        );
    }
}