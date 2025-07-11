// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNCommands.h"
#include "HTNStyle.h"

#define LOCTEXT_NAMESPACE "FHTNModule"

FHTNCommonCommands::FHTNCommonCommands()
	: TCommands<FHTNCommonCommands>("HTNEditor.Common", LOCTEXT("Common", "Common"), NAME_None, FHTNStyle::GetStyleSetName())
{}

void FHTNCommonCommands::RegisterCommands()
{
	UI_COMMAND(SearchHTN, "Search", "Search this HTN.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
	UI_COMMAND(OpenPluginWindow, "HTN", "Bring up HTN window", EUserInterfaceActionType::Button, FInputChord());
}

FHTNDebuggerCommands::FHTNDebuggerCommands()
	: TCommands<FHTNDebuggerCommands>("HTNEditor.Debugger", LOCTEXT("Debugger", "Debugger"), NAME_None, FHTNStyle::GetStyleSetName())
{}

void FHTNDebuggerCommands::RegisterCommands()
{
	/*
	UI_COMMAND(BackInto, "Back: Into", "Show state from previous step, can go into subtrees", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(BackOver, "Back: Over", "Show state from previous step, don't go into subtrees", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ForwardInto, "Forward: Into", "Show state from next step, can go into subtrees", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ForwardOver, "Forward: Over", "Show state from next step, don't go into subtrees", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(StepOut, "Step Out", "Show state from next step, leave current subtree", EUserInterfaceActionType::Button, FInputChord());
	*/

	UI_COMMAND(PausePlaySession, "Pause", "Pause simulation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ResumePlaySession, "Resume", "Resume simulation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(StopPlaySession, "Stop", "Stop simulation", EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(CurrentValues, "Current", "View current values", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(ValuesOfSelectedNode, "Selected", "View values after the selected node in the current plan.", EUserInterfaceActionType::RadioButton, FInputChord());
}

FHTNBlackboardCommands::FHTNBlackboardCommands()
	: TCommands<FHTNBlackboardCommands>("HTNEditor.Blackboard", LOCTEXT("Blackboard", "Blackboard"), NAME_None, FHTNStyle::GetStyleSetName())
{}

void FHTNBlackboardCommands::RegisterCommands()
{
	UI_COMMAND(DeleteEntry, "Delete", "Delete this blackboard entry", EUserInterfaceActionType::Button, FInputChord(EKeys::Platform_Delete));
}

#undef LOCTEXT_NAMESPACE
