// Copyright 2023, SweejTech Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

class FToolBarBuilder;
class FMenuBuilder; 
class SAudioInspectorEditorWindow;
class SWindow;

class FAudioInspectorModule : public IModuleInterface
{
public:

	static FAudioInspectorModule& GetModule();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

	void SetUpdateInterval(float UpdateInterval);

	void RebuildActiveSoundsColumns();
	
private:

	void OnMainFrameCreationFinished(TSharedPtr<SWindow> Window, bool bShouldShowProjectDialogAtStartup);

	void OnReloadComplete(EReloadCompleteReason Reason);

	void RegisterMenus();

	void ClearSlateReferences();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	TSharedPtr < SDockTab > AudioInspectorDockTab;

	TSharedPtr< SAudioInspectorEditorWindow > AudioInspectorEditorWindow;
};
