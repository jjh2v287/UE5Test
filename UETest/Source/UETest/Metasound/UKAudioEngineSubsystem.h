// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/VolumeFader.h"
#include "Subsystems/AudioEngineSubsystem.h"
#include "UKAudioEngineSubsystem.generated.h"

struct FListener;
/**
 * 
 */
UCLASS()
class UETEST_API UUKAudioEngineSubsystem : public UAudioEngineSubsystem
{
	GENERATED_BODY()
private:
	static inline UUKAudioEngineSubsystem* Instance = nullptr;
	
public: 
	virtual ~UUKAudioEngineSubsystem() override = default;
	static UUKAudioEngineSubsystem* Get() { return Instance; }

	//~ Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	//~ Begin UAudioEngineSubsystem interface
	virtual void Update() override;
	//~ End UAudioEngineSubsystem interface

	UFUNCTION(BlueprintCallable)
	void StartInGameVolumeFader(float InVolume, float InDuration);

	float TODTime = 0.0f;

private:
	void ResetTaskData();

	static void AsyncOcclusionTraceStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);
	static void AsyncOcclusionTraceEnd(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);

	static void AsyncNavOcclusionStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);
	static void AsyncNavOcclusionEnd(uint32 QueryID, ENavigationQueryResult::Type Result, FNavPathSharedPtr NavPath);
	
	static const float GetOcclusionRate(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);
	static const float GetNavOcclusionRate(const FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);

	static const float GetDopplerPitchMultiplier(FListener const& InListener, const FActiveSound* ActiveSound, const float DeltaTime);
	
private:
	static const bool bAsync = true;
	
	struct FUKOcclusionAsyncTraceInfo
	{
		Audio::FDeviceId AudioDeviceID = 0;
		FActiveSound* ActiveSound = nullptr;
	};

	struct FUKOcclusionAsyncTraceCompleteInfo
    {
    	FTraceHandle TraceHandle;
		bool bTaskComplete = false;
    	bool bHit = false;
    };
	
	static TMap<FTraceHandle, FUKOcclusionAsyncTraceInfo> TraceOcclusionMap;
	static TMap<FActiveSound*, FUKOcclusionAsyncTraceCompleteInfo> TraceCompleteHandleMap;
	static FTraceDelegate ActiveSoundTraceDelegate;

	struct FUKNavOcclusionAsyncInfo
	{
		uint32 AsynId = 0;
		Audio::FDeviceId AudioDeviceID = 0;
		FActiveSound* ActiveSound = nullptr;
		float Distance = 0.0f;
	};

	struct FUKNavOcclusionAsyncCompleteInfo
	{
		uint32 AsynId = 0;
		bool bTaskComplete = false;
		float OcclusionDistance = 0.0f;
	};
	
	static TMap<uint32, FUKNavOcclusionAsyncInfo> NavOcclusionMap;
	static TMap<FActiveSound*, FUKNavOcclusionAsyncCompleteInfo> NavOcclusionCompleteMap;
	static FNavPathQueryDelegate NavPathQueryDelegate;

	Audio::FVolumeFader InGameVolumeFader;

	// 트리 계충 구조로 사운드 볼륨을 조절
	//void UGameplayStatics::PlaySound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier, float StartTime, USoundConcurrency* ConcurrencySettings, const AActor* OwningActor, bool bIsUISound)
};

#pragma region Meta Sound Interface
namespace Audio
{
	namespace OcclusionInterface
	{
		const extern FName Name;

		namespace Inputs
		{
			const extern FName Occlusion;
		} // namespace Inputs

		Audio::FParameterInterfacePtr GetInterface();
	} // namespace OcclusionInterface

	namespace NavOcclusionInterface
	{
		const extern FName Name;

		namespace Inputs
		{
			const extern FName NavOcclusion;
		} // namespace Inputs

		Audio::FParameterInterfacePtr GetInterface();
	} // namespace NavOcclusionInterface

	namespace NavDopplerPitchInterface
	{
		const extern FName Name;

		namespace Inputs
		{
			const extern FName DopplerPitch;
		} // namespace Inputs

		Audio::FParameterInterfacePtr GetInterface();
	} // namespace NavDopplerPitchInterface
	
} // namespace Audio
#pragma endregion Meta Sound Interface