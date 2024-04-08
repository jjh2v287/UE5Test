// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5EditorModule.h"
#include "UE5EditorEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "UE5EditorModule"

void FUE5EditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FUE5EditorEditorModeCommands::Register();
}

void FUE5EditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FUE5EditorEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUE5EditorModule, UE5EditorEditorMode)