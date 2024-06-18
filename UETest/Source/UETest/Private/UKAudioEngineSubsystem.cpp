// Copyright Kong Studios, Inc. All Rights Reserved.


#include "UKAudioEngineSubsystem.h"
#include "AudioDevice.h"
#include "UKAudioSettings.h"

bool UUKAudioEngineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void UUKAudioEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UUKAudioSettings* AudioSettings = GetMutableDefault<UUKAudioSettings>();
	check(AudioSettings);
	AudioSettings->RegisterParameterInterfaces();
}

void UUKAudioEngineSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UUKAudioEngineSubsystem::Update()
{
	Super::Update();
	
	Audio::FDeviceId CurrentDeviceId = GetAudioDeviceId();
	FAudioThread::RunCommandOnAudioThread([CurrentDeviceId]()
	{
		if (FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get())
		{
			FAudioDeviceHandle DeviceHandle = AudioDeviceManager->GetAudioDevice(CurrentDeviceId);
			if (DeviceHandle.IsValid())
			{
				FAudioDevice* AudioDevice = DeviceHandle.GetAudioDevice();
				for (FActiveSound* ActiveSound : AudioDevice->GetActiveSounds())
				{
					if (USoundBase* SoundBase = Cast<USoundBase>(ActiveSound->GetSound()))
					{
						if (Audio::IParameterTransmitter* Transmitter = ActiveSound->GetTransmitter())
						{
							Audio::FParameterInterfacePtr OcclusionInterface = Audio::OcclusionInterface::GetInterface();
							bool bImplementsOcclusion = SoundBase->ImplementsParameterInterface(OcclusionInterface);
							if(bImplementsOcclusion)
							{
								int32 ClosestListenerIndex = ActiveSound->FindClosestListener();
								check(ClosestListenerIndex != INDEX_NONE);
								FVector ListenerLocation;
								FVector SoundLocation = ActiveSound->Transform.GetLocation();
								const bool bAllowOverride = false;
								bool bIsOccluded = false;
								AudioDevice->GetListenerPosition(ClosestListenerIndex, ListenerLocation, bAllowOverride);

								TArray<FAudioParameter> ParamsToUpdate;
								if (UWorld* WorldPtr = ActiveSound->GetWorld())
								{
									FCollisionQueryParams Params(SCENE_QUERY_STAT(SoundOcclusion), true);
									Params.AddIgnoredActor( ActiveSound->GetOwnerID() );
									bIsOccluded = WorldPtr->LineTraceTestByChannel(SoundLocation, ListenerLocation, ECollisionChannel::ECC_Visibility, Params);

									ParamsToUpdate.Append(
								{
										{ Audio::OcclusionInterface::Inputs::Occlusion, bIsOccluded ? 0.3f : 1.0f },
									});
								}
								
								Transmitter->SetParameters(MoveTemp(ParamsToUpdate));
							}
						}
					}
				}
			}
		}
	});
}

#define LOCTEXT_NAMESPACE "AudioParameterInterface"
#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "UK.Occlusion"
namespace Audio
{
	namespace OcclusionInterface
	{
		const FName Name = AUDIO_PARAMETER_INTERFACE_NAMESPACE;

		namespace Inputs
		{
			const FName Occlusion = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Occlusion");
		} // namespace Inputs

		Audio::FParameterInterfacePtr GetInterface()
		{
			struct FInterface : public Audio::FParameterInterface
			{
				FInterface()
					: FParameterInterface(OcclusionInterface::Name, { 1, 0 })
				{
					Inputs =
					{
						{
							FText(),
							NSLOCTEXT("AudioGeneratorInterface_Attenuation", "DistanceDescription", "Distance between listener and sound location in game units."),
							FName(),
							{ Inputs::Occlusion, 0.0f }
						}
					};
				}
			};

			static FParameterInterfacePtr InterfacePtr;
			if (!InterfacePtr.IsValid())
			{
				InterfacePtr = MakeShared<FInterface>();
			}

			return InterfacePtr;
		}
	} // namespace AttenuationInterface
#undef AUDIO_PARAMETER_INTERFACE_NAMESPACE
} // namespace Audio
#undef LOCTEXT_NAMESPACE