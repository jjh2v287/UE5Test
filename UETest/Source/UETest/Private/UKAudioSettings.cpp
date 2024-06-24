// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAudioSettings.h"

#include "MetasoundFrontendRegistryContainer.h"
#include "UKAudioEngineSubsystem.h"

UUKAudioSettings::UUKAudioSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("UKAudio");
	bParameterInterfacesRegistered = false;
}

void UUKAudioSettings::RegisterParameterInterfaces()
{
	if(!bParameterInterfacesRegistered)
	{
		bParameterInterfacesRegistered = true;
		UE_LOG(LogAudio, Display, TEXT("Registering Audio::OcclusionInterface::GetInterface..."));
		Audio::IAudioParameterInterfaceRegistry& InterfaceRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
		InterfaceRegistry.RegisterInterface(Audio::OcclusionInterface::GetInterface());
		InterfaceRegistry.RegisterInterface(Audio::NavOcclusionInterface::GetInterface());

		// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
		FMetasoundFrontendRegistryContainer::Get()->RegisterPendingNodes();
	}
}
