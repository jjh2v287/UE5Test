// Copyright SweejTech Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "AudioInspectorStyle.h"

class FAudioInspectorCommands : public TCommands<FAudioInspectorCommands>
{
public:

	FAudioInspectorCommands()
		: TCommands<FAudioInspectorCommands>(TEXT("AudioInspector"), NSLOCTEXT("Contexts", "AudioInspector", "Audio Inspector Plugin"), NAME_None, FAudioInspectorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};