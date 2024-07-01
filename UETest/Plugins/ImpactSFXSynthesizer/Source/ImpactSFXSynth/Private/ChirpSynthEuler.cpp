// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ChirpSynthEuler.h"
#include "ExtendArrayMath.h"
#include "ImpactSFXSynthLog.h"
#include "ModalSynth.h"
#include "DSP/FloatArrayMath.h"
#include "ImpactSFXSynth/Public/Utils.h"

#define INIT_AMP (1e-3f)
#define GAIN_INTERP (0.15f)

namespace LBSImpactSFXSynth
{
	FChirpSynthEuler::FChirpSynthEuler(float InSamplingRate, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
							 const FChirpSynthEulerParams& Params, const int32 NumUsedModals, const float InMaxDuration) 
	: SamplingRate(InSamplingRate), Duration(0.f)
	, MaxDuration(InMaxDuration)
	, CurrentChirpRate(Params.ChirpRate), CurrentPitchScale(Params.PitchScale), CurrentMaxRampDuration(Params.RampDuration)
	, LastRandTime(0.f), LastTimeUpdateChirpParams(0.f)
	{
		if(!ModalsParamsPtr.IsValid())
			return;
		
		TimeResolution = 1.0f / SamplingRate;
		
		const TArrayView<const float> ModalsParams = ModalsParamsPtr->GetParams();
		
		int32 NumModals = ModalsParams.Num() / FModalSynth::NumParamsPerModal;
		NumModals = NumUsedModals > 0 ? FMath::Min(NumModals, NumUsedModals) : NumModals;
		NumTrueModal = NumModals;
		checkf(NumModals > 0, TEXT("NumNodals must be larger than 0!"));

		UpwardGainModalIdx.Empty(NumModals);
		UpwardGainModalTime.Empty(NumModals);
		
		NumUsedParams = FModalSynth::NumParamsPerModal * NumModals;

		//Make sure NumModals is a multiplier of Vector register so all modals can run in SIMD when synthesizing
		const int32 RemVec = NumModals % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER;
		if(RemVec > 0)
			NumModals = NumModals + (AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER - RemVec);
			
		const int32 ClearSize = NumModals * sizeof(float);
		
		ImgBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(ImgBuffer.GetData(), ClearSize);

		RealBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(RealBuffer.GetData(), ClearSize);
		
		QBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(QBuffer.GetData(), ClearSize);
		
		PBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(PBuffer.GetData(), ClearSize);
		
		DecayBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(DecayBuffer.GetData(), ClearSize);
		
		FreqBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(FreqBuffer.GetData(), ClearSize);
		
		for(int i = 0, j = 0; i < NumUsedParams; i++, j++)
		{
			RealBuffer[j] = ModalsParams[i];
			i++;
			
			const float Decay = FMath::Exp(-ModalsParams[i] * Params.DecayScale / SamplingRate);
			DecayBuffer[j] = Decay;
			i++;

			const float FreqScale = ModalsParams[i] * Params.PitchScale;
			FreqBuffer[j] = ModalsParams[i];
			const float Freq = UE_TWO_PI *  FreqScale/ SamplingRate;
			PBuffer[j] = FMath::Cos(Freq) * Decay;
			QBuffer[j] = FMath::Sin(Freq) * Decay;
		}
		
	}
	
	bool FChirpSynthEuler::Synthesize(FMultichannelBufferView& OutAudio, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
	                                  const FChirpSynthEulerParams& Params, const bool bClampOutput)
	{
		if(NumTrueModal > ModalsParamsPtr->GetNumModals())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FChirpSynthEuler::Synthesize Number of modal parameters shouldn't be changed after initializing!"));
			return true;
		}
		
		if(MaxDuration > 0.f && Duration >= MaxDuration)
			return true;
		
		const int32 NumOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		const float FrameTime = NumOutputFrames * TimeResolution; 
		const TArrayView<const float> ModalData = ModalsParamsPtr->GetParams();

		UpdateChirpParamsIfNeeded(ModalData, Params);
		
		if(UpdateBuffers(ModalsParamsPtr, ModalData, Params, FrameTime))
			return true;
		
		TArrayView<float> FirstChannelBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
		if(FMath::IsNearlyEqual(Params.AmpScale, 1.0f, 1e-3f))
		{
			ExtendArrayMath::ArrayImpactModalEuler(RealBuffer, ImgBuffer, PBuffer, QBuffer,
													NumTrueModal, FirstChannelBuffer);	
		}
		else
		{
			ExtendArrayMath::ArrayImpactModalEuler(RealBuffer, ImgBuffer, PBuffer, QBuffer,
													NumTrueModal, Params.AmpScale, FirstChannelBuffer);
		}
		
		if(bClampOutput)
			Audio::ArrayClampInPlace(FirstChannelBuffer, -1.f, 1.f);

		const int32 NumChannels = OutAudio.Num();
		const float* CurrentData = FirstChannelBuffer.GetData();
		for(int i = 1; i < NumChannels; i++)
		{
			FMemory::Memcpy(OutAudio[i].GetData(), CurrentData, NumOutputFrames * sizeof(float));
		}

		const float ChirpDuration = GetCurrentChirpDuration();
		for(int i = 0; i < UpwardGainModalIdx.Num(); i++)
		{
			if(UpwardGainModalTime[i] >= GAIN_INTERP)
			{
				const int32 ModalIdx = UpwardGainModalIdx[i];

				int32 ParamIdx = ModalIdx * FModalSynth::NumParamsPerModal + 1;
				const float Decay =  FMath::Exp(-ModalData[ParamIdx] * Params.DecayScale / SamplingRate);
				DecayBuffer[ModalIdx] = Decay;
				
				const float FreqScale = GetChirpPhiSampling(ModalIdx, Params, ChirpDuration);
				
				PBuffer[ModalIdx] = FMath::Cos(FreqScale) * Decay;
				QBuffer[ModalIdx] = FMath::Sin(FreqScale) * Decay;
					
				UpwardGainModalTime.RemoveAt(i, 1, false);
				UpwardGainModalIdx.RemoveAt(i, 1, false);
			}
			else
				UpwardGainModalTime[i] += FrameTime;
		}
		
		Duration += FrameTime;
		
		return false;
	}

	float FChirpSynthEuler::GetCurrentChirpDuration()
	{
		const float ChirpDuration = Duration - LastTimeUpdateChirpParams;
		return CurrentMaxRampDuration >= 0.f ? FMath::Min(ChirpDuration, CurrentMaxRampDuration) : ChirpDuration;
	}

	bool FChirpSynthEuler::UpdateBuffers(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const TArrayView<const float>& ModalData,
	                                     const FChirpSynthEulerParams& Params, const float FrameTime)
	{
		if(FMath::IsNearlyZero(Params.AmpScale, 1e-3f))
			return true;
		
		bool bIsAllDecayToZero = true;
		const float ChirpDuration = GetCurrentChirpDuration();

		if(ModalsParamsPtr->IsParamChanged())
		{ 
			for(int i = 0, j = 0; i < NumUsedParams; i += FModalSynth::NumParamsPerModal, j++)
			{
				// Only allow amp changed on decay ~= 0.f
				if(!FMath::IsNearlyEqual(DecayBuffer[j], 1.f, 1e-3f))
					continue;

				const float Real = RealBuffer[j];
				const float Img = ImgBuffer[j];
				const float CurrentAmp = FMath::Abs(Real) + FMath::Abs(Img);
				if(FMath::IsNearlyZero(CurrentAmp, 1e-5f))
				{
					RealBuffer[j] = ModalData[i];
					ImgBuffer[j] = 0.f;
				}
				else
				{
					const float Scale = FMath::Abs(ModalData[i]) / CurrentAmp;
					RealBuffer[j] = Real * Scale;
					ImgBuffer[j] = Img * Scale;
				}
			}
		}
		
		if(IsRandomAmplitude(Params) && LastRandTime > Params.RandomRate)
		{
			LastRandTime = 0.f;
			for(int i = 0, j = 0; i < NumUsedParams; i++, j++)
			{
				const float CurrentAmp = FMath::Abs(RealBuffer[j]) + FMath::Abs(ImgBuffer[j]);
				if(FMath::IsNearlyZero(CurrentAmp, 1e-5f))
				{
					i += 2;
					continue;
				}
				
				bIsAllDecayToZero = false;
					
				const float AmpMod = FMath::Abs(ModalData[i]);
				i++;

				float Decay =  FMath::Exp(-ModalData[i] * Params.DecayScale / SamplingRate);
				i++;
				
				if(FMath::IsNearlyEqual(Decay, 1.0f, 1e-3f) && FMath::FRand() > 0.5f)
				{
					const float RandAmp = 2.f * (FMath::FRand() - 0.5f) * Params.AmplitudeScaleRandomRange * AmpMod;
					const float MaxAmp = FMath::Clamp(AmpMod + RandAmp, INIT_AMP, AmpMod * 1.2f);
					
					Decay = FMath::Pow(MaxAmp / CurrentAmp, 1.0f / (GAIN_INTERP * SamplingRate));
						
					UpwardGainModalIdx.Emplace(j);
					UpwardGainModalTime.Emplace(0.f);
				}

				const float Freq = GetChirpPhiSampling(j, Params, ChirpDuration);
				DecayBuffer[j] = Decay;
				PBuffer[j] = FMath::Cos(Freq) * Decay;
				QBuffer[j] = FMath::Sin(Freq) * Decay;
			}
		}
		else
		{
			LastRandTime += FrameTime;
			
			for(int i = 2, j = 0; i < NumUsedParams; i += 3, j++)
			{
				const float CurrentAmp = FMath::Abs(RealBuffer[j]) + FMath::Abs(ImgBuffer[j]);
				if(FMath::IsNearlyZero(CurrentAmp, 1e-5f))
					continue;
				bIsAllDecayToZero = false;
				
				const float Freq = GetChirpPhiSampling(j, Params, ChirpDuration);
				
				PBuffer[j] = FMath::Cos(Freq) * DecayBuffer[j];
				QBuffer[j] = FMath::Sin(Freq) * DecayBuffer[j];
			}
		}

		return bIsAllDecayToZero;
	}


	float FChirpSynthEuler::GetChirpPhiSampling(const int32 ModalIdx, const FChirpSynthEulerParams& Params, const float ChirpDuration)
	{
		return UE_TWO_PI * (FreqBuffer[ModalIdx] * Params.PitchScale  + Params.ChirpRate * ChirpDuration) / SamplingRate;
	}

	float FChirpSynthEuler::GetCurrentEffectiveFreq(const float F0, const float ChirpRate, const float ChirpDuration)
	{
		return F0 + ChirpDuration * ChirpRate * 2.f;
	}

	bool FChirpSynthEuler::IsRandomAmplitude(const FChirpSynthEulerParams& Params)
	{
		return Params.RandomRate > 0.f && Params.AmplitudeScaleRandomRange > 0.f;
	}
	
	void FChirpSynthEuler::UpdateChirpParamsIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthEulerParams& Params)
	{
		if(IsNoChirpChange(Params))		
			return;
		
		UpdateModalBuffers(ModalsParams, Params);
		UpdateCurrentChirpParams(Params);
	}
	
	void FChirpSynthEuler::UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthEulerParams& Params)
	{
		const float ChirpDuration = GetCurrentChirpDuration();
		
		//Update F0 based on ChirpRate to avoid jumping when calculating instant freqs
		if(CurrentMaxRampDuration > 0.f && ChirpDuration >= CurrentMaxRampDuration)
		{
			for(int i = FModalSynth::FreqBin, j = 0; i < NumUsedParams && j < NumTrueModal; i += FModalSynth::NumParamsPerModal, j++)
			{
				const float CurrenFreq = ModalsParams[i] * CurrentPitchScale + CurrentChirpRate * CurrentMaxRampDuration;
				FreqBuffer[j] = CurrenFreq / Params.PitchScale;
			}
		}
		else
		{
			for(int i = FModalSynth::FreqBin, j = 0; i < NumUsedParams && j < NumTrueModal; i += FModalSynth::NumParamsPerModal, j++)
			{
				const float CurrenFreq = GetCurrentEffectiveFreq(ModalsParams[i] * CurrentPitchScale, CurrentChirpRate, ChirpDuration);
				FreqBuffer[j] = (CurrenFreq - Duration * Params.ChirpRate * 2.f) / Params.PitchScale;
			}
		}
	}

	void FChirpSynthEuler::UpdateCurrentChirpParams(const FChirpSynthEulerParams& Params)
    {
    	CurrentPitchScale = Params.PitchScale;
    	CurrentChirpRate = Params.ChirpRate;
    	CurrentMaxRampDuration = GetRampDurationClamp(Params.RampDuration);
		LastTimeUpdateChirpParams = Duration;
    }

	float FChirpSynthEuler::GetRampDurationClamp(const float InValue)
	{
		return InValue;
	}

	bool FChirpSynthEuler::IsNoChirpChange(const FChirpSynthEulerParams& Params)
	{
		const bool bIsSameChirpRate =  FMath::IsNearlyEqual(Params.ChirpRate, CurrentChirpRate, 1e-2f);
		const bool bIsSamePitchScale = FMath::IsNearlyEqual(Params.PitchScale, CurrentPitchScale, 1e-4f);
		const bool bIsSameRampDuration =  FMath::IsNearlyEqual(GetRampDurationClamp(Params.RampDuration), CurrentMaxRampDuration, 1e-4f);
		return bIsSameChirpRate && bIsSamePitchScale && bIsSameRampDuration;
	}
}

namespace LBSImpactSFXSynth
{
	FSigmoidChirpSynthEuler::FSigmoidChirpSynthEuler(float InSamplingRate, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
		const FChirpSynthEulerParams& Params, const int32 NumUsedModals, const float InMaxDuration)
	: FChirpSynthEuler(InSamplingRate, ModalsParamsPtr, Params, NumUsedModals, InMaxDuration)
	, RealChirpRate(0.f), ExpFactor(0.f)
	{
		if(!ModalsParamsPtr.IsValid())
			return;
		
		CurrentMaxRampDuration = FMath::Abs(Params.RampDuration);
		
		const int32 NumData = RealBuffer.Num();
		TargetFreqBuffer.SetNumUninitialized(NumData);
		TArrayView<const float> ModalData = ModalsParamsPtr->GetParams();
        for(int i = FModalSynth::FreqBin, j = 0; i < NumUsedParams; i += FModalSynth::NumParamsPerModal, j++)
        {
        	const float TargetFreq = FMath::Clamp(ModalData[i] * Params.PitchScale, 20.f, FModalSynth::FMax);
        	TargetFreqBuffer[j] = TargetFreq;
        	FreqBuffer[j] = TargetFreq * 0.25f;
        }
	}

	bool FSigmoidChirpSynthEuler::Synthesize(FMultichannelBufferView& OutAudio,
		const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const FChirpSynthEulerParams& Params,
		const bool bClampOutput)
	{
		const float ChirpDuration = GetCurrentChirpDuration();
		
		RealChirpRate = CalculateChirpRate(Params.ChirpRate, Params.RampDuration);
		ExpFactor = RealChirpRate * ChirpDuration;
		
		return FChirpSynthEuler::Synthesize(OutAudio, ModalsParamsPtr, Params, bClampOutput);
	}

	float FSigmoidChirpSynthEuler::GetChirpPhiSampling(const int32 ModalIdx, const FChirpSynthEulerParams& Params, const float ChirpDuration)
	{
		const float F0 = FreqBuffer[ModalIdx];
		const float CurrentFreq = CalculateEffectiveFrequency(F0, TargetFreqBuffer[ModalIdx] - F0);
		return CurrentFreq * UE_TWO_PI / SamplingRate;
	}
	
	float FSigmoidChirpSynthEuler::CalculateEffectiveFrequency(const float F0, const float FreqRange)
	{
		return F0 + FreqRange * (-1.0f + 2.0f / (1.0f + FMath::Exp(-ExpFactor)));
	}
	
	void FSigmoidChirpSynthEuler::UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthEulerParams& Params)
	{
		//Update F0 based on ChirpRate to avoid jumping when calculating instant freqs
		for(int i = FModalSynth::FreqBin, j = 0; i < NumUsedParams && j < NumTrueModal; i += FModalSynth::NumParamsPerModal, j++)
		{
			const float F0 = FreqBuffer[j]; 
			FreqBuffer[j] = CalculateEffectiveFrequency(F0, TargetFreqBuffer[j] - F0);
			TargetFreqBuffer[j] = FMath::Clamp(ModalsParams[i] * Params.PitchScale, 0.f, FModalSynth::FMax);
		}
	}

	float FSigmoidChirpSynthEuler::CalculateChirpRate(const float ChirpRate, const float RampDuration)
    {
    	//At chirp rate = 1.f * 5.f, freq will reach target freq at RamDuration
    	return 5.f * ChirpRate/ GetRampDurationClamp(RampDuration);
    }
	
	float FSigmoidChirpSynthEuler::GetRampDurationClamp(const float InValue)
	{
		return FMath::Max(InValue, 1e-3f);
	}
}

#undef INIT_AMP
#undef GAIN_INTERP