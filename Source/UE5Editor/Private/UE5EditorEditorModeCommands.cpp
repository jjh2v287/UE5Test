// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5EditorEditorModeCommands.h"
#include "UE5EditorEditorMode.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "UE5EditorEditorModeCommands"

FUE5EditorEditorModeCommands::FUE5EditorEditorModeCommands()
	: TCommands<FUE5EditorEditorModeCommands>("UE5EditorEditorMode",
		NSLOCTEXT("UE5EditorEditorMode", "UE5EditorEditorModeCommands", "UE5Editor Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FUE5EditorEditorModeCommands::RegisterCommands()
{
	TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);

	UI_COMMAND(SimpleTool, "Show Actor Info", "Opens message box with info about a clicked actor", EUserInterfaceActionType::Button, FInputChord());
	ToolCommands.Add(SimpleTool);

	UI_COMMAND(InteractiveTool, "Measure Distance", "Measures distance between 2 points (click to set origin, shift-click to set end point)", EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(InteractiveTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FUE5EditorEditorModeCommands::GetCommands()
{
	return FUE5EditorEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
