// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "TimeStretchMetaNodes.h"
#include "MetasoundFrontendRegistries.h"

#define LOCTEXT_NAMESPACE "FTimeStretchMetaNodesModule"

void FTimeStretchMetaNodesModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FMetasoundFrontendRegistryContainer::Get()->RegisterPendingNodes();
}

void FTimeStretchMetaNodesModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTimeStretchMetaNodesModule, TimeStretchMetaNodes)