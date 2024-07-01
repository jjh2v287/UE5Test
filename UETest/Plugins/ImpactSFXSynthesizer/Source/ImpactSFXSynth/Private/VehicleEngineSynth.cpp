// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "VehicleEngineSynth.h"

#include "ExtendArrayMath.h"
#include "ImpactSFXSynthLog.h"
#include "ModalSynth.h"

#define FREQ_BASE (100.0f)
#define EXPONENT (2.718281828459045f)
#define INIT_AMP (1e-3f)
#define GAIN_INTERP (0.15f)

namespace LBSImpactSFXSynth
{
	FVehicleEngineSynth::FVehicleEngineSynth(const float InSamplingRate, const int32 InNumPulsePerCycle,
													const FImpactModalObjAssetProxyPtr& ModalsParams,
													const int32 NumModal, const float InHarmonicGain, const float InHarmonicFreqScale,
													const int32 InSeed)
		: SamplingRate(InSamplingRate), LastFreq(0.f), BaseFreq(0.f),
	     LastHarmonicRand(0.f), HarmonicGain(InHarmonicGain), HarmonicFreqScale(InHarmonicFreqScale),
		 CurrentNumModalUsed(0), PrevRPM(0.f), DecelerationTimer(0.f), bIsInDeceleration(false)
	{
		Seed = InSeed > -1 ? InSeed : FMath::Rand();
		RandomStream = FRandomStream(Seed);
		TimeStep = 1.f / SamplingRate;
		FrameTime = 0.f;
		
		NumPulsePerCycle = FMath::Max(1, InNumPulsePerCycle);
		RPMBaseLine = FREQ_BASE / NumPulsePerCycle * 60.f;

		if(ModalsParams.IsValid())
			InitBuffers(ModalsParams->GetParams(), NumModal);
	}

	void FVehicleEngineSynth::InitBuffers(const TArrayView<const float>& ModalsParams, const int32 NumUsedModals)
	{
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
		PBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(QBuffer.GetData(), ClearSize);
		FMemory::Memzero(PBuffer.GetData(), ClearSize);

		DecayBuffer.SetNumUninitialized(NumModals);
		
		for(int i = 0, j = 0; i < NumUsedParams; i+=3, j++)
		{
			RealBuffer[j] = ModalsParams[i] * HarmonicGain;
			DecayBuffer[j] = 1.0f;
		}
		
	}

	void FVehicleEngineSynth::Generate(FMultichannelBufferView& OutAudio, const FVehicleEngineParams& Params, const FImpactModalObjAssetProxyPtr& ModalsParams)
	{
		const int32 NumOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		if(NumOutputFrames <= 0 || Params.RPM < 1e-5f || !ModalsParams.IsValid())
			return;
		
		FrameTime = TimeStep * NumOutputFrames;
		
		LastFreq = BaseFreq;
		BaseFreq = FMath::Clamp(Params.RPM / 60.f * NumPulsePerCycle, 30.0f, 18000.0f);
		
		const TArrayView<float> HarmonicBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
		const TArrayView<const float> ModalData = ModalsParams->GetParams();

		const float RPMFreqRate = FMath::Max((Params.RPM - RPMBaseLine) / RPMBaseLine, -0.8f);

		const float DeltaRPM = Params.RPM - LastFreq / NumPulsePerCycle * 60.f;
		RPMCurve = FMath::Sqrt(FMath::Abs(DeltaRPM) * 10.f) * Params.RPMNoiseFactor;
		RPMCurve = FMath::Min(RPMCurve, 1.f);
		const float RPMChangeFactor = FMath::Max(Params.RPMNoiseFactor, RPMCurve);
		const float DeltaRand = Params.RandPeriod / FMath::Max(1.0f, RPMChangeFactor * 2.f);
		const float MaxAmpRand = Params.AmpRandMin * 2.0f;
		const float AmpRandRange = (Params.AmpRandMax - Params.AmpRandMin) * RandomStream.FRand() * FMath::Sign(DeltaRPM) * RPMCurve;
		const float FreqVar = Params.HarmonicFluctuation * RPMChangeFactor / 5.0f;
		
		FindRPMSlope(Params);
		const float RandChance = FMath::Clamp(RPMCurve * 2.f, 0.5f, 1.f);
		if(LastHarmonicRand > DeltaRand || ModalsParams->IsParamChanged())
		{
			LastHarmonicRand = 0.f;
			for(int i = 0, j = 0; i < NumUsedParams; i++, j++)
			{
				if(RandomStream.FRand() > RandChance)
				{
					i += 2;
					continue;
				}
					
				const float AmpMod = FMath::Abs(ModalData[i]) * HarmonicGain;
				i += 2;
				const float Freq = ModalData[i] * HarmonicFreqScale;

				const float RandAmp = FMath::Max(0.f, (1.0f + (RandomStream.FRand() - 0.5f) * MaxAmpRand + AmpRandRange * RandomStream.FRand()));
				const float MaxAmp = FMath::Min(AmpMod * RandAmp, AmpMod * 1.25f);
				
				const float FreqRPM = GetRandFreqPerSamplingRate(Params, RPMFreqRate, FreqVar, Freq) * UE_TWO_PI;
				
				float Decay = 1.f;
					
				if(MaxAmp > 0.f)
				{
					const float CurrentAmp = FMath::Abs(RealBuffer[j]) + FMath::Abs(ImgBuffer[j]);
					if(FMath::IsNearlyZero(CurrentAmp, 1e-5f))
					{
						RealBuffer[j] = INIT_AMP;
						ImgBuffer[j] = 0.f;

						Decay = FMath::Pow(MaxAmp / INIT_AMP, 1.0f / (GAIN_INTERP * SamplingRate)); 
					}
					else
						Decay = FMath::Pow(MaxAmp / CurrentAmp, 1.0f / (GAIN_INTERP * SamplingRate));
						
					UpwardGainModalIdx.Emplace(j);
					UpwardGainModalTime.Emplace(0.f);
				}
				else
					Decay = FMath::Exp(-(5.f + RandomStream.FRand() * 5.f) / SamplingRate);
					
				DecayBuffer[j] = Decay;
				PBuffer[j] = FMath::Cos(FreqRPM) * Decay;
				QBuffer[j] = FMath::Sin(FreqRPM) * Decay;
			}
		}
		else
		{
			LastHarmonicRand += FrameTime;
			
			if(BaseFreq != LastFreq)
			{
				for(int i = 2, j = 0; i < NumUsedParams; i += 3, j++)
				{
					const float FreqScale = (ModalData[i] * HarmonicFreqScale);
					const float FreqRPM = GetRandFreqPerSamplingRate(Params, RPMFreqRate, FreqVar, FreqScale) * UE_TWO_PI;
					
					PBuffer[j] = FMath::Cos(FreqRPM) * DecayBuffer[j];
					QBuffer[j] = FMath::Sin(FreqRPM) * DecayBuffer[j];
				}
			}
		}
		
		if(CurrentNumModalUsed == 1)
		{
			for(int i = 0; i < NumOutputFrames; i++)
			{
				const float Real = RealBuffer[0] * PBuffer[0] - ImgBuffer[0] * QBuffer[0];
				const float Img = ImgBuffer[0] * PBuffer[0] + RealBuffer[0] * QBuffer[0];
				RealBuffer[0] = Real;
				ImgBuffer[0] = Img;
				HarmonicBuffer[i] = Real;
			}
		}
		else
		{
			ExtendArrayMath::ArrayImpactModalEuler(RealBuffer, ImgBuffer, PBuffer, QBuffer,
													  CurrentNumModalUsed, HarmonicBuffer);
		}
			
		for(int i = 0; i < UpwardGainModalIdx.Num(); i++)
		{
			if(UpwardGainModalTime[i] >= GAIN_INTERP)
			{
				const int32 ModalIdx = UpwardGainModalIdx[i];

				const float FreqScale = (ModalData[ModalIdx * FModalSynth::NumParamsPerModal + 2] * HarmonicFreqScale);
				const float FreqRPM = GetRandFreqPerSamplingRate(Params, RPMFreqRate, FreqVar, FreqScale) * UE_TWO_PI;
				
				PBuffer[ModalIdx] = FMath::Cos(FreqRPM);
				QBuffer[ModalIdx] = FMath::Sin(FreqRPM);
				DecayBuffer[ModalIdx] = 1.0f;
					
				UpwardGainModalTime.RemoveAt(i, 1, false);
				UpwardGainModalIdx.RemoveAt(i, 1, false);
			}
			else
				UpwardGainModalTime[i] += FrameTime;
		}
	}

	float FVehicleEngineSynth::GetRandFreqPerSamplingRate(const FVehicleEngineParams& Params, const float RPMFreqRate,
	                                                       const float FreqVar, const float Freq) const
	{
		float FreqRPM = Freq * (1.f + RPMFreqRate);
		const float HarmonicScale = FMath::Sqrt(FreqRPM / BaseFreq) * FreqVar;
		FreqRPM += (RandomStream.FRand() - 0.5f) * (FreqRPM * HarmonicScale + Params.F0Fluctuation);
		return FreqRPM / SamplingRate;
	}

	void FVehicleEngineSynth::FindRPMSlope(const FVehicleEngineParams& Params)
	{
		const float DeltaRPM = Params.RPM - PrevRPM;
		PrevRPM = Params.RPM;
		if(FMath::IsNearlyZero(Params.ThrottleInput, 1e-1))
		{
			if(Params.RPM < Params.IdleRPM)
				SetNonDecelerationMode(Params);
			else
				SetDecelerationMode(Params);
			return;
		}
		
		// if(FMath::Abs(DeltaRPM) < 5.f)
		// {
		// 	if(FMath::Abs(Params.ThrottleInput) > 0.1f || Params.RPM < Params.IdleRPM)
		// 		SetNonDecelerationMode(Params);
		//
		// 	return;
		// }
		
		DecelerationTimer += FrameTime;
		if(DeltaRPM < -50.f)
		{
			SetDecelerationMode(Params);
		}
		else if (DeltaRPM > -20.f && DecelerationTimer > 0.1f)
			SetNonDecelerationMode(Params);
	}

	void FVehicleEngineSynth::SetDecelerationMode(const FVehicleEngineParams& Params)
	{
		CurrentNumModalUsed = Params.NumModalsDeceleration > 0 ? (FMath::Min(NumTrueModal, Params.NumModalsDeceleration)) : NumTrueModal;
		bIsInDeceleration = true;
		DecelerationTimer = 0.f;
	}

	void FVehicleEngineSynth::SetNonDecelerationMode(const FVehicleEngineParams& Params)
	{
		CurrentNumModalUsed = NumTrueModal;
		bIsInDeceleration = false;
	}
}

#undef FREQ_BASE
#undef EXPONENT
#undef INIT_AMP
#undef GAIN_INTERP