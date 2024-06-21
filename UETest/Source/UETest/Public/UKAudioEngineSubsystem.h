// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/AudioEngineSubsystem.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "UKAudioEngineSubsystem.generated.h"

// #define LOCTEXT_NAMESPACE "MetasoundStandardNodes_FootMateSound"
// namespace Metasound
// {
// 	enum class EFootSound : int32
// 	{
// 		Hard = 0,
// 		Soft,
// 	};
//
// 	DECLARE_METASOUND_ENUM(
// 		EFootSound,
// 		EFootSound::Hard,
// 		UETEST_API,
// 		FEnumFootSound,
// 		FEnumFootSoundInfo,
// 		FEnumFootSoundReadRef,
// 		FEnumFootSoundWriteRef
// 		);
//
// 	DEFINE_METASOUND_ENUM_BEGIN(EFootSound, FEnumFootSound, "FootSound")
// 		DEFINE_METASOUND_ENUM_ENTRY(EFootSound::Hard, "KneeModeHardDescription", "Hard", "KneeModeHardDescriptionTT", "Only audio strictly above the threshold is affected by the limiter."),
// 		DEFINE_METASOUND_ENUM_ENTRY(EFootSound::Soft, "KneeModeSoftDescription", "Soft", "KneeModeSoftDescriptionTT", "Limiter activates more smoothly near the threshold."),
// 	DEFINE_METASOUND_ENUM_END()
// }
// #undef LOCTEXT_NAMESPACE

/**
 * 
 */
UCLASS()
class UETEST_API UUKAudioEngineSubsystem : public UAudioEngineSubsystem
{
	GENERATED_BODY()
public: 
	virtual ~UUKAudioEngineSubsystem() override = default;

	//~ Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	//~ Begin UAudioEngineSubsystem interface
	virtual void Update() override;
	//~ End UAudioEngineSubsystem interface

private:
	void ResetTaskData();

	static void AsyncOcclusionTraceStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);
	static void AsyncOcclusionTraceEnd(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);

	static void AsyncNavOcclusionStart(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);
	static void AsyncNavOcclusionEnd(uint32 QueryID, ENavigationQueryResult::Type Result, FNavPathSharedPtr NavPath);
	
	static const float GetOcclusionRate(FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);
	static const float GetNavOcclusionRate(const FActiveSound* ActiveSound, const FVector SoundLocation, const FVector ListenerLocation);

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
} // namespace Audio
#pragma endregion Meta Sound Interface