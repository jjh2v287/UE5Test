// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundEnumDefinitions.h"
#include "DSP/AlignedBuffer.h"

namespace LBSImpactSFXSynth
{
	enum class ESynthesizerState
	{
		Init = 0,
		Running,
		Finished,
		Unknown
	};
	
	class IMPACTSFXSYNTH_API FDrumModalPresets
	{
	public:
		// Statically-defined scale/chord tone definitions.
		static TMap<Metasound::EDrumModalType, Audio::FAlignedFloatBuffer> PresetMap;
	};
	
	class IMPACTSFXSYNTH_API FResidualStats
	{
	public:
		static constexpr int32 NumSinPoint = 1024; // must match SinCycle.Num()
		static constexpr int32 MaxSinIdx = 1023;
		static constexpr int32 CosShift = 256;
		static constexpr float SinStep = UE_TWO_PI / 1024.f;
		static Audio::FAlignedFloatBuffer SinCycle;
	};
}
