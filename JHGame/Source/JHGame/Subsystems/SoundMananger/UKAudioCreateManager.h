// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveSoundUpdateInterface.h"
#include "AudioDevice.h"
#include "IAudioParameterInterfaceRegistry.h"
#include "Subsystems/AudioEngineSubsystem.h"
#include "UKAudioCreateManager.generated.h"

/**
 * 
 */
UCLASS()
class JHGAME_API UUKAudioCreateManager : public UAudioEngineSubsystem, public IActiveSoundUpdateInterface
{
	GENERATED_BODY()
private:
	static inline UUKAudioCreateManager* Instance = nullptr;
	
public: 
	virtual ~UUKAudioCreateManager() override = default;
	static UUKAudioCreateManager* Get() { return Instance; }

	//~ Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface
	
	// ~ IActiveSoundUpdateInterface
	virtual void OnNotifyAddActiveSound(FActiveSound& ActiveSound) override;
	virtual void OnNotifyPendingDelete(const FActiveSound& ActiveSound) override;
	// ~ IActiveSoundUpdateInterface

	/**
	 * Plays a sound directly with no attenuation, perfect for UI sounds.
	 *
	 * * Fire and Forget.
	 * * Not Replicated.
	 * @param Sound - Sound to play.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	  * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param OwningActor - The actor to use as the "owner" for concurrency settings purposes. 
	 *						Allows PlaySound calls to do a concurrency limit per owner.
	 * @param bIsUISound - True if sound is UI related, else false
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="UK Audio", meta=( WorldContext="WorldContextObject", AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true" ))
	static void PlaySound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundConcurrency* ConcurrencySettings = nullptr, const AActor* OwningActor = nullptr, bool bIsUISound = true);

	/**
	 * Plays a sound at the given location. This is a fire and forget sound and does not travel with any actor. 
	 * Replication is also not handled at this point.
	 * @param Sound - sound to play
	 * @param Location - World position to play sound at
	 * @param Rotation - World rotation to play sound at
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param OwningActor - The actor to use as the "owner" for concurrency settings purposes. Allows PlaySound calls
	 *						to do a concurrency limit per owner.
	 */
	UFUNCTION(BlueprintCallable, Category="UK Audio", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	static void PlaySoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, const AActor* OwningActor = nullptr, const UInitialActiveSoundParams* InitialParams = nullptr);

	/**
	 * This function allows users to create Audio Components with settings specifically for non-spatialized,
	 * non-distance-attenuated sounds. Audio Components created using this function by default will not have 
	 * Spatialization applied. Sound instances will begin playing upon spawning this Audio Component.
	 *
	 * * Not Replicated.
	 * @param Sound - Sound to play.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param PersistAcrossLevelTransition - Whether the sound should continue to play when the map it was played in is unloaded
	 * @param bAutoDestroy - Whether the returned audio component will be automatically cleaned up when the sound finishes 
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return An audio component to manipulate the spawned sound
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="UK Audio", meta=( WorldContext="WorldContextObject", AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true", Keywords = "play" ))
	static UAudioComponent* SpawnSound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundConcurrency* ConcurrencySettings = nullptr, bool bPersistAcrossLevelTransition = false, bool bAutoDestroy = true);

	/**
	 * Spawns a sound at the given location. This does not travel with any actor. Replication is also not handled at this point.
	 * @param Sound - sound to play
	 * @param Location - World position to play sound at
	 * @param Rotation - World rotation to play sound at
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param bAutoDestroy - Whether the returned audio component will be automatically cleaned up when the sound finishes 
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return An audio component to manipulate the spawned sound
	 */
	UFUNCTION(BlueprintCallable, Category="UK Audio", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	static UAudioComponent* SpawnSoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation = FRotator::ZeroRotator, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true);

	/** This function allows users to create and play Audio Components attached to a specific Scene Component. 
	 *  Useful for spatialized and/or distance-attenuated sounds that need to follow another object in space.
	 * @param Sound - sound to play
	 * @param AttachComponent - Component to attach to.
	 * @param AttachPointName - Optional named point within the AttachComponent to play the sound at
	 * @param Location - Depending on the value of Location Type this is either a relative offset from 
	 *					 the attach component/point or an absolute world position that will be translated to a relative offset
	 * @param Rotation - Depending on the value of Location Type this is either a relative offset from
	 *					 the attach component/point or an absolute world rotation that will be translated to a relative offset
	 * @param LocationType - Specifies whether Location is a relative offset or an absolute world position
	 * @param bStopWhenAttachedToDestroyed - Specifies whether the sound should stop playing when the
	 *										 owner of the attach to component is destroyed.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param bAutoDestroy - Whether the returned audio component will be automatically cleaned up when the sound finishes
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return An audio component to manipulate the spawned sound
	 */
	UFUNCTION(BlueprintCallable, Category="UK Audio", meta=(AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	static UAudioComponent* SpawnSoundAttached(USoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true);

private:
	static FString GetDebugInfo(const UObject* WorldContextObject, const USoundBase* Sound);
	
	void OnCheatManagerCreated(UCheatManager* CheatManager);
	static void OnCommand_AudioLog(const TArray<FString>& InParams);
	bool bLog = false;
};