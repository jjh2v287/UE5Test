// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAudioEngineSubsystem.h"
#include "AudioDevice.h"
#include "MetasoundSource.h"
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
	
	const Audio::FDeviceId CurrentDeviceId = GetAudioDeviceId();
	FAudioThread::RunCommandOnAudioThread([CurrentDeviceId]()
	{
		FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get();
		if(AudioDeviceManager == nullptr)
		{
			return;
		}
		
		const FAudioDeviceHandle DeviceHandle = AudioDeviceManager->GetAudioDevice(CurrentDeviceId);
		if (!DeviceHandle.IsValid())
		{
			return;
		}
		
		const FAudioDevice* AudioDevice = DeviceHandle.GetAudioDevice();
		if(AudioDevice == nullptr)
		{
			return;
		}

		const TArray<FActiveSound*> ActiveSounds = AudioDevice->GetActiveSounds();
		for (FActiveSound* ActiveSound : ActiveSounds)
		{
			const UMetaSoundSource* SoundBase = Cast<UMetaSoundSource>(ActiveSound->GetSound());
			if(SoundBase == nullptr)
			{
				continue;
			}

			const UWorld* WorldPtr = ActiveSound->GetWorld();
			if (WorldPtr == nullptr)
			{
				continue;
			}

			Audio::IParameterTransmitter* Transmitter = ActiveSound->GetTransmitter();
			if (Transmitter == nullptr)
			{
				continue;
			}

			TArray<FAudioParameter> ParamsToUpdate;
			const Audio::FParameterInterfacePtr OcclusionInterface = Audio::OcclusionInterface::GetInterface();
			const bool bImplementsOcclusion = SoundBase->ImplementsParameterInterface(OcclusionInterface);

			if(bImplementsOcclusion)
			{
				const int32 ClosestListenerIndex = ActiveSound->FindClosestListener();
				check(ClosestListenerIndex != INDEX_NONE);
				const FVector SoundLocation = ActiveSound->Transform.GetLocation();
				FVector ListenerLocation;
				AudioDevice->GetListenerPosition(ClosestListenerIndex, ListenerLocation, false);

				FCollisionQueryParams Params(SCENE_QUERY_STAT(SoundOcclusion), true);
				Params.AddIgnoredActor( ActiveSound->GetOwnerID() );
				const bool bIsOccluded = WorldPtr->LineTraceTestByChannel(SoundLocation, ListenerLocation, ECollisionChannel::ECC_Visibility, Params);

				ParamsToUpdate.Append(
			{
					{ Audio::OcclusionInterface::Inputs::Occlusion, bIsOccluded ? 0.3f : 1.0f },
				});
			}

			Transmitter->SetParameters(MoveTemp(ParamsToUpdate));
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
	} // namespace OcclusionInterface
#undef AUDIO_PARAMETER_INTERFACE_NAMESPACE
} // namespace Audio
#undef LOCTEXT_NAMESPACE