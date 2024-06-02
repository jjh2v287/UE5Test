// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#include "BlueprintProEditor.h"
#include "EdGraphUtilities.h"
#include "K2BlueprintGraphPanelPinFactory.h"



#define LOCTEXT_NAMESPACE "FBlueprintProEditorModule"

void FBlueprintProEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	TSharedPtr<FK2BlueprintGraphPanelPinFactory> BlueprintGraphPanelPinFactory = MakeShareable(new FK2BlueprintGraphPanelPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(BlueprintGraphPanelPinFactory);
}

void FBlueprintProEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintProEditorModule, BlueprintProEditor)