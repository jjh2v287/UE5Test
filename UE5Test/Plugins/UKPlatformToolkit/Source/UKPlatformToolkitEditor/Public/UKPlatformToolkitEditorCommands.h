// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "UKPlatformToolkitEditorStyle.h"

class FUKPlatformToolkitEditorCommands : public TCommands<FUKPlatformToolkitEditorCommands>
{
public:

	FUKPlatformToolkitEditorCommands()
		: TCommands<FUKPlatformToolkitEditorCommands>(TEXT("UKPlatformToolkitEditor"), NSLOCTEXT("Contexts", "UKPlatformToolkitEditor", "UKPlatformToolkitEditor Plugin"), NAME_None, FUKPlatformToolkitEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PlatformGroupSphereAction;
	TSharedPtr< FUICommandInfo > PlatformGroupBoxAction;
	TSharedPtr< FUICommandInfo > PlatformUnGroupAction;
};
