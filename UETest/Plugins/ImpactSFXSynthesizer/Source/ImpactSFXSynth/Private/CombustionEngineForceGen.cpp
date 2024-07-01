// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "CombustionEngineForceGen.h"

#include "ExtendArrayMath.h"
#include "ImpactSFXSynthLog.h"
#include "ModalSynth.h"

namespace LBSImpactSFXSynth
{
	FCombustionEngineForceGen::FCombustionEngineForceGen(const float InSamplingRate, const int32 InNumPulsePerRevolution, const int32 InSeed)
		: SamplingRate(InSamplingRate), DeltaCombustionTime(0.f),
		  LastCombustionTime(1e6f), HalfCombustionDuration(0.f),
	      CombustionStrength(0.f), ExhaustStrength(0.f), ExhaustDecay(0.f)
	{
		Seed = InSeed > -1 ? InSeed : FMath::Rand();
		RandomStream = FRandomStream(Seed);
		TimeStep = 1.f / SamplingRate;
		NumPulsePerRevolution = FMath::Max(1, InNumPulsePerRevolution);
	}
	
	void FCombustionEngineForceGen::Generate(FMultichannelBufferView& OutAudio, const FCombustionEngineForceParams& Params)
	{
		const int32 NumOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		if(NumOutputFrames <= 0 || Params.RPM < 1e-5f)
			return;
		
		const TArrayView<float> ForceBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
		
		for(int i = 0; i < NumOutputFrames; i++)
		{
			UpdateCombustionTime(Params);
			ForceBuffer[i] = GenerateForceMono(Params);
		}
	}

	void FCombustionEngineForceGen::UpdateCombustionTime(const FCombustionEngineForceParams& Params)
	{
		const float CurrentRPM = FMath::Max(1.f, Params.RPM + (FMath::FRand() - 0.5) * Params.RPMFluctuation);
		const float PulseTimeDistance = 60.f / (CurrentRPM * NumPulsePerRevolution);
		if(LastCombustionTime > PulseTimeDistance)
		{
			LastCombustionTime = 0.f;
			const float CombustionDuration = Params.CombustionCyclePercent * PulseTimeDistance;
			HalfCombustionDuration = CombustionDuration / 2.0f;
			DeltaCombustionTime = - HalfCombustionDuration;
			CombustionStrength = Params.CombustionStrengthMin + RandomStream.FRand() * Params.CombustionStrengthRange;
			ExhaustStrength = Params.ExhaustStrengthMin + RandomStream.FRand() * Params.ExhaustStrengthRange;
			ExhaustDecay = 1.f / (FMath::Sqrt((PulseTimeDistance - CombustionDuration)));
		}
		else
			LastCombustionTime += TimeStep;
	}
	
	float FCombustionEngineForceGen::GenerateForceMono(const FCombustionEngineForceParams& Params)
	{
		if(DeltaCombustionTime < HalfCombustionDuration)
		{
			const float Slope = FMath::Sin(UE_PI * DeltaCombustionTime / HalfCombustionDuration);
			const float Force = CombustionStrength * Slope + Params.NoiseStrength * (RandomStream.FRand() - 0.5f);;
			DeltaCombustionTime += TimeStep;
			return Force;
		}

		const float Slope = 1.0f - ExhaustDecay * FMath::Sqrt(DeltaCombustionTime - HalfCombustionDuration);
		if(Slope >= 0)
		{
			const float Force = ExhaustStrength * Slope;
			DeltaCombustionTime += TimeStep;
			return Force + Force * Params.NoiseStrength * (RandomStream.FRand() - 0.5f);
		}

		return 0.f;
	}
	
}
