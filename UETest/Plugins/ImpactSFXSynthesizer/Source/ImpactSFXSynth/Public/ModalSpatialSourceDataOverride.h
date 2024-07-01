// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "AudioDevice.h"

class UDynamicReverbComponent;

class FModalSpatialSourceDataOverride : public IAudioSourceDataOverride
{
public:
    FModalSpatialSourceDataOverride();

    /** Initializes the source data override plugin with the given buffer length. */
    virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
    
    /** Called when a source is assigned to a voice. */
    virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USourceDataOverridePluginSourceSettingsBase* InSettings) override;
    
    /** Called when a source is done playing and is released. */
    virtual void OnReleaseSource(const uint32 SourceId) override;

    /** Allows this plugin to override any source data. Called per audio source before any other parameters are updated on sound sources. */
    virtual void GetSourceDataOverrides(const uint32 SourceId, const FTransform& InListenerTransform, FWaveInstance* InOutWaveInstance) override;

    /** Called when all sources have finished processing. */
    virtual void OnAllSourcesProcessed() override;

private:
    
    UDynamicReverbComponent* GetNearestDynamicReverbComponent(const FWaveInstance* InOutWaveInstance);
};