// Copyright Epic Games, Inc. All Rights Reserved.

#include "UKPlatformToolkitEditorCommands.h"

#define LOCTEXT_NAMESPACE "FUKPlatformToolkitEditorModule"

void FUKPlatformToolkitEditorCommands::RegisterCommands()
{
	UI_COMMAND(PlatformGroupBoxAction, "UKPlatformTool Box Group", "Execute UKPlatformTool action", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(PlatformGroupSphereAction, "UKPlatformTool Sphere Group", "Execute UKPlatformTool action", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(PlatformUnGroupAction, "UKPlatformTool UnGroup", "Execute UKPlatformTool action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
