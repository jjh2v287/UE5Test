// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.


#include "BlueprintPro.h"

#define LOCTEXT_NAMESPACE "FBlueprintProModule"

void FBlueprintProModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FBlueprintProModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBlueprintProModule, BlueprintPro)