// Copyright Epic Games, Inc. All Rights Reserved.

#include "UETestEditor.h"

#include "MetasoundFrontendRegistryContainer.h"
#include "UKAudioSettings.h"

#define LOCTEXT_NAMESPACE "FUETestEditorModule"

void FUETestEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	UUKAudioSettings* AudioSettings = GetMutableDefault<UUKAudioSettings>();
	check(AudioSettings);
	AudioSettings->RegisterParameterInterfaces();
}

void FUETestEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUETestEditorModule, UETestEditor)