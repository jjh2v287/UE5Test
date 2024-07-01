// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "DSP/MultichannelBuffer.h"
#include "Math/RandomStream.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	struct FCombustionEngineForceParams
	{
		float RPM = 600.f;

		float RPMFluctuation = 100.f;
		
		float CombustionCyclePercent = 0.2f;
		float CombustionStrengthMin = 0.2f;
		float CombustionStrengthRange = 0.05f;		
		
		float ExhaustStrengthMin = 0.2f;
		float ExhaustStrengthRange = 0.05f;
		
		float NoiseStrength = 0.1f;
		
		FCombustionEngineForceParams() { }

		FCombustionEngineForceParams(const float InRPM, 
							const float InRPMFluctuation, const float InCombustionCyclePercent,
							const float InCombustionStrengthMin, const float InCombustionStrengthRange,
							const float InExhaustStrengthMin, const float InExhaustStrengthRange,
							const float InCombustionNoiseStrength)
		: RPM(InRPM), RPMFluctuation(InRPMFluctuation), 
		  CombustionStrengthMin(InCombustionStrengthMin), CombustionStrengthRange(InCombustionStrengthRange),
		  ExhaustStrengthMin(InExhaustStrengthMin), ExhaustStrengthRange(InExhaustStrengthRange),
		  NoiseStrength(InCombustionNoiseStrength)
		{
			CombustionCyclePercent =  FMath::Clamp(InCombustionCyclePercent, 0.01f, 0.99f);
		}
	};
	
	class IMPACTSFXSYNTH_API FCombustionEngineForceGen
	{
	public:
		FCombustionEngineForceGen(const float InSamplingRate, const int32 InNumPulsePerRevolution, const int32 InSeed = -1);

		void Generate(FMultichannelBufferView& OutAudio, const FCombustionEngineForceParams& Params);

	protected:
		void UpdateCombustionTime(const FCombustionEngineForceParams& Params);
		
		float GenerateForceMono(const FCombustionEngineForceParams& Params);
		
	private:
		float SamplingRate;
		
		int32 Seed;
		FRandomStream RandomStream;

		int32 NumPulsePerRevolution;
		float TimeStep;
		float DeltaCombustionTime;
		float LastCombustionTime;
		float HalfCombustionDuration;
		
		float CombustionStrength;
		float ExhaustStrength;
		float ExhaustDecay;
	};
}