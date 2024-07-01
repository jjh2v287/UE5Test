// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ResidualAnalyzer.h"

#include "ImpactSFXSynthEditor.h"
#include "ResidualObj.h"
#include "ResidualData.h"
#include "ResidualSynth.h"
#include "DSP/AudioFFT.h"
#include "DSP/FloatArrayMath.h"
#include "Sound/SoundWave.h"

namespace LBSImpactSFXSynth
{
	FResidualAnalyzer::FResidualAnalyzer(const USoundWave* Wave)
	{
		TArray<uint8> ImportedSoundWaveData;
		uint32 ImportedSampleRate;
		uint16 ImportedChannelCount;
		Wave->GetImportedSoundWaveData(ImportedSoundWaveData, ImportedSampleRate, ImportedChannelCount);
		if(ImportedChannelCount > 2 || ImportedChannelCount <= 0)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("FResidualAnalyzer: only mono and stereo audio are supported!"));
			return;
		}
		
		SamplingRate = ImportedSampleRate;
		const int32 TotalSamples = ImportedSoundWaveData.Num() / sizeof(int16);
		
		if (TotalSamples > 0)
		{
			FFFTSettings FFTSettings;
			FFTSettings.Log2Size = Audio::CeilLog2(NumFFTAnalyze);
			FFTSettings.bArrays128BitAligned = true;
			FFTSettings.bEnableHardwareAcceleration = true;
		
			checkf(Audio::FFFTFactory::AreFFTSettingsSupported(FFTSettings), TEXT("No fft algorithm supports fft settings."));
			FFT = FFFTFactory::NewFFTAlgorithm(FFTSettings);
			if(FFT.IsValid())
			{
				GetPaddedWaveMonoData(ImportedSoundWaveData, ImportedChannelCount, TotalSamples);
				InitFFTBuffers();
				InitFreqBuffers();
			}
		}
	}

	void FResidualAnalyzer::InitFFTBuffers()
	{
		InReal.SetNumUninitialized(FFT->NumInputFloats());
		OutComplex.SetNumUninitialized(FFT->NumOutputFloats());
		OutMagnitude.SetNumUninitialized(FFT->NumOutputFloats() / 2);
		WindowBuffer.SetNumUninitialized(FFT->NumInputFloats());
		FResidualSynth::GenerateHannWindow(WindowBuffer.GetData(), FFT->NumInputFloats(), true);
	}

	void FResidualAnalyzer::InitFreqBuffers()
	{
		HopSize = NumFFTAnalyze / 2;
		
		ErbMin = UResidualData::Freq2Erb(FreqMin);
		ErbMax = UResidualData::Freq2Erb(FreqMax);
		TArray<float> ErbFreqs;
		ErbFreqs.Empty(NumErb);
		const float ErbStep = (ErbMax - ErbMin) / (NumErb - 1);
		for(int i = 0; i < NumErb; i++)
			ErbFreqs.Emplace(UResidualData::Erb2Freq(ErbMin + i * ErbStep));

		Freq2ErbIndexes.Empty(NumFreqs);
		NumFreqs = NumFFTAnalyze / 2 + 1;
		FreqResolution = SamplingRate / NumFFTAnalyze;
		const int32 LastBand = NumErb - 1;
		int32 CurrentErbBand = 0;
		for(int i = 0; i < NumFreqs; i++)
		{
			const float CurrentFreq = i * FreqResolution;
			while(CurrentFreq > ErbFreqs[CurrentErbBand] && CurrentErbBand < LastBand)
				CurrentErbBand++;

			if(CurrentErbBand >= LastBand)
				break;
			
			Freq2ErbIndexes.Emplace(CurrentErbBand);
		}
		
		const int32 NumRemain = NumFreqs - Freq2ErbIndexes.Num();
		for(int i = 0; i < NumRemain; i++)
			Freq2ErbIndexes.Emplace(CurrentErbBand);		
	}

	bool FResidualAnalyzer::StartAnalyzing(UResidualObj* OutResidualObj)
	{
		const float Duration = WaveData.Num() / SamplingRate;
		if(Duration > 10.0f)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("FResidualAnalyzer::StartAnalyzing: duration must be lower than 10 seconds!"));
			return false;
		}
		
		if(!FFT.IsValid())
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("FResidualAnalyzer::StartAnalyzing: invalid FFT algorithm!"));
			return false;
		}
		
		const int32 EndIdx = WaveData.Num() - NumFFTAnalyze; //Don't include end padding
		int32 CurrentIdx = 0;
		TArrayView<const float> WaveDataView = TArrayView<float>(WaveData);
		const float* DataBuffer = WaveDataView.GetData();
		float* InData = InReal.GetData();
		float* OutComplexBuff = OutComplex.GetData();
		const int32 NumFrames = EndIdx / NumFFTAnalyze;
		if(NumFrames < UResidualObj::NumMinErbFrames)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("FResidualAnalyzer::StartAnalyzing: wave file is too short!"));
			return false;
		}
		
		OutResidualObj->Data.Empty(NumErb * NumFrames);
		TArray<float> ErbEnv;
		ErbEnv.SetNumUninitialized(NumErb);
		int32 NumFrame = 0;
		while (CurrentIdx < EndIdx)
		{
			FMemory::Memcpy(InData, &DataBuffer[CurrentIdx], NumFFTAnalyze * sizeof(float));
			CurrentIdx += HopSize;
			
			Audio::ArrayMultiplyInPlace(WindowBuffer, InReal);
			
			FFT->ForwardRealToComplex(InData, OutComplexBuff);
			Audio::ArrayComplexToPower(OutComplex, OutMagnitude);
			Audio::ArraySqrtInPlace(OutMagnitude); //Use magnitude instead of power to reduce computational cost when synthesizing
			Audio::ArrayMultiplyByConstantInPlace(OutMagnitude, 1.0 / NumFFTAnalyze);
			
			FMemory::Memzero(ErbEnv.GetData(), ErbEnv.Num() * sizeof(float));
			for(int i = 0; i < NumFreqs; i++)
				ErbEnv[Freq2ErbIndexes[i]] += OutMagnitude[i];

			float TotalMag;
			Audio::ArraySum(ErbEnv, TotalMag);
			if(TotalMag < ErbCutThreshold)
			{
				if(NumFrame == 0) //Still at starting frame then we just skip
					continue;
				
				if(CurrentIdx < EndIdx) //If reaching end frame then we don't trim 
				{
					int32 RemainIdx = CurrentIdx;
					while (RemainIdx < EndIdx && WaveDataView[RemainIdx] < ErbCutThreshold)
						RemainIdx++;

					if(RemainIdx >= EndIdx)
						break;
				}
			}
			
			for(int i = 0; i < NumErb; i++)
				OutResidualObj->Data.Emplace(ErbEnv[i]);
			
			NumFrame++;
		}
		
		OutResidualObj->SetProperties(1, NumFFTAnalyze, HopSize, NumErb, NumFrame, SamplingRate, ErbMax, ErbMin);

		CalibrateSynthMagnitudeToOriginal(OutResidualObj);
		return true;
	}

	void FResidualAnalyzer::GetPaddedWaveMonoData(TArray<uint8> ImportedSoundWaveData, uint16 ImportedChannelCount, int32 TotalSamples)
	{
		const int32 NumSamples = TotalSamples / ImportedChannelCount;
		const int32 Padding = NumFFTAnalyze / 2;
		WaveData.SetNumUninitialized(NumSamples + Padding + NumFFTAnalyze);
		FMemory::Memzero(WaveData.GetData(), WaveData.Num() * sizeof(float));
			
		const int16* InputBuffer = (int16*)ImportedSoundWaveData.GetData();
		for (int32 i = 0, j = Padding; i < TotalSamples; i += ImportedChannelCount, j++) //Only use first channel
			WaveData[j] = static_cast<float>(InputBuffer[i]) / 32768.0f;
	}

	void FResidualAnalyzer::CalibrateSynthMagnitudeToOriginal(UResidualObj* OutResidualObj)
	{
		Audio::ArraySquareInPlace(WaveData);
		float OrgPower;
		Audio::ArraySum(WaveData, OrgPower);

		const auto ResidualDataAssetProxy =  MakeShared<FResidualDataAssetProxy>(OutResidualObj);
		float CurrentAmpScale = 1.f;
		FResidualSynth ResidualSynth = FResidualSynth(SamplingRate, HopSize, 1, ResidualDataAssetProxy,
													  1.f, CurrentAmpScale, 1.f, 0, 0.f, -1.f);
		
		Audio::FAlignedFloatBuffer SynthesizedData;
		float UpperAmpScale = 10.0f;
		float LowerAmpScale = 0.2f;
		int NumLoops = 0;
		float PowerFac = 0.f;
		for(; NumLoops < 50; NumLoops++)
		{
			FResidualSynth::SynthesizeFull(ResidualSynth, SynthesizedData);
			Audio::ArraySquareInPlace(SynthesizedData);
			float SynthPower = 0.f;
			Audio::ArraySum(SynthesizedData, SynthPower);
			
			PowerFac = OrgPower / SynthPower;
			if(FMath::IsNearlyEqual(PowerFac, 1.0f, 0.01f))
				break;

			if(PowerFac < 1.0f)
				UpperAmpScale = CurrentAmpScale;
			else
				LowerAmpScale = CurrentAmpScale;
			CurrentAmpScale = (UpperAmpScale + LowerAmpScale) / 2.0f;
			
			ResidualSynth.ChangeScalingParams(0.f, -1.f, 1.f, CurrentAmpScale, 1.f, 1.f);
			ResidualSynth.Restart();
		}

		Audio::ArrayMultiplyByConstantInPlace(OutResidualObj->Data, CurrentAmpScale);
	}
}
