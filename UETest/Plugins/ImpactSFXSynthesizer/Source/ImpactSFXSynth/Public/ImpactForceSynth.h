// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactModalObj.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	struct FImpactForceSpawnParams
	{
		float SpawnRate;
		float SpawnChance;
		float SpawnChanceDecayRate;
		float SpawnDuration;		
		float StrengthMin;
		float StrengthRange;
		float ImpactDuration;
		float StrengthDurationFactor;
		float SpawnInterval;
		
		FImpactForceSpawnParams()
		{
			SpawnRate = 0.f;
			SpawnInterval = 1e6;
			SpawnChance = 0.f;
			SpawnChanceDecayRate = 0.f;
			SpawnDuration = -1.f;		
			StrengthMin = 0.5f;
			StrengthRange = 0.5f;
			ImpactDuration = 0.01f;
			StrengthDurationFactor = 0.25f;
		}

		FImpactForceSpawnParams(const float InSpawnRate, const float InSpawnChance, const float InSpawnChanceDecayRate,
								const float InSpawnDuration, const float InStrengthMin, const float InStrengthRange, const float InImpactDuration,
								const float InStrengthDurationFactor)
		: SpawnRate(InSpawnRate), SpawnChance(InSpawnChance), SpawnChanceDecayRate(InSpawnChanceDecayRate),
		  SpawnDuration(InSpawnDuration), StrengthMin(InStrengthMin), StrengthRange(InStrengthRange), 
		  StrengthDurationFactor(InStrengthDurationFactor)
		{
			SpawnInterval = SpawnRate > 0.f ? 1.0f / SpawnRate : 1e6;
			ImpactDuration = FMath::Max(1e-3f, InImpactDuration);
		}

		bool IsInSpawnDuration(const float CurrentTime) const
		{
			return SpawnDuration < 0.f || SpawnDuration > CurrentTime;
		}

		bool IsPositiveStrength() const
		{
			return StrengthMin + StrengthRange > 1e-5f;
		}
		
		bool IsCanSpawnNewImpact(const float CurrentTime) const
		{
			return IsPositiveStrength() && SpawnRate > 0.f && SpawnChance > 0.f && IsInSpawnDuration(CurrentTime);
		}
	};
	
	class IMPACTSFXSYNTH_API FImpactForceSynth
	{
	public:
		FImpactForceSynth(const FImpactModalObjAssetProxyPtr& ModalsParams, const float InSamplingRate, const int32 InNumOutChannel,
		                  const FImpactForceSpawnParams& ForceSpawnParams, const int32 NumUsedModals = -1,
		                  const float AmplitudeScale = 1.f, const float DecayScale = 1.f, const float FreqScale = 1.f,
		                  const int32 InSeed = -1, const bool bInForceReset = false);

		virtual ~FImpactForceSynth() = default;
		
		bool Synthesize(FMultichannelBufferView& OutAudio, const FImpactForceSpawnParams& ForceSpawnParams,
						const FImpactModalObjAssetProxyPtr& ModalsParams,
						const float AmplitudeScale = 1.f, const float DecayScale = 1.f, const float FreqScale = 1.f,
		                const bool bIsAutoStop = true, bool bClampOutput = true, bool bIsForceTrigger = false);
		
		int32 GetSeed() const { return Seed; };

	protected:
		void InitBuffers(const TArrayView<const float>& ModalsParams, const int32 NumUsedModals);

		void InitPreCalBuffers(const TArrayView<const float>& ModalsParams, float AmplitudeScale, float DecayScale,
							   float FreqScale);
		
		float GetImpactStrengthRand(const FImpactForceSpawnParams& ForceSpawnParams) const;

		virtual void StartSynthesizing(FMultichannelBufferView& OutAudio, const int32 NumOutputFrames, const int32 NumModals, const FImpactForceSpawnParams& ForceSpawnParams);
		virtual void SpawnNewImpact(const FImpactForceSpawnParams& ForceSpawnParams, bool bForceTrigger);
		virtual void UpdateCurrentForceStrength();

	protected:
		float SamplingRate;
		int32 NumOutChannel;
		
		int32 Seed;
		FRandomStream RandomStream;
		
		float LastForceStrength;
		float LastForceDecayRate;
		float CurrentForceStrength;
		float LastImpactSpawnTime;

		float TimeStep;
		float CurrentTime;
		
		float LastImpactCenterTime;
		float ImpactCenterTime;
		
		float ImpactDecayDuration;
		float ImpactStrength;

		int32 NumUsedParams;
		float AmpImpDurationScale;

		float CurAmpScale;
		float CurDecayScale;
		float CurFreqScale;
		float FrameTime;

		bool bNoForceReset;
		
		FAlignedFloatBuffer TwoDecayCosBuffer;
		FAlignedFloatBuffer DecaySqrBuffer;
		FAlignedFloatBuffer ForceGainBuffer;
		FAlignedFloatBuffer OutL1Buffer;
		FAlignedFloatBuffer OutL2Buffer;
		FAlignedFloatBuffer OutR1Buffer;
		FAlignedFloatBuffer OutR2Buffer;
	};

	class IMPACTSFXSYNTH_API FImpactExponentForceSynth : public FImpactForceSynth
	{
	public:
		FImpactExponentForceSynth(const FImpactModalObjAssetProxyPtr& ModalsParams, const float InSamplingRate, const int32 InNumOutChannel,
						  const FImpactForceSpawnParams& ForceSpawnParams, const int32 NumUsedModals = -1,
						  const float AmplitudeScale = 1.f, const float DecayScale = 1.f, const float FreqScale = 1.f,
						  const int32 InSeed = -1, const bool bInForceReset = false);

	public:
		static float GetDecaySamplingRate(const float Duration, const float SamplingRate, const float MinValue = 1e-3f);
		
	protected:
		virtual void SpawnNewImpact(const FImpactForceSpawnParams& ForceSpawnParams, bool bForceTrigger) override;
		virtual void UpdateCurrentForceStrength() override;

	protected:
		float CurrentForceDecayRate;
	};
}