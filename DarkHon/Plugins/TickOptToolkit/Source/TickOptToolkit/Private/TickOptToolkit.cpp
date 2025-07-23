// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#include "TickOptToolkit.h"
#include "TickOptToolkitSettings.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#endif

DEFINE_LOG_CATEGORY(LogTickOptToolkit);
DEFINE_STAT(STAT_TickOptToolkit);

IMPLEMENT_MODULE(FTickOptToolkit, TickOptToolkit)

#define LOCTEXT_NAMESPACE "TickOptToolkit"

const FName NAME_NiagaraComponent = "NiagaraComponent";

void FTickOptToolkit::StartupModule()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Tick Opt Toolkit",
			LOCTEXT("SettingsName", "Tick Opt Toolkit"),
			LOCTEXT("SettingsDescription", "Configure the Tick Opt Toolkit plugin"),
			GetMutableDefault<UTickOptToolkitSettings>());
	}
#endif // WITH_EDITOR
}

void FTickOptToolkit::ShutdownModule()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Tick Opt Toolkit");
	}
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE
