// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

namespace LBSImpactSFXSynth
{
	IMPACTSFXSYNTH_API bool IsPowerOf2(const int32 Num);
	IMPACTSFXSYNTH_API int32 PositiveMod(const int32 Value, const int32 MaxValue);
	IMPACTSFXSYNTH_API float GetPitchScale(const float Pitch);
	IMPACTSFXSYNTH_API float GetPitchScaleClamped(const float InPitchShift, const float MinShift = -72.f, const float MaxPitch = 72.f);
	IMPACTSFXSYNTH_API float GetDampingRatioClamped(const float InRatio);
	IMPACTSFXSYNTH_API float GetRandRange(const FRandomStream& RandomStream, const float InMinValue, const float InRange);
}
