// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ChirpSynth.h"
#include "ExtendArrayMath.h"
#include "ImpactSFXSynthLog.h"
#include "ModalSynth.h"
#include "DSP/FloatArrayMath.h"
#include "ImpactSFXSynth/Public/Utils.h"

namespace LBSImpactSFXSynth
{
	FChirpSynth::FChirpSynth(float InSamplingRate, int32 InNumFramesPerBlock, TArrayView<const float> ModalsParams,
							 const FChirpSynthParams& Params, const int32 NumUsedModals, const float InMaxDuration) 
	: SamplingRate(InSamplingRate), NumFramesPerBlock(InNumFramesPerBlock), Duration(0.f), RandAccumTime(0.f), LastRandTime(0.f)
	, CurrentRandRate(1.f), MaxDuration(InMaxDuration)
	, CurrentChirpRate(Params.ChirpRate), CurrentPitchScale(Params.PitchScale), CurrentMaxRampDuration(Params.RampDuration)
	{
		InitBuffers(ModalsParams, NumUsedModals, Params);
	}

	int32 FChirpSynth::InitBuffers(TArrayView<const float> ModalsParams, const int32 NumUsedModals, const FChirpSynthParams& Params)
	{
		TimeResolution = 1.0f / SamplingRate;
		TimeBuffer.SetNumUninitialized(NumFramesPerBlock);
		ResetTimeBuffer();
		
		int32 NumModals = ModalsParams.Num() / FModalSynth::NumParamsPerModal;
		NumModals = NumUsedModals > 0 ? FMath::Min(NumModals, NumUsedModals) : NumModals; 
		checkf(NumModals > 0, TEXT("NumNodals must be larger than 0!"));
		
		const int32 NumUsedParams = FModalSynth::NumParamsPerModal * NumModals;
		ModalParamsBuffer.SetNumUninitialized(NumUsedParams);
		FMemory::Memcpy(ModalParamsBuffer.GetData(), ModalsParams.GetData(), NumUsedParams * sizeof(float));
		
		bNotAllSameDecayParam = NumModals > 1 ? false : true; //If only one modal then don't have to merge all decay into one
		for(int i = FModalSynth::DecayBin + FModalSynth::NumParamsPerModal; i < NumUsedParams; i += FModalSynth::NumParamsPerModal)
		{
			if(ModalParamsBuffer[i - FModalSynth::NumParamsPerModal] != ModalParamsBuffer[i])
			{
				bNotAllSameDecayParam = true;
				break;
			}
		}
		
		return NumModals;
	}

	bool FChirpSynth::Synthesize(FMultichannelBufferView& OutAudio, TArrayView<const float> ModalsParams,
								 const FChirpSynthParams& Params, const bool bClampOutput)
	{
		if(ModalsParams.Num() < ModalParamsBuffer.Num())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FChirpSynth::Synthesize Number of modal parameters shouldn't be changed on synthesizing!"));
			return true;
		}
		
		if(MaxDuration > 0.f && Duration >= MaxDuration)
			return true;
		
		const int32 NumOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		checkf(NumOutputFrames <= NumFramesPerBlock, TEXT("FModalSynth::Synthesize: NumOutputFrames (%d) > NumFramesPerBlock (%d)"), NumOutputFrames, NumFramesPerBlock);
		
		const int32 NumUsedParams = ModalParamsBuffer.Num();
		const int32 NumModals = NumUsedParams / FModalSynth::NumParamsPerModal;
		if(NumModals * FModalSynth::NumParamsPerModal != NumUsedParams)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FChirpSynth::Synthesize: Number of input parameters (%d) is incorrect!"), NumUsedParams);
			return false;
		}

		UpdateChirpRateIfNeeded(ModalsParams, Params);
		UpdateRandBufferIfNeeded(ModalsParams, Params, NumModals);
		
		TArrayView<float> FirstChannelBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
		const bool bIsFinish = StartSynthesize(FirstChannelBuffer, Params);
		
		if(bClampOutput)
			Audio::ArrayClampInPlace(FirstChannelBuffer, -1.f, 1.f);

		const int32 NumChannels = OutAudio.Num();
		const float* CurrentData = FirstChannelBuffer.GetData();
		for(int i = 1; i < NumChannels; i++)
		{
			FMemory::Memcpy(OutAudio[i].GetData(), CurrentData, NumOutputFrames * sizeof(float));
		}

		const float DeltaTime = NumOutputFrames * TimeResolution; 
		RandAccumTime += DeltaTime;
		Duration += DeltaTime;

		if(!bIsFinish)
			Audio::ArrayAddConstantInplace(TimeBuffer, DeltaTime);
		
		return bIsFinish;
	}

	void FChirpSynth::UpdateRandBufferIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params, const int32 NumModals)
	{
		if(IsRandomAmplitude(Params) && RandAccumTime > Params.RandomRate)
		{
			if(CurrentRandAmpBuffer.Num() != NumModals || LastRandAmpBuffer.Num() != NumModals)
			{ // Skip random on first init
				LastRandAmpBuffer.SetNumUninitialized(NumModals);
				CurrentRandAmpBuffer.SetNumUninitialized(NumModals);
				FMemory::Memzero(LastRandAmpBuffer.GetData(), NumModals * sizeof(float));
				FMemory::Memzero(CurrentRandAmpBuffer.GetData(), NumModals * sizeof(float));
			}
			else
			{
				const float CurrentAlpha = GetCurrentRandAlpha();
				for(int i = 0; i < NumModals; i++)
				{
					LastRandAmpBuffer[i] = FMath::Lerp(LastRandAmpBuffer[i], CurrentRandAmpBuffer[i], CurrentAlpha);
					CurrentRandAmpBuffer[i] = 2.f * (FMath::FRand() - 0.5f) * Params.AmplitudeScaleRandomRange * ModalParamsBuffer[i * FModalSynth::NumParamsPerModal];
				}
			}
			
			CurrentRandRate = FMath::RandRange(Params.RandomRate / 2.0f, Params.RandomRate * 2.f);
			LastRandTime = Duration;
		}

		//Always reset this whether we rand or not
		if(RandAccumTime > Params.RandomRate)
			RandAccumTime = 0.f;
	}

	bool FChirpSynth::StartSynthesize(const TArrayView<float>& OutAudio, const FChirpSynthParams& Params)
	{
		const int32 NumUsedParams = ModalParamsBuffer.Num();
		const int32 NumModals = NumUsedParams / FModalSynth::NumParamsPerModal;
		bool bIsFinish = true;
		const float OutFrameTime = OutAudio.Num() * TimeResolution;
		const float CurrentAlpha = GetCurrentRandAlpha();
		const bool bReachDuration = IsReachRamDuration(); 
		const float ChirpDuration = bReachDuration ? CurrentMaxRampDuration : Duration;   
		for(int i = 0, j = 0; i < NumUsedParams; i += FModalSynth::NumParamsPerModal, j++)
		{
			float Amp = FMath::Clamp(ModalParamsBuffer[i] * Params.AmpScale, -1.f, 1.f);
			//Is the sound small enough to be ignored?
			if(FMath::Abs(Amp) < 1e-4f)
				continue;

			if(IsRandomAmplitude(Params) && LastRandAmpBuffer.Num() == NumModals)
				Amp += FMath::Lerp(LastRandAmpBuffer[j], CurrentRandAmpBuffer[j], CurrentAlpha);
			
			float Decay = - ModalParamsBuffer[i + FModalSynth::DecayBin] * Params.DecayScale;
			ModalParamsBuffer[i] *= FMath::Exp(Decay * OutFrameTime);
			Decay = bNotAllSameDecayParam ? Decay : 0.f; //if all modals have the same decay value, then do decay outside of loop
			 
			const int32 FreqBin = i + FModalSynth::FreqBin;
			const float F0 = ModalParamsBuffer[FreqBin] * CurrentPitchScale;
			const float Freq = GetCurrentEffectiveFreq(F0, Params.ChirpRate, ChirpDuration);
			
			bIsFinish = false;
			if(Freq <= FModalSynth::FMin || Freq >= FModalSynth::FMax || FMath::IsNearlyZero(Params.ChirpRate) || bReachDuration)
			{
				const float PhiSpeed = FMath::Clamp(Freq, FModalSynth::FMin, FModalSynth::FMax) * UE_TWO_PI;
				ExtendArrayMath::ArrayImpactModalDeltaDecay(TimeBuffer, Amp, Decay, PhiSpeed, OutAudio);
				continue;
			}
			
			if(NumModals > 1)
				ExtendArrayMath::ArrayLinearChirpSynth(TimeBuffer, Params.ChirpRate, Amp, Decay, F0, OutAudio);
			else
				ExtendArrayMath::ArrayLinearChirpSynthSingle(TimeBuffer, Params.ChirpRate, Amp, Decay, F0, OutAudio);
		}

		if(!bNotAllSameDecayParam)
		{
			const float Decay = -ModalParamsBuffer[FModalSynth::DecayBin] * Params.DecayScale;
			ExtendArrayMath::ArrayDeltaTimeDecayInPlace(TimeBuffer, Decay, OutAudio);
		}
		
		return bIsFinish;
	}

	float FChirpSynth::GetCurrentEffectiveFreq(const float F0, const float ChirpRate, const float ChirpDuration)
	{
		return F0 + ChirpDuration * ChirpRate * 2.f;
	}

	bool FChirpSynth::IsRandomAmplitude(const FChirpSynthParams& Params)
	{
		return Params.RandomRate > 0.f && Params.AmplitudeScaleRandomRange > 0.f;
	}
	
	bool FChirpSynth::IsReachRamDuration()
	{
		return CurrentMaxRampDuration >= 0.f && Duration >= CurrentMaxRampDuration;
	}
	
	float FChirpSynth::GetCurrentRandAlpha() const
	{
		return FMath::Min(1.0f, (Duration - LastRandTime) / (CurrentRandRate + 1e-12));
	}

	void FChirpSynth::UpdateChirpRateIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params)
	{
		if(IsNoChirpChange(Params))		
			return;
		
		UpdateModalBuffers(ModalsParams, Params);
		UpdateCurrentChirpParams(Params);
	}
	
	void FChirpSynth::UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params)
	{
		const int32 NumUsedParams = ModalParamsBuffer.Num();
		const float ChirpDuration = IsReachRamDuration() ? CurrentMaxRampDuration  : Duration; 
		//Update F0 based on ChirpRate to avoid jumping when calculating instant freqs
		for(int i = FModalSynth::FreqBin; i < NumUsedParams; i += FModalSynth::NumParamsPerModal)
		{
			const float CurrenFreq = GetCurrentEffectiveFreq(ModalParamsBuffer[i] * CurrentPitchScale, CurrentChirpRate, ChirpDuration);
			ModalParamsBuffer[i] = (CurrenFreq - Duration * Params.ChirpRate * 2.f) / Params.PitchScale;
		}
	}

	void FChirpSynth::ResetTimeBuffer()
	{
		for(int i = 0; i < NumFramesPerBlock; i++)
			TimeBuffer[i] = i * TimeResolution;
	}

	void FChirpSynth::UpdateCurrentChirpParams(const FChirpSynthParams& Params)
    {
    	CurrentPitchScale = Params.PitchScale;
    	CurrentChirpRate = Params.ChirpRate;
    	CurrentMaxRampDuration = GetRampDurationClamp(Params.RampDuration);
    }

	float FChirpSynth::GetRampDurationClamp(const float InValue)
	{
		return InValue;
	}

	bool FChirpSynth::IsNoChirpChange(const FChirpSynthParams& Params)
	{
		const bool bIsSameChirpRate =  FMath::IsNearlyEqual(Params.ChirpRate, CurrentChirpRate, 1e-2f);
		const bool bIsSamePitchScale = FMath::IsNearlyEqual(Params.PitchScale, CurrentPitchScale, 1e-4f);
		const bool bIsSameRampDuration =  FMath::IsNearlyEqual(GetRampDurationClamp(Params.RampDuration), CurrentMaxRampDuration, 1e-4f);
		return bIsSameChirpRate && bIsSamePitchScale && bIsSameRampDuration;
	}
}

namespace LBSImpactSFXSynth
{
	FSigmoidChirpSynth::FSigmoidChirpSynth(float InSamplingRate, int32 InNumFramesPerBlock, TArrayView<const float> ModalsParams,
		const FChirpSynthParams& Params, const int32 NumUsedModals, const float InMaxDuration)
	: FChirpSynth(InSamplingRate, InNumFramesPerBlock, ModalsParams, Params, NumUsedModals, InMaxDuration)
	{
		LastChangeFreqTime = 0.f;
		CurrentMaxRampDuration = FMath::Abs(Params.RampDuration);
		bIsAllFrequencyReachTarget = false;
		InitRampBuffers(Params);
	}

	bool FSigmoidChirpSynth::StartSynthesize(const TArrayView<float>& OutAudio, const FChirpSynthParams& Params)
	{
		check(TargetFreqBuffer.Num() == (ModalParamsBuffer.Num() / FModalSynth::NumParamsPerModal));
		
		const int32 NumUsedParams = ModalParamsBuffer.Num();
		const int32 NumModals = NumUsedParams / FModalSynth::NumParamsPerModal;
		bool bIsFinish = true;
		const float OutFrameTime = OutAudio.Num() * TimeResolution;
		const float ChirpDuration = Duration - LastChangeFreqTime;
		const float ChirpRate = CalculateChirpRate(Params.ChirpRate, Params.RampDuration);
		const float CurrentAlpha = GetCurrentRandAlpha();
		bIsAllFrequencyReachTarget = true;
		for(int i = 0, j = 0; i < NumUsedParams; i += FModalSynth::NumParamsPerModal, j++)
		{
			float Amp = FMath::Clamp(ModalParamsBuffer[i] * Params.AmpScale, -1.f, 1.f);
			//Is the sound small enough to be ignored?
			if(FMath::Abs(Amp) < 1e-4f)
				continue;

			float Decay = -ModalParamsBuffer[i + FModalSynth::DecayBin] * Params.DecayScale;
			ModalParamsBuffer[i] *= FMath::Exp(Decay * OutFrameTime);
			Decay = bNotAllSameDecayParam ? Decay : 0.f; //if all modals have the same decay value, then do decay outside of loop
			
			const int32 FreqBin = i + FModalSynth::FreqBin;
			const float F0 = ModalParamsBuffer[FreqBin];
			const float FTarget = TargetFreqBuffer[j];
			const float FreqRange = FTarget - F0;
			
			const float ExpFactor = ChirpDuration * ChirpRate;
			bool bReachFTarget = IsNearlyReachTargetFrequency(F0, FTarget, ExpFactor); 
			const float CurrentFreq =  bReachFTarget ? FTarget : CalculateEffectiveFrequency(F0, FreqRange, ExpFactor);
			CurrentFreqBuffer[j] = CurrentFreq;
			bReachFTarget = bReachFTarget || FMath::IsNearlyEqual(CurrentFreq, FTarget, TargetFrequencyTolerance);
			
			if(IsRandomAmplitude(Params) && LastRandAmpBuffer.Num() == NumModals)
			{
				Amp += FMath::Lerp(LastRandAmpBuffer[j], CurrentRandAmpBuffer[j], CurrentAlpha);
			}

			//Gradually introducing/removing nearly DC signal based on their frequency
			Amp = (CurrentFreq >= FChirpSynth::FreqAmpThreshold) ? Amp : (Amp * CurrentFreq / FChirpSynth::FreqAmpThreshold);
			
			bIsFinish = false;
			
			if(bReachFTarget)
			{
				const float PhiSpeed = (CurrentFreq * UE_TWO_PI);
				ExtendArrayMath::ArrayImpactModalDeltaDecay(TimeBuffer, Amp, Decay, PhiSpeed, OutAudio);
			}
			else
			{
				bIsAllFrequencyReachTarget = false;
				ArrayChirpSynthesize(OutAudio, ChirpRate, Amp, Decay, F0, FreqRange, 0.f);
			}
		}
		
		if(!bNotAllSameDecayParam && Params.DecayScale > 1e-6f)
		{
			const float Decay = -ModalParamsBuffer[FModalSynth::DecayBin] * Params.DecayScale;
			ExtendArrayMath::ArrayDeltaTimeDecayInPlace(TimeBuffer, Decay, OutAudio);
		}
		
		return bIsFinish;
	}
	
	bool FSigmoidChirpSynth::IsNearlyReachTargetFrequency(const float F0, const float FTarget, const float ExpFactor)
	{
		return ExpFactor > IMPS_EXP_FACTOR_MAX || FMath::IsNearlyEqual(F0, FTarget, TargetFrequencyTolerance);
	}
	
	float FSigmoidChirpSynth::CalculateEffectiveFrequency(const float F0, const float FreqRange, const float ExpFactor)
	{
		return F0 + FreqRange * (-1.0f + 2.0f / (1.0f + FMath::Exp(-ExpFactor)));
	}

	void FSigmoidChirpSynth::ArrayChirpSynthesize(const TArrayView<float>& OutAudio, float ChirpRate, float Amp, float Decay, float F0,
												  float FreqRange, const float Bias)
    {
		if(ChirpRate > 0.01f)
		{
			ExtendArrayMath::ArraySigmoidChirpSynth(TimeBuffer, ChirpRate, Amp, Decay, F0, FreqRange, Bias, OutAudio);
		}
		else
		{ //If chirp rate is too smale just use linear for better performance and avoid floating point accuracy problem 
			ChirpRate = FreqRange / CurrentMaxRampDuration;
			ExtendArrayMath::ArrayLinearChirpSynth(TimeBuffer, ChirpRate, Amp, Decay, F0, OutAudio);
		}
    }
	
	void FSigmoidChirpSynth::UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params)
	{
		const int32 NumUsedParams = ModalParamsBuffer.Num();
		check(ModalsParams.Num() >= NumUsedParams);
		
		//Update F0 based on ChirpRate to avoid jumping when calculating instant freqs
		for(int i = FModalSynth::FreqBin, j = 0; i < NumUsedParams; i += FModalSynth::NumParamsPerModal, j++)
		{
			TargetFreqBuffer[j] = FMath::Clamp(ModalsParams[i] * Params.PitchScale, 0.f, FModalSynth::FMax);						
			ModalParamsBuffer[i] = CurrentFreqBuffer[j];			
		}
		
		LastChangeFreqTime = Duration;
		ResetTimeBuffer();
	}

	void FSigmoidChirpSynth::UpdateRandBufferIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params,
	                                                  int32 NumModals)
	{
		if(bIsAllFrequencyReachTarget && Params.RandomRate > 0.f && RandAccumTime > Params.RandomRate && Params.PitchShiftRandomRange > 0.f && IsNoChirpChange(Params))
		{
			int32 NumFreqChange = 0;
			for(int i = FModalSynth::FreqBin, j = 0; j < NumModals; i+= FModalSynth::NumParamsPerModal, j++)
			{
				//Skip low freq modals as they can sound fake
				//Avoid making all modals change pitch at the same time 
				if(NumFreqChange < 3 && FMath::FRand() <= FreqRandChance)
				{
					const float RandScale = GetPitchScaleClamped(2 * (FMath::FRand() - 0.5f) * Params.PitchShiftRandomRange);
					const float TargetFreq = ModalsParams[i] * Params.PitchScale * RandScale;
					TargetFreqBuffer[j] = FMath::Clamp(TargetFreq, 0.f, FModalSynth::FMax);
					NumFreqChange++;
				}
				
				ModalParamsBuffer[i] = CurrentFreqBuffer[j];
			}
			
			LastChangeFreqTime = Duration;
			ResetTimeBuffer();
		}

		FChirpSynth::UpdateRandBufferIfNeeded(ModalsParams, Params, NumModals);
	}
	
	void FSigmoidChirpSynth::InitRampBuffers(const FChirpSynthParams& Params)
	{
		const int32 NumUsedParams = ModalParamsBuffer.Num(); 
		const int32 NumModals = NumUsedParams / FModalSynth::NumParamsPerModal;
		TargetFreqBuffer.SetNumUninitialized(NumModals);
		CurrentFreqBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(CurrentFreqBuffer.GetData(), NumModals * sizeof(float));
		
		for(int i = FModalSynth::FreqBin, j = 0; i < NumUsedParams; i += FModalSynth::NumParamsPerModal, j++)
		{
			const float TargetFreq = FMath::Clamp(ModalParamsBuffer[i] * Params.PitchScale, 0.f, FModalSynth::FMax);
			TargetFreqBuffer[j] = TargetFreq;
			ModalParamsBuffer[i] = TargetFreq * 0.25f;
		}
	}

	float FSigmoidChirpSynth::CalculateChirpRate(const float ChirpRate, const float RampDuration)
    {
    	//At chirp rate = 1.f * 2.5f, freq will reach target freq at RamDuration
    	return 2.5f * ChirpRate/ GetRampDurationClamp(RampDuration);
    }
	
	float FSigmoidChirpSynth::GetRampDurationClamp(const float InValue)
	{
		return FMath::Max(InValue, 1e-3f);
	}
}

namespace LBSImpactSFXSynth
{
	FExponentChirpSynth::FExponentChirpSynth(float InSamplingRate, int32 InNumFramesPerBlock,
		TArrayView<const float> ModalsParams, const FChirpSynthParams& Params, const int32 NumUsedModals,
		const float InMaxDuration)
		: FSigmoidChirpSynth(InSamplingRate, InNumFramesPerBlock, ModalsParams, Params, NumUsedModals, InMaxDuration)
	{
		
	}

	bool FExponentChirpSynth::IsNearlyReachTargetFrequency(float F0, float FTarget, float ExpFactor)
	{
		return ExpFactor > IMPS_EXP_FACTOR_MAX || FMath::IsNearlyEqual(F0, FTarget, TargetFrequencyTolerance);
	}

	float FExponentChirpSynth::CalculateEffectiveFrequency(float F0, float FreqRange, float ExpFactor)
	{
		return F0 + FreqRange * (-1.0f + 2.0f / (1.0f + FMath::Exp(-ExpFactor)));
	}

	void FExponentChirpSynth::ArrayChirpSynthesize(const TArrayView<float>& OutAudio, float ChirpRate, float Amp, float Decay, float F0, float FreqRange, const float Bias)
	{
		if(ChirpRate > 0.05f)
		{
			ExtendArrayMath::ArrayExponentChirpSynth(TimeBuffer, ChirpRate, Amp, Decay, F0, FreqRange, Bias, OutAudio);
		}
		else
		{ //If chirp rate is too smale just use linear for better performance and avoid floating point accuracy problem 
			ChirpRate = FreqRange / CurrentMaxRampDuration;
			ExtendArrayMath::ArrayLinearChirpSynth(TimeBuffer, ChirpRate, Amp, Decay, F0, OutAudio);
		}
	}
	
	float FExponentChirpSynth::CalculateChirpRate(const float ChirpRate, const float RampDuration)
	{
		//At chirp rate = 1.f * 5.f, freq will reach target freq at RamDuration
		return 5.f * ChirpRate / GetRampDurationClamp(RampDuration);
	}
}