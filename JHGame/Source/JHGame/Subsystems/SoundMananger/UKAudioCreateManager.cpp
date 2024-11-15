// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAudioCreateManager.h"
#include "Engine/Engine.h"
#include "AudioDevice.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CheatManager.h"
#include "GameFramework/WorldSettings.h"
#include "JHGame/Common/UKCommonLog.h"
#include "JHGame/GameFramework/UKCheatManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKAudioCreateManager)

bool UUKAudioCreateManager::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && !IsRunningDedicatedServer();
}

void UUKAudioCreateManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
	UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateUObject(this, &ThisClass::OnCheatManagerCreated));
}

void UUKAudioCreateManager::Deinitialize()
{
	Super::Deinitialize();
	Instance = nullptr;
}

void UUKAudioCreateManager::OnNotifyAddActiveSound(FActiveSound& ActiveSound)
{
	if (bLog)
	{
		UAudioComponent* AudioComponent = UAudioComponent::GetAudioComponentFromID(ActiveSound.GetAudioComponentID());
		const UObject* WorldContextObject = AudioComponent ? static_cast<UObject*>(AudioComponent) : static_cast<UObject*>(ActiveSound.GetWorld());
		UK_LOG(Log, "Owner: %s | AudioComponent: %s | DebugInfo: %s", *ActiveSound.GetOwnerName(), *ActiveSound.GetAudioComponentName(), *GetDebugInfo(WorldContextObject, ActiveSound.GetSound()));
	}
}

void UUKAudioCreateManager::OnNotifyPendingDelete(const FActiveSound& ActiveSound)
{
	if (bLog)
	{
		UAudioComponent* AudioComponent = UAudioComponent::GetAudioComponentFromID(ActiveSound.GetAudioComponentID());
		const UObject* WorldContextObject = AudioComponent ? static_cast<UObject*>(AudioComponent) : static_cast<UObject*>(ActiveSound.GetWorld());
		UK_LOG(Log, "Owner: %s | AudioComponent: %s | DebugInfo: %s", *ActiveSound.GetOwnerName(), *ActiveSound.GetAudioComponentName(), *GetDebugInfo(WorldContextObject, ActiveSound.GetSound()));
	}
}

void UUKAudioCreateManager::PlaySound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier, float StartTime, USoundConcurrency* ConcurrencySettings, const AActor* OwningActor, bool bIsUISound)
{
	if (UUKAudioCreateManager::Get()->bLog)
	{
		UK_LOG(Log, "%s", *GetDebugInfo(WorldContextObject, Sound));
	}
	UGameplayStatics::PlaySound2D(WorldContextObject, Sound, VolumeMultiplier, PitchMultiplier, StartTime, ConcurrencySettings, OwningActor, bIsUISound);
}

void UUKAudioCreateManager::PlaySoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation, float VolumeMultiplier, float PitchMultiplier, float StartTime, class USoundAttenuation* AttenuationSettings, USoundConcurrency* ConcurrencySettings, const AActor* OwningActor, const UInitialActiveSoundParams* InitialParams)
{
	if (UUKAudioCreateManager::Get()->bLog)
	{
		UK_LOG(Log, "%s", *GetDebugInfo(WorldContextObject, Sound));
	}
	UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, Rotation, VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings, ConcurrencySettings, OwningActor, InitialParams);
}

UAudioComponent* UUKAudioCreateManager::SpawnSound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier, float StartTime, USoundConcurrency* ConcurrencySettings, bool bPersistAcrossLevelTransition, bool bAutoDestroy)
{
	if (UUKAudioCreateManager::Get()->bLog)
	{
		UK_LOG(Log, "%s", *GetDebugInfo(WorldContextObject, Sound));
	}
	return UGameplayStatics::SpawnSound2D(WorldContextObject, Sound, VolumeMultiplier, PitchMultiplier, StartTime, ConcurrencySettings, bPersistAcrossLevelTransition, bAutoDestroy);
}

UAudioComponent* UUKAudioCreateManager::SpawnSoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation, float VolumeMultiplier, float PitchMultiplier, float StartTime, class USoundAttenuation* AttenuationSettings, USoundConcurrency* ConcurrencySettings, bool bAutoDestroy)
{
	if (UUKAudioCreateManager::Get()->bLog)
	{
		UK_LOG(Log, "%s", *GetDebugInfo(WorldContextObject, Sound));
	}
	return UGameplayStatics::SpawnSoundAtLocation(WorldContextObject, Sound, Location, Rotation, VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings, ConcurrencySettings, bAutoDestroy);
}

UAudioComponent* UUKAudioCreateManager::SpawnSoundAttached(USoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, FRotator Rotation, EAttachLocation::Type LocationType, bool bStopWhenAttachedToDestroyed, float VolumeMultiplier, float PitchMultiplier, float StartTime, USoundAttenuation* AttenuationSettings, USoundConcurrency* ConcurrencySettings, bool bAutoDestroy)
{
	if (UUKAudioCreateManager::Get()->bLog)
	{
		UK_LOG(Log, "%s", *GetDebugInfo(AttachToComponent, Sound));
	}
	return UGameplayStatics::SpawnSoundAttached(Sound, AttachToComponent, AttachPointName, Location, Rotation, LocationType, bStopWhenAttachedToDestroyed, VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings, ConcurrencySettings, bAutoDestroy);
}

FString UUKAudioCreateManager::GetDebugInfo(const UObject* WorldContextObject, const USoundBase* Sound)
{
	float DeviceVolume = 0.0f;
	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (IsValid(ThisWorld) && (ThisWorld->bAllowAudioPlayback || !ThisWorld->IsNetMode(NM_DedicatedServer)))
	{
		FAudioDevice* AudioDevice = (ThisWorld ? ThisWorld->GetAudioDeviceRaw() : nullptr);
        if (!AudioDevice)
        {
        	AudioDevice = (GEngine ? GEngine->GetMainAudioDeviceRaw() : nullptr);
        }
    
        DeviceVolume = AudioDevice ? AudioDevice->GetTransientPrimaryVolume() : -1.0f;
	}

	return FString::Printf(TEXT("| WorldContextObject: %s | Sound: %s | DeviceVolume: %f"),
		*GetNameSafe(WorldContextObject),
		*GetNameSafe(Sound),
		DeviceVolume
		);
}

void UUKAudioCreateManager::OnCheatManagerCreated(UCheatManager* CheatManager)
{
	if (UUKCheatManager* UKCheatManager = Cast<UUKCheatManager>(CheatManager))
	{
		UKCheatManager->RegisterCommand("audio", FOnUKCommand::FDelegate::CreateStatic(OnCommand_AudioLog));
	}	
}

void UUKAudioCreateManager::OnCommand_AudioLog(const TArray<FString>& InParams)
{
	if (InParams.IsEmpty() || InParams.Num() < 2)
	{
		return;
	}

	FString Param = InParams[0].ToLower();
	if (Param.Compare(TEXT("log")) != 0)
	{
		return;
	}
	
	Param = InParams[1].ToLower();
	if (Param.Compare(TEXT("1")) == 0)
	{
		UUKAudioCreateManager::Get()->bLog = true;
	}
	else
	{
		UUKAudioCreateManager::Get()->bLog = false;
	}
}
