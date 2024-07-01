// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace LBSImpactSFXSynth
{
	struct FModalMods
	{
		int32 NumModals = -1;
		float AmplitudeScale = 1.f;
		float DecayScale = 1.f;
		float FreqScale = 1.f;
	};
	
	class IMPACTSFXSYNTH_API FModalFFT
	{
		static constexpr int32 NumAdjBin = 3;
		
	public:
		static bool GetFFTMag(TArrayView<const float> ModalsParams, const float SamplingRate, const int32 NumFFT, const int32 HopSize,   
		                      TArrayView<float> FFTMagBuffer, const int32 CurrentFrame, const FModalMods& InMods);
		
		virtual ~FModalFFT() = default;
	};
}
