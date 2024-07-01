// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactModalObj.h"
#include "ModalFFT.h"
#include "ResidualData.h"
#include "ResidualSynth.h"
#include "SynthParamPresets.h"
#include "DSP/FFTAlgorithm.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	struct FScratchImpactSpawnParams
	{
		float SpawnRate;
		float SpawnChance;
		float SpawnChanceDecayRate;
		float SpawnDuration;		
		float GainMin;
		float GainRange;
		float GainDecayRate;
		int32 FrameStartMin;
		int32 FrameStartMax;
		float ResidualPitchScale;
		
		FScratchImpactSpawnParams(const float InSpawnRate, const float InSpawnChance, const float InSpawnChanceDecayRate,
								  const float InSpawnDuration, const float InGainMin,
								  const float InGainRange, const float InGainDecayRate,
								  const int32 InFrameStartMin, const int32 InFrameStartMax, const float InResidualPitchScale)
		: SpawnRate(InSpawnRate), SpawnChance(InSpawnChance), SpawnChanceDecayRate(InSpawnChanceDecayRate),
		SpawnDuration(InSpawnDuration), GainMin(InGainMin), GainRange(InGainRange),
		GainDecayRate(InGainDecayRate), FrameStartMin(InFrameStartMin), FrameStartMax(InFrameStartMax), ResidualPitchScale(InResidualPitchScale)
		{
			
		}
	};

	struct IMPACTSFXSYNTH_API FScratchImpactInfo
	{
		int32 Frame = 0;
		
		float AmpScale = 1.0f;
		FModalMods ExciteModalMods = FModalMods();
		bool bExciteModalEnable = true;
		
		FModalMods ResonModalMods = FModalMods();
		bool bResonModalEnable = true;

		bool bResidualEnable = true;
		float ResidualPitchScale = 1.f;
	};

	class IMPACTSFXSYNTH_API FScratchingSynth
	{
	public:
		static constexpr int32 MaxNumImpacts = 8;
		
		using FResidualDataAssetProxyRef = TSharedRef<FResidualDataAssetProxy, ESPMode::ThreadSafe>;
		
		FScratchingSynth(const float InSamplingRate, const int32 InNumFramesPerBlock, const int32 InNumOutChannel,
					   const int32 InSeed, const FResidualDataAssetProxyPtr& ResidualDataProxyPtr,
					   const float ResidualPlaySpeed, const float ResidualAmplitudeScale=1.0f, const float ResidualPitchScale = 1.0f,
					   const float ResidualStartTime = 0.f, const float ResidualDuration=-1.f);

		bool Synthesize(const FScratchImpactSpawnParams& SpawnParams, const FImpactModalObjAssetProxyPtr& ExciteModalsParams, const FModalMods& ExciteModalMods,
						const FImpactModalObjAssetProxyPtr& ResonModalsParams, const FModalMods& ResonModalMods,
						FMultichannelBufferView& OutAudio, bool bClampOutput);

		int32 GetSeed() const { return Seed; };

	protected:
		void IniFFTBuffers();
		
		bool SynthesizeOneFrame(const FScratchImpactSpawnParams& SpawnParams, const FImpactModalObjAssetProxyPtr& ExciteModalsParams, const FModalMods& ExciteModalMods,
								const FImpactModalObjAssetProxyPtr& ResonModalsParams, const FModalMods& ResonModalMods);
		bool SpawnImpactIfNeeded(const FScratchImpactSpawnParams& SpawnParams, const FImpactModalObjAssetProxyPtr& ExciteModalsParams, const FModalMods& ExciteModalMods,
								 const FImpactModalObjAssetProxyPtr& ResonModalsParams, const FModalMods& ResonModalMods);
		void SynthesizeCurrentImpacts(const FImpactModalObjAssetProxyPtr& ExciteModalsParams, const FImpactModalObjAssetProxyPtr& ResonModalsParams);
		void DoIFFT();

	private:
		float SamplingRate;
		int32 NumOutChannel;
		
		FRandomStream ImpactRandomStream;

		TUniquePtr<FResidualSynth> ResidualSynth;
		int32 NumFFT;
		int32 HopSize;
		float TimeStep;
		int32 Seed;
		
		TUniquePtr<IFFTAlgorithm> FFT;
		TArray<FAlignedFloatBuffer> ConjBuffers;
		FAlignedFloatBuffer ComplexSpectrum;
		TArray<FAlignedFloatBuffer> OutReals;
		FAlignedFloatBuffer WindowBuffer;
		
		Audio::FAlignedFloatBuffer MagBuffer;
		
		//Buffer to current synthesized data and to be copy to output when requested 
		TArray<Audio::FAlignedFloatBuffer> SynthesizedDataBuffers;
		int32 CurrentSynthBufferIndex;
		int32 CurrentFrame;
		
		float LastImpactSpawnTime;
		bool bIsFinalFrameSynth;
		
		TArray<FScratchImpactInfo> ImpactInfos;
	};
}

