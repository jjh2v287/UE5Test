// Copyright Kong Studios, Inc. All Rights Reserved.


#include "UKAudioSettings.h"
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
	}
}
