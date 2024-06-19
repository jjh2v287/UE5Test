// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAudioEngineSubsystem.h"
#include "AudioDevice.h"
#include "EngineUtils.h"
#include "MetasoundSource.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "UKAudioSettings.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavFilters/NavigationQueryFilter.h"

TMap<FTraceHandle, UUKAudioEngineSubsystem::FUKOcclusionAsyncTraceInfo> UUKAudioEngineSubsystem::TraceToActiveSoundMap;
TMap<FActiveSound*, UUKAudioEngineSubsystem::FUKOcclusionAsyncTraceCompleteInfo> UUKAudioEngineSubsystem::TraceCompleteHandleMap;
FTraceDelegate UUKAudioEngineSubsystem::ActiveSoundTraceDelegate;

TMap<uint32, UUKAudioEngineSubsystem::FUKNavOcclusionAsyncTraceInfo> UUKAudioEngineSubsystem::NavOcclusionMap;
TMap<FActiveSound*, UUKAudioEngineSubsystem::FUKNavOcclusionAsyncTraceCompleteInfo> UUKAudioEngineSubsystem::NavOcclusionCompleteMap;
FNavPathQueryDelegate UUKAudioEngineSubsystem::NavPathQueryDelegate;

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

	if (!ActiveSoundTraceDelegate.IsBound())
	{
		ActiveSoundTraceDelegate.BindStatic(&AsyncOcclusionTraceEnd);
	}

	if (!NavPathQueryDelegate.IsBound())
	{
		NavPathQueryDelegate.BindStatic(&AsyncNavOcclusionEnd);
	}
	
	TraceToActiveSoundMap.Reset();
	TraceCompleteHandleMap.Reset();

	NavOcclusionMap.Reset();
	NavOcclusionCompleteMap.Reset();
}

void UUKAudioEngineSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (ActiveSoundTraceDelegate.IsBound())
	{
		ActiveSoundTraceDelegate.Unbind();
	}

	if (NavPathQueryDelegate.IsBound())
	{
		NavPathQueryDelegate.Unbind();
	}
	
	TraceToActiveSoundMap.Reset();
	TraceCompleteHandleMap.Reset();

	NavOcclusionMap.Reset();
	NavOcclusionCompleteMap.Reset();
}

void UUKAudioEngineSubsystem::Update()
{
	Super::Update();
	
	const Audio::FDeviceId CurrentDeviceId = GetAudioDeviceId();
	FAudioThread::RunCommandOnAudioThread([CurrentDeviceId]()
	{
		FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get();
		const bool bNotValidAudioDeviceManager = AudioDeviceManager == nullptr;
		if (bNotValidAudioDeviceManager)
		{
			return;
		}
		
		const FAudioDeviceHandle DeviceHandle = AudioDeviceManager->GetAudioDevice(CurrentDeviceId);
		const bool bNotValidDeviceHandle = !DeviceHandle.IsValid(); 
		if (bNotValidDeviceHandle)
		{
			return;
		}
		
		const FAudioDevice* AudioDevice = DeviceHandle.GetAudioDevice();
		const bool bNotValidAudioDevice = AudioDevice == nullptr;
		if (bNotValidAudioDevice)
		{
			return;
		}

		const TArray<FActiveSound*> ActiveSounds = AudioDevice->GetActiveSounds();
		for (FActiveSound* ActiveSound : ActiveSounds)
		{
			const UMetaSoundSource* SoundBase = Cast<UMetaSoundSource>(ActiveSound->GetSound());
			const bool bNotValidSoundBase = SoundBase == nullptr;
			if (bNotValidSoundBase)
			{
				continue;
			}

			const UWorld* WorldPtr = ActiveSound->GetWorld();
			const bool bNotValidWorldPtr = WorldPtr == nullptr;
			if (bNotValidWorldPtr)
			{
				continue;
			}

			Audio::IParameterTransmitter* Transmitter = ActiveSound->GetTransmitter();
			const bool bNotValidTransmitter = Transmitter == nullptr;
			if (bNotValidTransmitter)
			{
				continue;
			}

			const int32 ClosestListenerIndex = ActiveSound->FindClosestListener();
			const bool bNotValidListener = ClosestListenerIndex == INDEX_NONE;
			if(bNotValidListener)
			{
				continue;
			}

			// Seting Sound And Listener World Location Info
			FVector ListenerLocation;
			const FVector SoundLocation = ActiveSound->Transform.GetLocation();
			AudioDevice->GetListenerPosition(ClosestListenerIndex, ListenerLocation, false);

			// Seting Meta Sound Parameter Value
			TArray<FAudioParameter> ParamsToUpdate;
			const Audio::FParameterInterfacePtr OcclusionInterface = Audio::OcclusionInterface::GetInterface();
			const Audio::FParameterInterfacePtr NavOcclusionInterface = Audio::NavOcclusionInterface::GetInterface();
			const bool bParameterOcclusion = SoundBase->ImplementsParameterInterface(OcclusionInterface);
			const bool bParameterNavOcclusion = SoundBase->ImplementsParameterInterface(NavOcclusionInterface);

			const bool bAllNotParameter = !bParameterOcclusion && !bParameterNavOcclusion; 
			if(bAllNotParameter)
			{
				continue;
			}
			
			if (bParameterOcclusion)
			{
				if(bAsync)
				{
					UUKAudioEngineSubsystem::AsyncOcclusionTraceStart(ActiveSound, SoundLocation, ListenerLocation);

					const FUKOcclusionAsyncTraceCompleteInfo* TraceComplete = TraceCompleteHandleMap.Find(ActiveSound);
					const bool bTaskComplete = TraceComplete && TraceComplete->bTaskComplete;
					if(bTaskComplete)
					{
						FUKOcclusionAsyncTraceInfo TraceDetails;
						if (TraceToActiveSoundMap.RemoveAndCopyValue(TraceComplete->TraceHandle, TraceDetails))
						{
							ParamsToUpdate.Append(
						{
								{ Audio::OcclusionInterface::Inputs::Occlusion, TraceComplete->bHit ? 1.0f : 0.0f },
							});

							TraceCompleteHandleMap.Remove(ActiveSound);
						}
					}
				}
				else
				{
					const float OcclusionRate = UUKAudioEngineSubsystem::GetOcclusionRate(ActiveSound, SoundLocation, ListenerLocation);
					ParamsToUpdate.Append(
				{
						{ Audio::OcclusionInterface::Inputs::Occlusion, OcclusionRate },
					});
				}
			}

			if(bParameterNavOcclusion)
			{
				if(bAsync)
				{
					UUKAudioEngineSubsystem::AsyncNavOcclusionStart(ActiveSound, SoundLocation, ListenerLocation);

					const FUKNavOcclusionAsyncTraceCompleteInfo* TraceComplete = NavOcclusionCompleteMap.Find(ActiveSound);
					const bool bTaskComplete = TraceComplete && TraceComplete->bTaskComplete;
					if(bTaskComplete)
					{
						FUKNavOcclusionAsyncTraceInfo TraceDetails;
						if (NavOcclusionMap.RemoveAndCopyValue(TraceComplete->AsynId, TraceDetails))
						{
							ParamsToUpdate.Append(
						{
								{ Audio::NavOcclusionInterface::Inputs::NavOcclusion, TraceComplete->OcclusionRate },
							});

							NavOcclusionCompleteMap.Remove(ActiveSound);
						}
					}
				}
				else
				{
					const float NavOcclusionRate = UUKAudioEngineSubsystem::GetNavOcclusionRate(ActiveSound, SoundLocation, ListenerLocation);
					ParamsToUpdate.Append(
				{
						{ Audio::NavOcclusionInterface::Inputs::NavOcclusion, NavOcclusionRate },
					});
				}
			}

			// Toss Meta Sound Parameter Value
			Transmitter->SetParameters(MoveTemp(ParamsToUpdate));
		}
	});
}

void UUKAudioEngineSubsystem::AsyncOcclusionTraceStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	bool bNotComplete = TraceCompleteHandleMap.Contains(ActiveSound);
	if(bNotComplete)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(AsyncSoundOcclusion), true);
	Params.AddIgnoredActor( ActiveSound->GetOwnerID() );
	
	FUKOcclusionAsyncTraceInfo TraceDetails;
	TraceDetails.AudioDeviceID = ActiveSound->AudioDevice->DeviceID;
	TraceDetails.ActiveSound = ActiveSound;
	FTraceHandle TraceHandle = ActiveSound->GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, SoundLocation, ListenerLocation, ECollisionChannel::ECC_Visibility, Params, FCollisionResponseParams::DefaultResponseParam, &ActiveSoundTraceDelegate);
	TraceToActiveSoundMap.Add(TraceHandle, TraceDetails);

	FUKOcclusionAsyncTraceCompleteInfo TraceComplete;
	TraceComplete.TraceHandle = TraceHandle;
	TraceComplete.bHit = false;
	TraceComplete.bTaskComplete = false;
	TraceCompleteHandleMap.Add(ActiveSound, TraceComplete);
}

void UUKAudioEngineSubsystem::AsyncOcclusionTraceEnd(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	// Look for any results that resulted in a blocking hit
	bool bFoundBlockingHit = false;
	for (const FHitResult& HitResult : TraceDatum.OutHits)
	{
		if (HitResult.bBlockingHit)
		{
			bFoundBlockingHit = true;
			break;
		}
	}

	FAudioThread::RunCommandOnAudioThread( [TraceHandle, bFoundBlockingHit]()
	{
		FUKOcclusionAsyncTraceInfo* TraceDetails = TraceToActiveSoundMap.Find(TraceHandle);
		const bool bNotValidTraceInfo = TraceDetails == nullptr;
		if (bNotValidTraceInfo)
		{
			return;
		}

		FUKOcclusionAsyncTraceCompleteInfo* TraceComplete = TraceCompleteHandleMap.Find(TraceDetails->ActiveSound);
		const bool bNotValidTraceCompleteInfo = TraceComplete == nullptr;
		if (bNotValidTraceCompleteInfo)
		{
			return;
		}

		TraceComplete->bHit = bFoundBlockingHit;
		TraceComplete->bTaskComplete = true;
	});
}

void UUKAudioEngineSubsystem::AsyncNavOcclusionStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	bool bNotComplete = NavOcclusionCompleteMap.Contains(ActiveSound);
	if(bNotComplete)
	{
		return;
	}
	
	UNavigationSystemV1* NavigationSystem = Cast<UNavigationSystemV1>(ActiveSound->GetWorld()->GetNavigationSystem());
	// FPathFindingQuery FindingQuery;
	FNavAgentProperties AgentProperties;
	ANavigationData* NavigationData = NULL;

	NavigationData = NavigationSystem->GetDefaultNavDataInstance();
	const FPathFindingQuery FindingQuery(nullptr, *NavigationData, SoundLocation, ListenerLocation, UNavigationQueryFilter::GetQueryFilter(*NavigationData, nullptr, nullptr));
	uint32 QueryID = NavigationSystem->FindPathAsync(AgentProperties, FindingQuery, NavPathQueryDelegate, EPathFindingMode::Regular);

	FUKNavOcclusionAsyncTraceInfo Info;
	Info.AsynId = QueryID;
	Info.ActiveSound = ActiveSound;
	NavOcclusionMap.Add(QueryID, Info);

	FUKNavOcclusionAsyncTraceCompleteInfo CompleteInfo;
	CompleteInfo.AsynId = QueryID;
	NavOcclusionCompleteMap.Add(ActiveSound, CompleteInfo);
}

void UUKAudioEngineSubsystem::AsyncNavOcclusionEnd(uint32 QueryID, ENavigationQueryResult::Type Result, FNavPathSharedPtr NavPath)
{
	FAudioThread::RunCommandOnAudioThread( [QueryID, NavPath]()
	{
		FUKNavOcclusionAsyncTraceInfo* Info = NavOcclusionMap.Find(QueryID);
		const bool bNotValidInfo = Info == nullptr;
		if(bNotValidInfo)
		{
			return;
		}
		
		FUKNavOcclusionAsyncTraceCompleteInfo* CompleteInfo= NavOcclusionCompleteMap.Find(Info->ActiveSound);
		const bool bNotValidCompleteInfo = CompleteInfo == nullptr;
		if(bNotValidCompleteInfo)
		{
			return;
		}
		
		const float PathLength = NavPath->GetLength();
		const float OcclusionRate = UKismetMathLibrary::MapRangeClamped(PathLength, 0.0f, 3600.0f, 0.0f, 1.0f);
		CompleteInfo->bTaskComplete = true;
		CompleteInfo->OcclusionRate = OcclusionRate;
	});
}

const float UUKAudioEngineSubsystem::GetOcclusionRate(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SoundOcclusion), true);
	Params.AddIgnoredActor( ActiveSound->GetOwnerID() );
	const bool bIsOccluded = ActiveSound->GetWorld()->LineTraceTestByChannel(SoundLocation, ListenerLocation, ECollisionChannel::ECC_Visibility, Params);
	const float OcclusionRate = bIsOccluded ? 1.0f : 0.0f;
	return OcclusionRate;
}

const float UUKAudioEngineSubsystem::GetNavOcclusionRate(const FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	float OcclusionRate = 0.0f;
	UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToLocationSynchronously(ActiveSound->GetWorld(), SoundLocation, ListenerLocation);
	const bool bNotValidNavigationPath = NavigationPath == nullptr;
	if(bNotValidNavigationPath)
	{
		return OcclusionRate;
	}

	const bool bNotValidPath = !NavigationPath->GetPath().IsValid();
	if (bNotValidPath)
	{
		return OcclusionRate;
	}

	if(Debugging)
	{
		AActor* FoundActor = nullptr;
		const FString OwnerName = ActiveSound->GetOwnerName();
		for (TActorIterator<AActor> It(ActiveSound->GetWorld()); It; ++It)
		{
			if (It->GetName() == OwnerName)
			{
				FoundActor = *It;
				break;
			}
		}

		if(FoundActor)
		{
			USplineComponent* SplineComponent = FoundActor->GetComponentByClass<USplineComponent>();
			if(SplineComponent == nullptr)
			{
				SplineComponent = Cast<USplineComponent>(FoundActor->AddComponentByClass(USplineComponent::StaticClass(),false, FTransform::Identity, false));
			}
			SplineComponent->SetVisibility(true);
			SplineComponent->SetHiddenInGame(false);
			SplineComponent->ClearSplinePoints(false);
			SplineComponent->SetSplinePoints(NavigationPath->PathPoints, ESplineCoordinateSpace::World, true);
		}
	}
	
	const float PathLength = NavigationPath->GetPathLength();
	OcclusionRate = UKismetMathLibrary::MapRangeClamped(PathLength, 0.0f, 3600.0f, 0.0f, 1.0f);

	return OcclusionRate;
}

#pragma region Meta Sound Interface
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
							NSLOCTEXT("UK Sound Occlusion", "OcclusionDescription", "LineTraceTestByChannel ECC_Visibility"),
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

#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "UK.NavOcclusion"
	namespace NavOcclusionInterface
	{
		const FName Name = AUDIO_PARAMETER_INTERFACE_NAMESPACE;

		namespace Inputs
		{
			const FName NavOcclusion = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("NavOcclusion");
		} // namespace Inputs

		Audio::FParameterInterfacePtr GetInterface()
		{
			struct FInterface : public Audio::FParameterInterface
			{
				FInterface()
					: FParameterInterface(NavOcclusionInterface::Name, { 1, 0 })
				{
					Inputs =
					{
						{
							FText(),
							NSLOCTEXT("UK Sound NavOcclusion", "NavOcclusionDescription", "NavMesh Way Point Length Base Occlusion"),
							FName(),
							{ Inputs::NavOcclusion, 0.0f }
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
	} // namespace NavOcclusionInterface
#undef AUDIO_PARAMETER_INTERFACE_NAMESPACE
	
} // namespace Audio
#undef LOCTEXT_NAMESPACE
#pragma endregion Meta Sound Interface