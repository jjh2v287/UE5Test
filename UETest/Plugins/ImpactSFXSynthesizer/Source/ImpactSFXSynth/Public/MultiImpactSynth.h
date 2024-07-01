// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ModalSynth.h"
#include "ResidualSynth.h"
#include "MultiImpactData.h"
#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	enum class EMultiImpactVariationSpawnType
	{
		Random = 0,
		Increment,
		Decrement,
		PingPong
	};
	
	struct FMultiImpactSpawnParams
	{
		int32 ImpactSpawnBurst;
		float ImpactSpawnRate;
		int32 ImpactSpawnCount;
		float ImpactSpawnChance;
		float ImpactChanceDecayRate;
		float ImpactSpawnDuration;
		float ImpactSpawnDelay;
		float ImpactStrengthMin;
		float ImpactStrengthRange;
		float ImpactStrengthDecayRate;

		FMultiImpactSpawnParams(const int32 InImpactSpawnBurst, const float InImpactSpawnRate, const int32 InImpactSpawnCount, const float InImpactSpawnChance, const float InImpactChanceDecayRate,
								const float InImpactSpawnDuration, const float InImpactSpawnDelay, const float InImpactStrengthMin, const float InImpactStrengthRange, const float InImpactStrengthDecayRate)
		: ImpactSpawnBurst(InImpactSpawnBurst), ImpactSpawnRate(InImpactSpawnRate), ImpactSpawnCount(InImpactSpawnCount), ImpactSpawnChance(InImpactSpawnChance), ImpactChanceDecayRate(InImpactChanceDecayRate)
		, ImpactSpawnDuration(InImpactSpawnDuration), ImpactSpawnDelay(InImpactSpawnDelay), ImpactStrengthMin(InImpactStrengthMin), ImpactStrengthRange(InImpactStrengthRange), ImpactStrengthDecayRate(InImpactStrengthDecayRate)
		{ }
	};
	
	class IMPACTSFXSYNTH_API FMultiImpactSynth
	{
	
	public:
		using FMultiImpactDataAssetProxyRef = TSharedRef<FMultiImpactDataAssetProxy, ESPMode::ThreadSafe>;
		
		FMultiImpactSynth(FMultiImpactDataAssetProxyRef InDataAssetProxyRef, const int32 InMaxNumImpacts,
						  const int32 InSamplingRate, const int32 InNumFramesPerBlock, const int32 InNumChannels,
						  const bool InIsStopSpawningOnMax = false, const int32 InSeed = -1,
						  EMultiImpactVariationSpawnType InSpawnType = EMultiImpactVariationSpawnType::Random,
						  int32 Slack = 10);		
		
		virtual ~FMultiImpactSynth() = default;
		
		bool Synthesize(FMultichannelBufferView& OutAudioView, const FMultiImpactSpawnParams& SpawnParams,
						const float InGlobalDecayScale, const float InGlobalResidualSpeedScale,
						const float InGlobalModalPitchShift, const float InGlobalResidualPitchShift,
						const bool bClamp = true);

		void Restart();
		
		void StopAllSynthesizers();
		
		int32 GetSeed() const { return Seed; }
		bool CanRun() const { return bHasModalSynth || bHasResidualSynth; }
		
	private:
		FMultiImpactDataAssetProxyRef MultiImpactProxy;
		int32 MaxNumImpacts;
		float SamplingRate;
		int32 NumFramesPerBlock;
		int32 NumChannels;
		bool bIsStopSpawningOnMax;
		
		TArray<int32> VariationIndexes;
		TArray<TSharedPtr<FModalSynth>> ModalSynths;
		TArray<TSharedPtr<FResidualSynth>> ResidualSynths;
		
		bool bHasModalSynth;
		bool bHasResidualSynth;
		int32 NumUsedModals;
		
		int32 Seed;
		FRandomStream RandomStream;
		float NextFrameTime;
		float AccumNoSpawnTime;
		int32 NumActiveModalSynths;
		int32 NumActiveResidualSynths;
		float CurrentSpawnChance;
		EMultiImpactVariationSpawnType VariationSpawnType;
		int32 LastVariationIndex;
		bool bIsRevert;

		float GlobalDecayScale;
		float GlobalResidualSpeedScale;

		float GlobalModalPitchShift;
		float GlobalResidualPitchShift;
		
		bool SpawningNewImpacts(const int32 NumFramesToGenerate, const FMultiImpactSpawnParams& SpawnParams);
		void StopWeakestSynths(int32 NumImpactToSpawn);

		const FImpactSpawnInfo* GeVariationSpawnInfo(TArrayView<const FImpactSpawnInfo> SpawnInfos);
		
		float GetImpactStrengthScaleClamped(const float InValue) const;		
	};
}
