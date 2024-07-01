// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IAudioExtensionPlugin.h"
#include "AudioDevice.h"

// For creating our custom source data override plugin
class FSourceDataOverridePluginFactory : public IAudioSourceDataOverrideFactory
{
public:
	virtual FString GetDisplayName() override
	{
		static FString DisplayName = FString(TEXT("Modal Spatial"));
		return DisplayName;
	}
	
	virtual bool SupportsPlatform(const FString& PlatformName) override
	{
		// return PlatformName == FString(TEXT("Windows")) || PlatformName == FString(TEXT("Android"));
		return true;
	}

	/**
	* @return the UClass type of your settings for source data overrides. This allows us to only pass in user settings for your plugin.
	*/
	virtual UClass* GetCustomSourceDataOverrideSettingsClass() const override
	{
		return nullptr;
	}

	/**
	* @return a new instance of your source data override plugin, owned by a shared pointer.
	*/
	virtual TAudioSourceDataOverridePtr CreateNewSourceDataOverridePlugin(FAudioDevice* OwningDevice) override;
};


class FImpactSFXSynthModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	IAudioPluginFactory* GetPluginFactory(EAudioPlugin PluginType);
	
private:
	FSourceDataOverridePluginFactory SourceDataOverridePluginFactory;
};
