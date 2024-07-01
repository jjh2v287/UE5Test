// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CirBufferCustom.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;
	
	class IMPACTSFXSYNTH_API FHadamardDiffusion
	{
	public:
		static constexpr int32 NumChannels = 4;
		
		FHadamardDiffusion(float InSamplingRate, int32 InNumStages, float InMaxDelay, bool bDoubleDelayEachStage);
		
		void ProcessAudio(FMultichannelBufferView& OutAudio, const TArrayView<const float>& InAudio);
		
		void ResetBuffers();
		
	private:

		float SamplingRate;
		int32 NumStages;
		float MaxDelay;
		int32 NumBuffers;
		TArray<int32> ChannelWriteIndex;
		TArray<FAlignedFloatBuffer> DelayBuffers;
	};
}