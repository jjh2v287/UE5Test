// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAudioEngineSubsystem.h"
#include "AudioDevice.h"
#include "MetasoundSource.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "UKAudioSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavFilters/NavigationQueryFilter.h"

TMap<FTraceHandle, UUKAudioEngineSubsystem::FUKOcclusionAsyncTraceInfo> UUKAudioEngineSubsystem::TraceOcclusionMap;
TMap<FActiveSound*, UUKAudioEngineSubsystem::FUKOcclusionAsyncTraceCompleteInfo> UUKAudioEngineSubsystem::TraceCompleteHandleMap;
FTraceDelegate UUKAudioEngineSubsystem::ActiveSoundTraceDelegate;

TMap<uint32, UUKAudioEngineSubsystem::FUKNavOcclusionAsyncInfo> UUKAudioEngineSubsystem::NavOcclusionMap;
TMap<FActiveSound*, UUKAudioEngineSubsystem::FUKNavOcclusionAsyncCompleteInfo> UUKAudioEngineSubsystem::NavOcclusionCompleteMap;
FNavPathQueryDelegate UUKAudioEngineSubsystem::NavPathQueryDelegate;

bool UUKAudioEngineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void UUKAudioEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Instance = this;
	
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
	ResetTaskData();
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
	ResetTaskData();
}

void UUKAudioEngineSubsystem::Update()
{
	Super::Update();
	TRACE_CPUPROFILER_EVENT_SCOPE(UKAudioEngineSubsystem::Update);
	
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

		const bool bNotUKAudioSystem = UUKAudioEngineSubsystem::Get() == nullptr;
		if(bNotUKAudioSystem)
		{
			return;
		}

		const TArray<FActiveSound*> ActiveSounds = AudioDevice->GetActiveSounds();
		for (FActiveSound* ActiveSound : ActiveSounds)
		{
			const bool bNotValidActiveSound = ActiveSound == nullptr;
			if (bNotValidActiveSound)
			{
				continue;
			}
			
			ActiveSound->SetVolume(UUKAudioEngineSubsystem::Get()->MasterVolume);
			
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
			if (bNotValidListener)
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
			const Audio::FParameterInterfacePtr NavDopplerPitchInterface = Audio::NavDopplerPitchInterface::GetInterface();
			
			const bool bParameterOcclusion = SoundBase->ImplementsParameterInterface(OcclusionInterface);
			const bool bParameterNavOcclusion = SoundBase->ImplementsParameterInterface(NavOcclusionInterface);
			const bool bParameterDopplerPitch = SoundBase->ImplementsParameterInterface(NavDopplerPitchInterface);

			const bool bAllNotParameter = !bParameterOcclusion && !bParameterNavOcclusion && !bParameterDopplerPitch; 
			if (bAllNotParameter)
			{
				continue;
			}
			
			if (bParameterOcclusion)
			{
				if (bAsync)
				{
					UUKAudioEngineSubsystem::AsyncOcclusionTraceStart(ActiveSound, SoundLocation, ListenerLocation);

					const FUKOcclusionAsyncTraceCompleteInfo* TraceComplete = TraceCompleteHandleMap.Find(ActiveSound);
					const bool bTaskComplete = TraceComplete && TraceComplete->bTaskComplete;
					if (bTaskComplete)
					{
						FUKOcclusionAsyncTraceInfo TraceInfo;
						if (TraceOcclusionMap.RemoveAndCopyValue(TraceComplete->TraceHandle, TraceInfo))
						{
							ParamsToUpdate.Append( { { Audio::OcclusionInterface::Inputs::Occlusion, TraceComplete->bHit ? 1.0f : 0.0f }, });
							TraceCompleteHandleMap.Remove(ActiveSound);
						}
					}
				}
				else
				{
					const float OcclusionRate = UUKAudioEngineSubsystem::GetOcclusionRate(ActiveSound, SoundLocation, ListenerLocation);
					ParamsToUpdate.Append( { { Audio::OcclusionInterface::Inputs::Occlusion, OcclusionRate }, });
				}
			}

			if (bParameterNavOcclusion)
			{
				if (bAsync)
				{
					UUKAudioEngineSubsystem::AsyncNavOcclusionStart(ActiveSound, SoundLocation, ListenerLocation);

					const FUKNavOcclusionAsyncCompleteInfo* TraceComplete = NavOcclusionCompleteMap.Find(ActiveSound);
					const bool bTaskComplete = TraceComplete && TraceComplete->bTaskComplete;
					if (bTaskComplete)
					{
						FUKNavOcclusionAsyncInfo TraceInfo;
						if (NavOcclusionMap.RemoveAndCopyValue(TraceComplete->AsynId, TraceInfo))
						{
							ParamsToUpdate.Append( { { Audio::NavOcclusionInterface::Inputs::NavOcclusion, TraceComplete->OcclusionDistance }, });
							NavOcclusionCompleteMap.Remove(ActiveSound);
						}
					}
				}
				else
				{
					const float OcclusionDistance = UUKAudioEngineSubsystem::GetNavOcclusionRate(ActiveSound, SoundLocation, ListenerLocation);
					ParamsToUpdate.Append( { { Audio::NavOcclusionInterface::Inputs::NavOcclusion, OcclusionDistance }, });
				}
			}

			if(bParameterDopplerPitch)
			{
				// SoundBase->getv
				uint32 OwnerID = ActiveSound->GetOwnerID();
				FUObjectItem* OwnerObjectItem = GUObjectArray.IndexToObject(OwnerID);
				TWeakObjectPtr<AActor> OwnerActor = Cast<AActor>(OwnerObjectItem->Object);
				if(OwnerActor.IsValid())
				{
					const FVector Velocity = OwnerActor->GetVelocity();
				}

				const TArray<FListener>& Listeners =  AudioDevice->GetListeners();
				const FListener& Listener = Listeners[ClosestListenerIndex];
				const float DopplerPitch = UUKAudioEngineSubsystem::GetDopplerPitchMultiplier(Listener, ActiveSound, AudioDevice->GetGameDeltaTime());
				ParamsToUpdate.Append( { { Audio::NavDopplerPitchInterface::Inputs::DopplerPitch, DopplerPitch }, });
			}

			// Toss Meta Sound Parameter Value
			Transmitter->SetParameters(MoveTemp(ParamsToUpdate));
		}
	});

	/*FAudioThread::RunCommandOnAudioThread([CurrentDeviceId]()
	{
		FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get();
		const FAudioDeviceHandle DeviceHandle = AudioDeviceManager->GetAudioDevice(CurrentDeviceId);
		const FAudioDevice* AudioDevice = DeviceHandle.GetAudioDevice();
		const TArray<FActiveSound*> ActiveSounds = AudioDevice->GetActiveSounds();

		for (FActiveSound* ActiveSound : ActiveSounds)
		{
			const UMetaSoundSource* SoundBase = Cast<UMetaSoundSource>(ActiveSound->GetSound());
			Audio::IParameterTransmitter* Transmitter = ActiveSound->GetTransmitter();
			FAudioParameter AudioParameter = FAudioParameter(TEXT("Angle"), 7777.0f);
			FAudioParameter AudioParameter = FAudioParameter(TEXT("TriggerName"), EAudioParameterType::Trigger);
			TArray<FAudioParameter> ParamsToSet;
			ParamsToSet.Emplace(AudioParameter);
			Transmitter->SetParameters(MoveTemp(ParamsToSet));
		}
	});*/

	/*float CurrentPitchScale = 0.0f;
	FAudioParameter OutParam;
	TArray<Audio::FParameterInterface::FInput> ParanInputs = NavDopplerPitchInterface->GetInputs();

	const bool bNotEmptyParanInputs = !ParanInputs.IsEmpty(); 
	if(bNotEmptyParanInputs)
	{
		const FName ParamName = ParanInputs[0].InitValue.ParamName;
		const bool IsParameter = Transmitter->GetParameter(ParamName, OutParam);
		if(IsParameter)
		{
			CurrentPitchScale = OutParam.FloatParam;
		}
	}*/
}

void UUKAudioEngineSubsystem::ResetTaskData()
{
	TraceOcclusionMap.Reset();
	TraceCompleteHandleMap.Reset();

	NavOcclusionMap.Reset();
	NavOcclusionCompleteMap.Reset();
}

void UUKAudioEngineSubsystem::AsyncOcclusionTraceStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	bool bNotComplete = TraceCompleteHandleMap.Contains(ActiveSound);
	if (bNotComplete)
	{
		return;
	}

	UWorld* World = ActiveSound->GetWorld();
	const bool bNotWorld = World == nullptr;
	if(bNotWorld)
	{
		return;
	}
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AsyncSoundOcclusion), true);
	Params.AddIgnoredActor( ActiveSound->GetOwnerID() );
	const int32 PlayerNum = UGameplayStatics::GetNumPlayerControllers(World);
	for(int32 i = 0; i < PlayerNum; i++)
	{
		Params.AddIgnoredActor( UGameplayStatics::GetPlayerController(World, i) );
		Params.AddIgnoredActor( UGameplayStatics::GetPlayerPawn(World, i) );
	}
	
	FUKOcclusionAsyncTraceInfo TraceInfo;
	TraceInfo.AudioDeviceID = ActiveSound->AudioDevice->DeviceID;
	TraceInfo.ActiveSound = ActiveSound;
	FTraceHandle TraceHandle = ActiveSound->GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, SoundLocation, ListenerLocation, ECollisionChannel::ECC_Visibility, Params, FCollisionResponseParams::DefaultResponseParam, &ActiveSoundTraceDelegate);
	TraceOcclusionMap.Emplace(TraceHandle, TraceInfo);

	FUKOcclusionAsyncTraceCompleteInfo TraceComplete;
	TraceComplete.TraceHandle = TraceHandle;
	TraceComplete.bHit = false;
	TraceComplete.bTaskComplete = false;
	TraceCompleteHandleMap.Emplace(ActiveSound, TraceComplete);
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
		FUKOcclusionAsyncTraceInfo* TraceInfo = TraceOcclusionMap.Find(TraceHandle);
		const bool bNotValidTraceInfo = TraceInfo == nullptr;
		if (bNotValidTraceInfo)
		{
			return;
		}

		FUKOcclusionAsyncTraceCompleteInfo* TraceComplete = TraceCompleteHandleMap.Find(TraceInfo->ActiveSound);
		const bool bNotValidTraceCompleteInfo = TraceComplete == nullptr;
		if (bNotValidTraceCompleteInfo)
		{
			return;
		}
		
		FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get();
		const bool bNotValidAudioDeviceManager = AudioDeviceManager == nullptr;
		if (bNotValidAudioDeviceManager)
		{
			return;
		}
		
		const FAudioDeviceHandle DeviceHandle = AudioDeviceManager->GetAudioDevice(TraceInfo->AudioDeviceID);
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
		const bool bNotContainsActiveSound = !ActiveSounds.Contains(TraceInfo->ActiveSound);
		if (bNotContainsActiveSound)
		{
			TraceOcclusionMap.Remove(TraceHandle);
			TraceCompleteHandleMap.Remove(TraceInfo->ActiveSound);
			return;
		}
		
		TraceComplete->bTaskComplete = true;
		TraceComplete->bHit = bFoundBlockingHit;
	});
}

void UUKAudioEngineSubsystem::AsyncNavOcclusionStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	bool bNotComplete = NavOcclusionCompleteMap.Contains(ActiveSound);
	if (bNotComplete)
	{
		return;
	}

	UWorld* World = ActiveSound->GetWorld();
	const bool bNotWorld = World == nullptr;
	if (bNotWorld)
	{
		return;
	}
	
	UNavigationSystemV1* NavigationSystem = Cast<UNavigationSystemV1>(World->GetNavigationSystem());
	const bool bNotNavigationSystem = NavigationSystem == nullptr;
	if (bNotNavigationSystem)
	{
		return;
	}
	
	ANavigationData* NavigationData = NavigationSystem->GetDefaultNavDataInstance();
	const bool bNotValidNavigationData = NavigationData == nullptr;
	if (bNotValidNavigationData)
	{
		return;
	}

	const FVector SourceDirection = SoundLocation - ListenerLocation;
	const float Distance = SourceDirection.Size();
	
	const FNavAgentProperties AgentProperties;
	const FVector Extent = FVector(100.0f, 100.0f, 500.0f);
	FNavLocation ProjectedLocation;
	NavigationSystem->ProjectPointToNavigation(ListenerLocation, ProjectedLocation, Extent, &AgentProperties);

	FPathFindingQuery FindingQuery(nullptr, *NavigationData, SoundLocation, ProjectedLocation.Location, UNavigationQueryFilter::GetQueryFilter(*NavigationData, nullptr, nullptr));
	FindingQuery.SetAllowPartialPaths(true);
	uint32 QueryID = NavigationSystem->FindPathAsync(AgentProperties, FindingQuery, NavPathQueryDelegate, EPathFindingMode::Regular);

	FUKNavOcclusionAsyncInfo Info;
	Info.AsynId = QueryID;
	Info.ActiveSound = ActiveSound;
	Info.AudioDeviceID = ActiveSound->AudioDevice->DeviceID;
	Info.Distance = Distance;
	NavOcclusionMap.Emplace(QueryID, Info);

	FUKNavOcclusionAsyncCompleteInfo CompleteInfo;
	CompleteInfo.AsynId = QueryID;
	NavOcclusionCompleteMap.Emplace(ActiveSound, CompleteInfo);
}

void UUKAudioEngineSubsystem::AsyncNavOcclusionEnd(uint32 QueryID, ENavigationQueryResult::Type Result, FNavPathSharedPtr NavPath)
{
	FAudioThread::RunCommandOnAudioThread( [QueryID, NavPath, Result]()
	{
		FUKNavOcclusionAsyncInfo* Info = NavOcclusionMap.Find(QueryID);
		const bool bNotValidInfo = Info == nullptr;
		if (bNotValidInfo)
		{
			return;
		}
		
		FUKNavOcclusionAsyncCompleteInfo* CompleteInfo= NavOcclusionCompleteMap.Find(Info->ActiveSound);
		const bool bNotValidCompleteInfo = CompleteInfo == nullptr;
		if (bNotValidCompleteInfo)
		{
			return;
		}

		FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get();
		const bool bNotValidAudioDeviceManager = AudioDeviceManager == nullptr;
		if (bNotValidAudioDeviceManager)
		{
			return;
		}
		
		const FAudioDeviceHandle DeviceHandle = AudioDeviceManager->GetAudioDevice(Info->AudioDeviceID);
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
		const bool bNotContainsActiveSound = !ActiveSounds.Contains(Info->ActiveSound);
		if (bNotContainsActiveSound)
		{
			NavOcclusionMap.Remove(QueryID);
			NavOcclusionCompleteMap.Remove(Info->ActiveSound);
			return;
		}

		const float Distance = Info->Distance;
		const float PathLength = NavPath->GetLength();
		const bool bIsSuccess = Result == ENavigationQueryResult::Success;
		const float OcclusionDistance = bIsSuccess ? PathLength : Distance;
		
		CompleteInfo->bTaskComplete = true;
		CompleteInfo->OcclusionDistance = OcclusionDistance;
	});
}

const float UUKAudioEngineSubsystem::GetOcclusionRate(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	UWorld* World = ActiveSound->GetWorld();
	const bool bNotWorld = World == nullptr;
	if(bNotWorld)
	{
		return 0.0f;
	}
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SoundOcclusion), true);
	Params.AddIgnoredActor( ActiveSound->GetOwnerID() );
	const int32 PlayerNum = UGameplayStatics::GetNumPlayerControllers(World);
	for(int32 i = 0; i < PlayerNum; i++)
	{
		Params.AddIgnoredActor( UGameplayStatics::GetPlayerController(World, i) );
		Params.AddIgnoredActor( UGameplayStatics::GetPlayerPawn(World, i) );
	}
	
	const bool bIsOccluded = ActiveSound->GetWorld()->LineTraceTestByChannel(SoundLocation, ListenerLocation, ECollisionChannel::ECC_Visibility, Params);
	const float OcclusionRate = bIsOccluded ? 1.0f : 0.0f;

	return OcclusionRate;
}

const float UUKAudioEngineSubsystem::GetNavOcclusionRate(const FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation)
{
	const FVector SourceDirection = SoundLocation - ListenerLocation;
	const float Distance = SourceDirection.Size();
	UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToLocationSynchronously(ActiveSound->GetWorld(), SoundLocation, ListenerLocation);

	const bool bNotValidNavigationPath = NavigationPath == nullptr;
	if (bNotValidNavigationPath)
	{
		return Distance;
	}

	const bool bNotValidPath = !NavigationPath->GetPath().IsValid();
	if (bNotValidPath)
	{
		return Distance;
	}

	const float OcclusionDistance = NavigationPath->GetPathLength();

	return OcclusionDistance;
}

const float UUKAudioEngineSubsystem::GetDopplerPitchMultiplier(FListener const& InListener, const FActiveSound* ActiveSound, const float DeltaTime)
{
	const FVector SoundLocation = ActiveSound->Transform.GetLocation();
	const FVector LastLocation = ActiveSound->LastLocation;
	const FVector SoundVelocity = (SoundLocation - LastLocation) / DeltaTime;
	
	// Mach(마하) cm/sec
	static const float SpeedOfSoundInAirAtSeaLevel = 33000.0f;

	const FVector SourceToListenerNorm = (InListener.Transform.GetLocation() - SoundLocation).GetSafeNormal();

	// find source and listener speeds along the line between them
	float const SourceVelMagTorwardListener = FVector::DotProduct(SoundVelocity, SourceToListenerNorm);
	float const ListenerVelMagAwayFromSource = FVector::DotProduct(InListener.Velocity, SourceToListenerNorm);

	// multiplier = 1 / (1 - ((sourcevel - listenervel) / speedofsound) );
	float const InvDopplerPitchScale = 1.0f - ( (SourceVelMagTorwardListener - ListenerVelMagAwayFromSource) / SpeedOfSoundInAirAtSeaLevel );
	float const PitchScale = 1.0f / InvDopplerPitchScale;

	// factor in user-specified intensity
	const float DopplerIntensity = 1.0f;
	float const FinalPitchScale = ((PitchScale - 1.0f) * DopplerIntensity) /*+ 1.0f*/;
	
	return FinalPitchScale;
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

#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "UK.DopplerPitch"
	namespace NavDopplerPitchInterface
	{
		const FName Name = AUDIO_PARAMETER_INTERFACE_NAMESPACE;

		namespace Inputs
		{
			const FName DopplerPitch = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("DopplerPitch");
		} // namespace Inputs

		Audio::FParameterInterfacePtr GetInterface()
		{
			struct FInterface : public Audio::FParameterInterface
			{
				FInterface()
					: FParameterInterface(NavDopplerPitchInterface::Name, { 1, 0 })
				{
					Inputs =
					{
						{
							FText(),
							NSLOCTEXT("DopplerPitch", "DopplerPitchDescription", "DopplerPitch"),
							FName(),
							{ Inputs::DopplerPitch, 0.0f }
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
	} // namespace NavDopplerPitchInterface
#undef AUDIO_PARAMETER_INTERFACE_NAMESPACE
	
} // namespace Audio
#undef LOCTEXT_NAMESPACE
#pragma endregion Meta Sound Interface