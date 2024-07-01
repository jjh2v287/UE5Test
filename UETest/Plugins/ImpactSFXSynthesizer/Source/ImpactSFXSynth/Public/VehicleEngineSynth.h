// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactModalObj.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	struct FVehicleEngineParams
	{
		float RPM = 600.f;
		float IdleRPM = 600.f;
		float RPMNoiseFactor = 0.01f;
		float RandPeriod = 0.1f;
		float ThrottleInput = 1.f;
		
		float F0Fluctuation = 5.f;
		float HarmonicFluctuation = 1.f;
		
		float AmpRandMin = 0.1f;
		float AmpRandMax = 0.5f;

		int32 NumModalsDeceleration = 4;
		
		FVehicleEngineParams() { }

		FVehicleEngineParams(const float InRPM,
							const float InIdleRPM,
							const float InRPMNoiseFactor,
							const float InThrottleInput,
							const float InRandPeriod,
							const float InF0Fluctuation, const float InHarmonicFluctuation, 
							const float InAmpRandMin, const float InAmpRandMax,
							const int32 InNumModalsDeceleration)
		: RPM(InRPM),
		  RPMNoiseFactor(InRPMNoiseFactor),
		  RandPeriod(InRandPeriod),
		  ThrottleInput(InThrottleInput),
		  F0Fluctuation(InF0Fluctuation),		  
		  AmpRandMin(InAmpRandMin),
		  AmpRandMax(InAmpRandMax),
          NumModalsDeceleration(InNumModalsDeceleration)
		{
			HarmonicFluctuation = FMath::Max(0.f, InHarmonicFluctuation);
			IdleRPM = FMath::Max(1.f, InIdleRPM + 1.f); //Plus 1 for a small offset
		}
	};
	
	class IMPACTSFXSYNTH_API FVehicleEngineSynth
	{
	public:

		FVehicleEngineSynth(const float InSamplingRate, const int32 InNumPulsePerCycle,
							  const FImpactModalObjAssetProxyPtr& ModalsParams, const int32 NumModal,
							  const float InHarmonicGain, const float InHarmonicFreqScale,
							  const int32 InSeed = -1);

		void Generate(FMultichannelBufferView& OutAudio, const FVehicleEngineParams& Params, const FImpactModalObjAssetProxyPtr& ModalsParams);

		float GetCurrentBaseFreq() const { return BaseFreq; }
		bool IsInDeceleration() const { return bIsInDeceleration; }
		float GetRPMCurve() const { return  RPMCurve; }
		
	protected:
		void InitBuffers(const TArrayView<const float>& ModalsParams, const int32 NumUsedModals);
		
		FORCEINLINE float GetRandFreqPerSamplingRate(const FVehicleEngineParams& Params, float RPMFreqRate, float FreqVar, float Freq) const;

		void FindRPMSlope(const FVehicleEngineParams& Params);

		void SetDecelerationMode(const FVehicleEngineParams& Params);
		void SetNonDecelerationMode(const FVehicleEngineParams& Params);
	
	private:
		float SamplingRate;
		
		int32 Seed;
		int32 NumPulsePerCycle;
		float RPMBaseLine;
		float LastFreq;
		float BaseFreq;
		
		FRandomStream RandomStream;

		float TimeStep;
		float FrameTime;
		
		int32 NumUsedParams;
		int32 NumTrueModal;
		float LastHarmonicRand;
		float HarmonicGain;
		float HarmonicFreqScale;
		FAlignedFloatBuffer RealBuffer;
		FAlignedFloatBuffer ImgBuffer;
		FAlignedFloatBuffer QBuffer;
		FAlignedFloatBuffer PBuffer;
		FAlignedFloatBuffer DecayBuffer;

		int32 CurrentNumModalUsed;
		
		TArray<int32> UpwardGainModalIdx;
		TArray<float> UpwardGainModalTime;

		float PrevRPM;
		float DecelerationTimer;
		bool bIsInDeceleration;
		float RPMCurve;
	};
}