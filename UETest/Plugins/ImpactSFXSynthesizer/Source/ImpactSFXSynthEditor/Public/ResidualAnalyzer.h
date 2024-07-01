// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/FFTAlgorithm.h"

class UResidualObj;
class USoundWave;

namespace LBSImpactSFXSynth
{
	using namespace Audio;
	
	class IMPACTSFXSYNTHEDITOR_API FResidualAnalyzer
	{
	public:
		static constexpr int32 NumFFTAnalyze = 1024;
		static constexpr int32 NumErb = 32;
		static constexpr float FreqMin = 20.0f;
		static constexpr float FreqMax = 20000.0f;
		static constexpr float ErbCutThreshold = 1e-3;
		
		FResidualAnalyzer(const USoundWave* Wave);

		bool StartAnalyzing(UResidualObj* OutResidualObj);
			
	private:
		float SamplingRate;

		TUniquePtr<IFFTAlgorithm> FFT;
		FAlignedFloatBuffer WaveData;
		FAlignedFloatBuffer WindowBuffer;
		FAlignedFloatBuffer OutComplex;
		FAlignedFloatBuffer InReal;
		FAlignedFloatBuffer OutMagnitude;

		int32 HopSize;
		int32 NumFreqs;
		float FreqResolution;
		float ErbMin;
		float ErbMax;
		TArray<int32> Freq2ErbIndexes;
		
		void GetPaddedWaveMonoData(TArray<uint8> ImportedSoundWaveData, uint16 ImportedChannelCount, int32 TotalSamples);
		void InitFFTBuffers();
		void InitFreqBuffers();
		
		void CalibrateSynthMagnitudeToOriginal(UResidualObj* OutResidualObj);
	};
}
