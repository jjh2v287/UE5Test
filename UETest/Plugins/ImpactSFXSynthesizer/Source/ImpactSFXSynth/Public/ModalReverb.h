// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactModalObj.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;
	
	struct FModalReverbParams
	{
		float FreqScatterScale;
		float PhaseScatterScale;
		float DecayMin;
		float DecaySlopeB;
		float DecaySlopeC;
		
		FModalReverbParams()
		{
			FreqScatterScale = 0.02f;
			PhaseScatterScale = 0.02f;
			DecayMin = 2.f;
			DecaySlopeB = 0.02f;
			DecaySlopeC = 0.23f;
		}

		FModalReverbParams(float InFreqScatterScale, float InPhaseScatterScale,
							float InDecayMin, float InDecaySlopeB, float InDecaySlopeC)
		:   PhaseScatterScale(InPhaseScatterScale),
			DecayMin(InDecayMin), DecaySlopeB(InDecaySlopeB), DecaySlopeC(InDecaySlopeC)
		{
			FreqScatterScale = FMath::Clamp(InFreqScatterScale, 0.f, 0.5f);
		}
	};
	
	class IMPACTSFXSYNTH_API FModalReverb
	{
	public:
		static constexpr int32 MaxSupportNumModals = 1024;
		
		static constexpr float LowBandFMin = 30.f;
		static constexpr float LowBandFMax = 200.f;
		static constexpr float MidBandFMin = 205.f;
		static constexpr float MidBandFMax = 4000.f;
		static constexpr float HighBandFMin = 4050.f;
		static constexpr float HighBandFMax = 18000.f;

		static constexpr float DefaultAbsorption = 2.f;
		static constexpr float NoReverbAbsorption = 1000.f;
		static constexpr float MaxRoomSize = 5000.f;
		static constexpr float MaxOpenRoomFactor = 10.f;
		static constexpr float DefaultDelay = 0.01f;
		
		FModalReverb(const float InSamplingRate, const FModalReverbParams& InReverbParams,
					 const int32 NumModalsLow, const int32 NumModalsMid, const int32 NumModalsHigh,
		             const float InAbsorption, const float InRoomSize, const float InOpenFactor, const float InGain);
		
		bool CreateReverb(TArrayView<float>& OutAudio, const TArrayView<const float>& InAudio,
						  const float InAbsorption, const float InRoomSize, const float InOpenRoomFactor,
						  const float InGain, bool bIsInAudioStop = true);
		
	protected:
		void SetEnvCoefs(const float InAbsorption, float InRoomSize, float InOpenRoomFactor, const float InGain);
		
		void InitBuffers(const int32 NumModalsLow, const int32 NumModalsMid, const int32 NumModalsHigh);
		void InitBufferBand(const float FMin, const float FMax, const int32 NumModals, const float BufferIndex, const float GainScale);
		FORCEINLINE float GetDecayFromFreqScale(float FreqScale, float FreqScaleSqr) const;
		FORCEINLINE float GetAmpFromFreqScale(float FreqScale, float FreqScaleSqr) const;
		
		void GetNumUsedModals(const bool bIsForceStop);
		void ScatterFreqAndPhase();
		
		void ProcessAudio(TArrayView<float>& OutAudio, const TArrayView<const float>& InAudio,
		                       const int32 NumOutputFrames);

		FORCEINLINE float ProcessOneSample(const float* TwoRCosData, const float* R2Data, const float* GainFData, const float* GainCData,
					float* OutL1BufferPtr, float* OutL2BufferPtr, const VectorRegister4Float& ThresholdReg,
					const VectorRegister4Float& InAudio1DReg, const VectorRegister4Float& InAudioReg) const;
	
	private:
		float SamplingRate;
		float LastInAudioSample;
		float AmpScale;
		float BaseDecay;
		float OutDoorMinDecay;
		
        FModalReverbParams ReverbParams;
    
        float TimeStep;
        int32 CurrentNumModals;
		int32 MaxNumModals;

        FAlignedFloatBuffer FreqBuffer;
        FAlignedFloatBuffer PhaseBuffer;
		FAlignedFloatBuffer GainBuffer;
        FAlignedFloatBuffer TwoDecayCosBuffer;
        FAlignedFloatBuffer DecaySqrBuffer;
        FAlignedFloatBuffer Activation1DBuffer;
        FAlignedFloatBuffer ActivationBuffer;
        FAlignedFloatBuffer OutL1Buffer;
        FAlignedFloatBuffer OutL2Buffer;
	};
}