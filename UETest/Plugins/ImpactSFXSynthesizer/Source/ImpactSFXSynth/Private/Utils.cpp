// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "ImpactSFXSynth/Public/Utils.h"

namespace LBSImpactSFXSynth
{
	bool IsPowerOf2(const int32 Num)
	{
		return  Num != 0 && !(Num & (Num - 1));
	}

	int32 PositiveMod(const int32 Value, const int32 MaxValue)
	{
		return  (MaxValue + (Value % MaxValue)) % MaxValue;
	}

	float GetPitchScale(const float Pitch)
	{
		return FMath::Pow(2.0f, Pitch / 12.0f);
	}

	float GetPitchScaleClamped(const float InPitchShift, const float MinShift, const float MaxPitch)
	{
		const float Pitch = FMath::Clamp(InPitchShift, MinShift, MaxPitch);
		return GetPitchScale(Pitch);
	}

	float GetDampingRatioClamped(const float InRatio)
	{
		return FMath::Clamp(InRatio, 0.f, 1.f);
	}

	float GetRandRange(const FRandomStream& RandomStream, const float InMinValue, const float InRange)
	{
		return InMinValue + InRange * RandomStream.GetFraction();
	}
}
