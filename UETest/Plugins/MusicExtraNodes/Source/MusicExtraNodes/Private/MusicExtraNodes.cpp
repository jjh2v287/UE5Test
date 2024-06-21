// Copyright 2023 Davide Socol. All Rights Reserved.

#include "MusicExtraNodes.h"

#include "MetasoundFrontendRegistries.h"

#define LOCTEXT_NAMESPACE "FMusicExtrasModule"

void FMusicExtraNodesModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FMetasoundFrontendRegistryContainer::Get()->RegisterPendingNodes();
}

void FMusicExtraNodesModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMusicExtraNodesModule, MusicExtraNodes)