// Copyright SweejTech Ltd. All Rights Reserved.

#include "AudioInspectorCommands.h"

#define LOCTEXT_NAMESPACE "FAudioInspectorModule"

void FAudioInspectorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Audio Inspector", "Bring up a SweejTech Audio Inspector tab window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
