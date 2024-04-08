// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"

class FToolBarBuilder;
class FMenuBuilder;

class FUKPlatformToolkitEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OnDeleteActorsBegin();
	void OnDeleteActorsEnd();
	void PlatformGroup(bool IsBox);
	void PlatformUnGroup();
private:

	void RegisterMenus();

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
