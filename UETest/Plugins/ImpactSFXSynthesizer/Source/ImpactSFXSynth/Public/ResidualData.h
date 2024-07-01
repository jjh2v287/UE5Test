// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ImpactSynthCurve.h"
#include "ResidualObj.h"
#include "UObject/Object.h"
#include "IAudioProxyInitializer.h"
#include "Curves/CurveFloat.h"

#include "ResidualData.generated.h"

#if WITH_EDITOR
DECLARE_DELEGATE(FOnResidualDataSynthesized)
DECLARE_DELEGATE_TwoParams(FOnResidualDataError, const FString&, const FString&)
#endif

class UResidualObj;
class FResidualDataAssetProxy;
using FResidualDataAssetProxyPtr = TSharedPtr<FResidualDataAssetProxy, ESPMode::ThreadSafe>;

UENUM(BlueprintType)
enum class EResidualPhaseGenerator : uint8
{
	Random UMETA(DisplayName = "None"),
	Constant UMETA(DisplayName = "Constant"),
	IncrementByFreq UMETA(DisplayName = "Saber")
};

USTRUCT(BlueprintType)
struct FResidualPhaseEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Effect Type"))
	EResidualPhaseGenerator EffectType = EResidualPhaseGenerator::Random;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Effect Scale", ClampMin="0.01", ClampMax="10."))
	float EffectScale = 1.f;
};

USTRUCT(BlueprintType)
struct FResidualMagnitudeEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Shift Energy by ERB (Circularly)", ToolTip="Shift energy distribution based on ERB distance. Max Value = Number of Erb."))
	int32 ShiftByErb = 0;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Shift Energy by Frequency (Circularly)", ToolTip="Shift energy distribution based on frequency distance. Max Value = (Number of FFT / 2) + 1."))
	int32 ShiftByFreq = 0;
	
	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Rolling Rate", ToolTip="Circularly shift magnitude over time to create a rolling effect."))
	float CircularShift = 0;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Chirp Rate", ToolTip="Gradually increasing/decreasing frequence bands linearly when playing."))
	float ChirpRate = 0.f;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Shift ERB Min", ClampMax="41.", ToolTip="Change minimum ERB value."))
	float ErbMinShift = 0.f;

	UPROPERTY(EditAnywhere, Category="Residual", meta = (DisplayName = "Shift ERB Max", ClampMin="-41", ToolTip="Change maximum ERB value."))
	float ErbMaxShift = 0.f;
};

UCLASS(editinlinenew, BlueprintType,  meta = (LoadBehavior = "LazyOnDemand"))
class IMPACTSFXSYNTH_API UResidualData : public UObject, public IAudioProxyDataFactory
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category="Residual", meta = (DisplayName = "Phase Effect"))
	FResidualPhaseEffect PhaseEffect;

	UPROPERTY(EditDefaultsOnly, Category="Residual", meta = (DisplayName = "Magnitude Effect"))
	FResidualMagnitudeEffect MagEffect;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Preview Pitch Shift", ToolTip="Shift pitches by semitone", ClampMin="-72", ClampMax="72"), Category="Preview")
	float PreviewPitchShift = 0;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Preview Play Speed", ClampMin="0.1", ClampMax="10."), Category="Preview")
	float PreviewPlaySpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Preview Seed", ToolTip="< 0 = Random", ClampMin="-1"), Category="Preview")
	int32 PreviewSeed = -1;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Preview Start Time", ToolTip="Star playing time"), Category="Preview")
	float PreviewStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Preview Duration", ToolTip="Plaing duration", ClampMin="-1"), Category="Preview")
	float PreviewDuration = -1.f;
	
public:
	static constexpr int32 MaxNumCurve = 2;
	
	UPROPERTY(EditDefaultsOnly, Category="Residual Obj", meta = (DisplayName = "Residual Obj"))
	TObjectPtr<UResidualObj> ResidualObj;
	
	UPROPERTY(EditDefaultsOnly, Category="Residual", meta = (DisplayName = "Amplitude over Time Mod Curve"))
	FImpactSynthCurve AmplitudeOverTimeCurve = FImpactSynthCurve(EImpactSynthCurveSource::None, EImpactSynthCurveDisplayType::ScaleOverTime,
																TEXT("Amplitude over Time Mod Curve"));

	UPROPERTY(EditDefaultsOnly, Category="Residual", meta = (DisplayName = "Amplitude over Frequency Mod Curve"))
	FImpactSynthCurve AmplitudeOverFreqCurve = FImpactSynthCurve(EImpactSynthCurveSource::None, EImpactSynthCurveDisplayType::ScaleOverFrequency,
																TEXT("Amplitude over Frequency Mod Curve"));
	
	FORCEINLINE FResidualPhaseEffect GetPhaseEffect() const { return PhaseEffect; }
	FORCEINLINE FResidualMagnitudeEffect GetMagEffect() const { return MagEffect; }
	
	static float Freq2Erb(const float Freq);
	static float Erb2Freq(const float Erb);
	void CheckAllCurveShared(const FProperty* Property, FName PropertyName);
	void CheckCurveShared(FImpactSynthCurve& InCurve, const FFloatRange InTimeRange, const FFloatRange InValueRange);

#if WITH_EDITORONLY_DATA
	
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Preview Volume", ClampMin="0", ClampMax="10."), Category="Preview")
	float PreviewVolumeScale = 1.0f;
	
	UPROPERTY(VisibleAnywhere, Category="Residual Obj", meta = (DisplayName = "Number of FFT Points"))
	int32 NumFFT;
	
	UPROPERTY(VisibleAnywhere, Category="Residual Obj", meta = (DisplayName = "Number of ERB"))
	int32 NumErb;
	
	UPROPERTY(VisibleAnywhere, Category="Residual Obj", meta = (DisplayName = "Number of Frames"))
	int32 NumFrame;
	
	UPROPERTY(VisibleAnywhere, Category="Residual Obj", meta = (DisplayName = "Sampling Rate"))
	float SamplingRate;
	
	FOnResidualDataSynthesized OnResidualDataSynthesized;
	FOnResidualDataSynthesized OnStopPlayingResidualData;
	FOnResidualDataError OnResidualDataError;
	
#endif
	
	//~Begin IAudioProxyDataFactory Interface.
	virtual TSharedPtr<Audio::IProxyData, ESPMode::ThreadSafe> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	//~ End IAudioProxyDataFactory Interface.
	FResidualDataAssetProxyPtr CreateNewResidualProxyData();
	FResidualDataAssetProxyPtr GetProxy();
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& InPropertyChangedEvent) override;
	
	UFUNCTION(CallInEditor, Category="Preview")
	void PlaySynthesizedSound() const;

	UFUNCTION(CallInEditor, Category="Preview")
	void StopPlaying() const;
	
	int32 GetNumCurves() const;

	void SetResidualObj(UResidualObj* InObj);
#endif

private:
	// cached proxy
	FResidualDataAssetProxyPtr Proxy{ nullptr };
	
#if WITH_EDITOR
	void UpdateResidualObjProperties();
#endif
};

class IMPACTSFXSYNTH_API FResidualDataAssetProxy : public Audio::TProxyData<FResidualDataAssetProxy>, public TSharedFromThis<FResidualDataAssetProxy, ESPMode::ThreadSafe>
{
public:
	IMPL_AUDIOPROXY_CLASS(FResidualDataAssetProxy);

	FResidualDataAssetProxy(const FResidualDataAssetProxy& InAssetProxy)
		: ResidualObj(InAssetProxy.ResidualObj)
		, PhaseEffect(InAssetProxy.PhaseEffect)
		, MagEffect(InAssetProxy.MagEffect)
		, ScaleAmplitudeCurve(InAssetProxy.ScaleAmplitudeCurve)
		, ScaleFreqCurve(InAssetProxy.ScaleFreqCurve)
		, PreviewPitchShift(InAssetProxy.PreviewPitchShift)
		, PreviewPlaySpeed(InAssetProxy.PreviewPlaySpeed)
		, PreviewSeed(InAssetProxy.PreviewSeed)
		, PreviewStartTime(InAssetProxy.PreviewStartTime)
		, PreviewDuration(InAssetProxy.PreviewDuration)
	{
	}
	
	explicit FResidualDataAssetProxy(const UResidualData* InResidualData, const float PlaySpeed, const int32 Seed,
	                                 const float StartTime, const float Duration, const float PitchShift);
	
	/**
	 * Should only be used in Editor for synthesizing
	 * @param InResidualObj 
	 */
	FResidualDataAssetProxy(UResidualObj* InResidualObj);
	
	const UResidualObj* GetResidualData() const { return ResidualObj; }

	FResidualPhaseEffect GetPhaseEffect() const { return PhaseEffect; }
	const FResidualMagnitudeEffect* GetMagEffect() const { return &MagEffect; }
	
	int32 GetNumFFT() const
	{
		if(ResidualObj)
		{
			return ResidualObj->GetNumFFT();
		}
		else
			return -1;
	}
	
	float GetPreviewPitchShift() const { return PreviewPitchShift; }
	float GetPreviewPlaySpeed() const { return PreviewPlaySpeed; }
	int32 GetPreviewSeed() const { return PreviewSeed; }
	float GetPreviewStartTime() const { return PreviewStartTime; }
	float GetPreviewDuration() const { return PreviewDuration; }
	
	float GetFreqScale(const float InFreq) const;

	float GetAmplitudeScale(const float InTime) const;
	bool HasAmplitudeScaleModifier() const;

protected:
	
	TObjectPtr<UResidualObj> ResidualObj;
	FResidualPhaseEffect PhaseEffect;
	FResidualMagnitudeEffect MagEffect;
	
	FRichCurve ScaleAmplitudeCurve;
	FRichCurve ScaleFreqCurve;
	
	float PreviewPitchShift;
	float PreviewPlaySpeed;
	int32 PreviewSeed;
	float PreviewStartTime;
	float PreviewDuration;
	
private:
	void InitSynthCurveBasedOnSource(const FImpactSynthCurve& InCurve, FRichCurve& OutCurve);
	void GetMagErbRange();
};
