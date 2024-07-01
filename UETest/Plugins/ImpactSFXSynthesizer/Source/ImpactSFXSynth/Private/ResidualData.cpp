// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "ResidualData.h"

#include "ResidualObj.h"
#include "ImpactSFXSynth/Public/Utils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ResidualData)

#if WITH_EDITOR
void UResidualData::SetResidualObj(UResidualObj* InObj)
{
	if(InObj)
	{
		ResidualObj = InObj;
		NumFFT = InObj->GetNumFFT();
		NumErb = InObj->GetNumErb();
		NumFrame = InObj->GetNumFrame();
		SamplingRate = InObj->GetSamplingRate();
	}
}
#endif

TSharedPtr<Audio::IProxyData, ESPMode::ThreadSafe> UResidualData::CreateProxyData(
		const Audio::FProxyDataInitParams& InitParams)
{
	return GetProxy();
}

TSharedPtr<FResidualDataAssetProxy> UResidualData::CreateNewResidualProxyData() 
{
	return MakeShared<FResidualDataAssetProxy, ESPMode::ThreadSafe>(this, PreviewPlaySpeed, PreviewSeed, PreviewStartTime, PreviewDuration, PreviewPitchShift);
}

FResidualDataAssetProxyPtr UResidualData::GetProxy()
{
#if WITH_EDITOR
	//Always return new proxy in editor as we can edit this data after MetaSound Graph is loaded
	return CreateNewResidualProxyData();
#endif
	
	if (!Proxy.IsValid())
	{
		Proxy = CreateNewResidualProxyData();
	}
	return Proxy;
}

float UResidualData::Freq2Erb(const float Freq)
{
	return 9.265f * FMath::Loge(1.f + Freq / 228.8455f); // 24.7f * 9.265f
}

float UResidualData::Erb2Freq(const float Erb)
{
	return 228.8455f * (FMath::Exp(Erb / 9.265f) - 1.0f); 
}

#if WITH_EDITOR

void UResidualData::PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
{
	if(const FProperty* Property = InPropertyChangedEvent.Property)
	{
		const FName PropertyName = Property->GetFName();
		const FName ResidualObjName = GET_MEMBER_NAME_CHECKED(UResidualData, ResidualObj);
		if (PropertyName == ResidualObjName)
		{
			UpdateResidualObjProperties();
		}
		else
			CheckAllCurveShared(Property, PropertyName);
	}
	
	UObject::PostEditChangeProperty(InPropertyChangedEvent);
}

void UResidualData::CheckAllCurveShared(const FProperty* Property, const FName PropertyName)
{
	const FName CurveSharedName = GET_MEMBER_NAME_CHECKED(FImpactSynthCurve, CurveShared);
	if (PropertyName != CurveSharedName)
		return;

	//TODO: Should Check for property owner here
	const FFloatRange ValueRange = FFloatRange(TRangeBound<float>(0.f), TRangeBound<float>());
	CheckCurveShared(AmplitudeOverTimeCurve, FFloatRange(), ValueRange);
	CheckCurveShared(AmplitudeOverFreqCurve, FFloatRange(TRangeBound<float>(0.f), TRangeBound<float>(1.f)), ValueRange);
}

void UResidualData::CheckCurveShared(FImpactSynthCurve& InCurve, const FFloatRange InTimeRange, const FFloatRange InValueRange)
{
	if(InCurve.CurveShared)
	{
		if(InValueRange.HasLowerBound() || InValueRange.HasUpperBound())
		{
			float MinValue;
			float MaxValue;
			InCurve.CurveShared->GetValueRange(MinValue, MaxValue);
			
			if(	(InValueRange.HasLowerBound() && MinValue < InValueRange.GetLowerBoundValue())
				|| (InValueRange.HasUpperBound() && MaxValue > InValueRange.GetUpperBoundValue())
			  )
			{
				InCurve.CurveShared = nullptr;
			}

			if(InCurve.CurveShared == nullptr && OnResidualDataError.IsBound())
			{
				const FString Title = TEXT("Invalid Y Axis (Value) Range!");
				FString Message;				
				if(InValueRange.HasLowerBound() && InValueRange.HasUpperBound())
					Message = FString::Printf(TEXT("Values must be in [%.1f, %.1f] range!"), InValueRange.GetLowerBoundValue(), InValueRange.GetUpperBoundValue());	
				else if (InValueRange.HasLowerBound())
					Message = FString::Printf(TEXT("Values must be >= %.1f!"), InValueRange.GetLowerBoundValue());
				else
					Message = FString::Printf(TEXT("Values must be <= %.1f!"), InValueRange.GetUpperBoundValue());
				OnResidualDataError.Execute(*Title, *Message);
				return;
			}
		}

		if(InTimeRange.HasLowerBound() || InTimeRange.HasUpperBound())
		{
			float MinValue;
			float MaxValue;
			InCurve.CurveShared->GetTimeRange(MinValue, MaxValue);
			
			if(	(InTimeRange.HasLowerBound() && MinValue < InTimeRange.GetLowerBoundValue())
				|| (InTimeRange.HasUpperBound() && MaxValue > InTimeRange.GetUpperBoundValue())
			  )
			{
				InCurve.CurveShared = nullptr;
			}

			if(InCurve.CurveShared == nullptr && OnResidualDataError.IsBound())
			{
				const FString Title = TEXT("Invalid X Axis (Time) Range!");
				FString Message;
				if(InTimeRange.HasLowerBound() && InTimeRange.HasUpperBound())
					Message = FString::Printf(TEXT("Time must be in [%.1f, %.1f] range!"), InTimeRange.GetLowerBoundValue(), InTimeRange.GetUpperBoundValue());	
				else if (InTimeRange.HasLowerBound())
					Message = FString::Printf(TEXT("Time must be >= %.1f!"), InTimeRange.GetLowerBoundValue());
				else
					Message = FString::Printf(TEXT("Time must be <= %.1f!"), InTimeRange.GetUpperBoundValue());
				OnResidualDataError.Execute(*Title, *Message);
				return;
			}
		}
	}
}

void UResidualData::UpdateResidualObjProperties()
{
	if(ResidualObj)
	{
		NumFFT = ResidualObj->GetNumFFT();
		if(!LBSImpactSFXSynth::IsPowerOf2(NumFFT))
		{
			if(OnResidualDataError.IsBound())
				OnResidualDataError.Execute(FString(TEXT("Invalid Residual Object")), FString(TEXT("Number of FFT must be a power of 2!")));
			ResidualObj = nullptr;
			return;
		}
		
		NumErb = ResidualObj->GetNumErb();
		NumFrame = ResidualObj->GetNumFrame();
		SamplingRate = ResidualObj->GetSamplingRate();
		
		if(NumErb * NumFrame != ResidualObj->Data.Num())
		{
			if(OnResidualDataError.IsBound())
				OnResidualDataError.Execute(FString(TEXT("Invalid Residual Object")), FString(TEXT("Data size is incorrect!")));
			ResidualObj = nullptr;
			return;
		}
	}
	else
	{
		NumFFT = 0;
		NumErb = 0;
		NumFrame = 0;
		SamplingRate = 0;
	}
}

void UResidualData::PostEditChangeChainProperty(FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	UObject::PostEditChangeChainProperty(InPropertyChangedEvent);
}

void UResidualData::PlaySynthesizedSound() const
{
	if(OnResidualDataSynthesized.IsBound())
		OnResidualDataSynthesized.Execute();
}

void UResidualData::StopPlaying() const
{
	if(OnStopPlayingResidualData.IsBound())
		OnStopPlayingResidualData.Execute();
}

int32 UResidualData::GetNumCurves() const
{
	int32 Count = 0;
	if(AmplitudeOverTimeCurve.CurveSource != EImpactSynthCurveSource::None)
		Count++;
	if(AmplitudeOverFreqCurve.CurveSource != EImpactSynthCurveSource::None)
		Count++;
	
	return Count;
}
#endif

FResidualDataAssetProxy::FResidualDataAssetProxy(const UResidualData* InResidualData, const float PlaySpeed,
												const int32 Seed, const float StartTime, const float Duration, const float PitchShift)
{
	ResidualObj = InResidualData->ResidualObj;
	PhaseEffect = InResidualData->GetPhaseEffect();
	
	InitSynthCurveBasedOnSource(InResidualData->AmplitudeOverTimeCurve, ScaleAmplitudeCurve);
	InitSynthCurveBasedOnSource(InResidualData->AmplitudeOverFreqCurve, ScaleFreqCurve);
	PreviewPitchShift = PitchShift;
	PreviewPlaySpeed = PlaySpeed;
	PreviewSeed = Seed;
	PreviewStartTime = StartTime;
	PreviewDuration = Duration;
	
	MagEffect = InResidualData->GetMagEffect();
	GetMagErbRange();
}

FResidualDataAssetProxy::FResidualDataAssetProxy(UResidualObj* InResidualObj)
		: PreviewPitchShift(0.f)
		, PreviewPlaySpeed(1.f)
		, PreviewSeed(-1)
		, PreviewStartTime(0.f)
		, PreviewDuration(-1.f)
{
	ResidualObj = InResidualObj;

	PhaseEffect = FResidualPhaseEffect();
	PhaseEffect.EffectType = EResidualPhaseGenerator::Random;
	
	MagEffect = FResidualMagnitudeEffect();
	GetMagErbRange();
	
	ScaleAmplitudeCurve.Reset();
	ScaleFreqCurve.Reset();
}

float FResidualDataAssetProxy::GetFreqScale(const float InFreq) const
{
	if(ScaleFreqCurve.IsEmpty())
		return 1.f;
	
	const float KeyTime = FMath::Clamp(UResidualData::Freq2Erb(InFreq) / FImpactSynthCurve::ErbMax, 0.f, 1.f);
	const float OutValue = ScaleFreqCurve.Eval(KeyTime, 1.f);
	return FMath::Max(OutValue, 0.f);
}

float FResidualDataAssetProxy::GetAmplitudeScale(const float InTime) const
{
	if(ScaleAmplitudeCurve.IsEmpty())
		return 1.f;
	
	return ScaleAmplitudeCurve.Eval(InTime, 1.f);
}

bool FResidualDataAssetProxy::HasAmplitudeScaleModifier() const
{
	if(ScaleAmplitudeCurve.IsEmpty())
		return false;

	return true;
}

void FResidualDataAssetProxy::InitSynthCurveBasedOnSource(const FImpactSynthCurve& InCurve, FRichCurve& OutCurve)
{
	OutCurve.Reset();
	switch (InCurve.CurveSource)
	{
	case EImpactSynthCurveSource::Custom:
		OutCurve = InCurve.CurveCustom; //Just copy data
		break;
	case EImpactSynthCurveSource::Shared:
		if(InCurve.CurveShared)
		{
			OutCurve = InCurve.CurveShared->FloatCurve;
		}
		break;
	default:
		break;
	}
}

void FResidualDataAssetProxy::GetMagErbRange()
{
	if(ResidualObj)
	{
		MagEffect.ErbMinShift = FMath::Min(ResidualObj->GetErbMax() - 0.1f, ResidualObj->GetErbMin() + MagEffect.ErbMinShift);
		MagEffect.ErbMaxShift = FMath::Max(ResidualObj->GetErbMin() + 0.1f, ResidualObj->GetErbMax() + MagEffect.ErbMaxShift);
		if(MagEffect.ErbMinShift >= MagEffect.ErbMaxShift)
			MagEffect.ErbMinShift = MagEffect.ErbMaxShift - 0.1f;
	}
}
