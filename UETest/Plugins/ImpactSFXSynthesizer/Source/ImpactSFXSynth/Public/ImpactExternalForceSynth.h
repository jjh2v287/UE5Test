// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactModalObj.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;
	
	class IMPACTSFXSYNTH_API FImpactExternalForceSynth
	{
	public:
		FImpactExternalForceSynth(const FImpactModalObjAssetProxyPtr& ModalsParams, const float InSamplingRate, const int32 InNumOutChannel,
								  const int32 NumUsedModals = -1,
				                  const float AmplitudeScale = 1.f, const float DecayScale = 1.f, const float FreqScale = 1.f);

		virtual bool Synthesize(FMultichannelBufferView& OutAudio, const TArray<TArrayView<const float>>& InForces,
								const FImpactModalObjAssetProxyPtr& ModalsParams,
								const float AmplitudeScale = 1.f, const float DecayScale = 1.f, const float FreqScale = 1.f,
								const bool bIsForceStop = true, bool bClampOutput = true);

		virtual ~FImpactExternalForceSynth() = default;
		
	protected:
		void InitBuffers(const TArrayView<const float>& ModalsParams, const int32 NumUsedModals,
						const float AmplitudeScale, const float DecayScale, const float FreqScale);

		void InitPreCalBuffers(const TArrayView<const float>& ModalsParams, float AmplitudeScale, float DecayScale, float FreqScale);

		virtual void ReInitBuffersIfNeeded(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const float AmplitudeScale, const float DecayScale, const float FreqScale);
		
		void GetNumUsedModals(const bool bIsForceStop);

		virtual void StartSynthesizing(FMultichannelBufferView& OutAudio, const TArray<TArrayView<const float>>& InForces,
									   const int32 NumOutputFrames);
		
	protected:
		float TimeStep;
		float FrameTime;
		int32 NumUsedParams;
		int32 CurrentNumModals;
		
		float CurAmpScale;
		float CurDecayScale;
		float CurFreqScale;
		
		FAlignedFloatBuffer TwoDecayCosBuffer;
		FAlignedFloatBuffer DecaySqrBuffer;
		FAlignedFloatBuffer ForceGainBuffer;
		FAlignedFloatBuffer OutL1Buffer;
		FAlignedFloatBuffer OutL2Buffer;
		FAlignedFloatBuffer OutR1Buffer;
		FAlignedFloatBuffer OutR2Buffer;
	
	private:
		float SamplingRate;
		int32 NumOutChannel;
	};
}