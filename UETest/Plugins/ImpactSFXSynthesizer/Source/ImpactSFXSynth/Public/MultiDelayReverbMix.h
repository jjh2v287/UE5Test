// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CirBufferCustom.h"
#include "DSP/Delay.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	class FMultiDelayReverbMix
	{
	public:
		static constexpr int32 NumChannels = 4;
		
		FMultiDelayReverbMix(float InSamplingRate, float InGain, float InMinDelay, float InMaxDelay);
		void ProcessAudio(TArrayView<float>&  OutAudio, const TArrayView<const float>& InAudio, const float FeedbackGain);
		void ResetBuffers();

	private:
		FORCEINLINE void SetDecayGain(float InGain);
		FORCEINLINE void ProcessOneSample(TArrayView<float>& OutAudio, int i, const float InSample);
		
		float SamplingRate;
		float DecayGain;

		TArray<int32> ChannelDelaySample;
		TArray<int32> ChannelWriteIndex;
		TArray<FAlignedFloatBuffer> DelayBuffers;
	};


}
