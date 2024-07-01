// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactModalObj.h"
#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/MultichannelBuffer.h"
#include "SynthParamPresets.h"

namespace LBSImpactSFXSynth
{
	
	using namespace Audio;
	
	class IMPACTSFXSYNTH_API FModalSynth
	{
		
	public:
		static constexpr int32 NumParamsPerModal = 3;
		static constexpr int32 AmpBin = 0;
		static constexpr int32 DecayBin = 1;
		static constexpr int32 FreqBin = 2;
		
		static constexpr float FMax = 20000.f;
		static constexpr float FMin = 20.f;
		static constexpr float DecayMin = 0.f;
		static constexpr float DecayMax = 1000.f;
		
		FModalSynth(float InSamplingRate, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
					const float InStartTime = 0.f, const float InDuration = -1.f, const int32 NumUsedModals = -1,
					const float AmplitudeScale = 1.f, const float DecayScale = 1.f, const float FreqScale = 1.f,
					const float InImpactStrengthScale = 1.f, const float InDampingRatio = 0.5f, const float InDelayTime = 0.f,
					const bool bRandomlyGetModal = false);

		void ResetAllStates(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, 
						         const float InStartTime, const float InDuration,
						         float AmplitudeScale, float DecayScale,
								 float FreqScale, float InImpactStrengthScale, float InDampingRatio,
								 const bool bRandomlyGetModal, const float InDelayTime);
		
		virtual ~FModalSynth() = default;
		
		bool Synthesize(FMultichannelBufferView& OutAudio, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
						bool bAddToOutput = false, bool bClampOutput = true);

		static void SynthesizeFullOneChannel(FModalSynth& ModalSynth, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, TArray<float>& SynthData);
		
		float GetCurrentTIme() const { return CurrentTime; }
		float GetDuration() const { return CurrentTime - StartTime; }
		bool IsFinished() const { return CurrentState == ESynthesizerState::Finished; }
		bool IsRunning() const;
		void ForceStop();
		
		float GetCurrentMaxAmplitude() const;
		
	private:
		void InitBuffers(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const int32 NumUsedModals);

		float SamplingRate;

		int32 NumUsedParams;
		int32 NumTrueModal;
		float StartTime;
		float MaxDuration;
		float CurrentTime;
		float TimeResolution;
		
		ESynthesizerState CurrentState;
		
		FAlignedFloatBuffer RealBuffer;
		FAlignedFloatBuffer ImgBuffer;
		FAlignedFloatBuffer QBuffer;
		FAlignedFloatBuffer PBuffer;

		float CurrentAmpScale;
		float CurrentDecayScale;
		float CurrentFreqScale;
		
		float TotalMaxAmplitude;
		float CurrentTotalAmplitude;
		float DelayTime;
		float FrameTime;
	};
}
