// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ResidualData.h"
#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/FFTAlgorithm.h"
#include "DSP/MultichannelBuffer.h"
#include "SynthParamPresets.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;
	
	class IMPACTSFXSYNTH_API FResidualSynth
	{
	public:
		using FResidualDataAssetProxyRef = TSharedRef<FResidualDataAssetProxy, ESPMode::ThreadSafe>;
		static constexpr int32 MinNumAnalyzedErbFrames = 2; // First warmup always needs to sync 2 frame
		
		FResidualSynth(const float InSamplingRate, const int32 InNumFramesPerBlock, const int32 InNumOutChannel,
		               FResidualDataAssetProxyRef ResidualDataAssetProxyRef,
		               const float InPlaySpeed, const float InAmplitudeScale, const float InPitchScale = 1.0f, const int32 Seed = -1,
		               const float InStartTime = 0.f, const float Duration = -1.f, const bool InIsLoop = false,
		               const float InImpactStrengthScale = 1.0f, const float InRandomLoop = 0.f, const float InDelayTime = 0.f,
		               const float InRandomMagnitudeScale = 0.f);

		void ChangeScalingParams(const float InStartTime, const float InDuration, const float InPlaySpeed,
								const float InAmplitudeScale, const float InPitchScale, const float InImpactStrengthScale);

		void ChangePlaySpeed(const float InPlaySpeed, const float InStartTime, const float Duration, const float InImpactStrengthScale, const UResidualObj* ResidualObj);
		
		void Restart(const float InDelayTime = 0.f);
		
		virtual ~FResidualSynth() = default;

		/**
		 * @brief Synthesize the number of samples requested based on current internal state. 
		 * @param OutAudio Mono or Stereo channels. The number of synthesized samples is based on the first channel size.
		 * @param bAddToOutput True = Add synthesized to current buffer. False = replace current data with synthesized data. 
		 * @param bClampOutput Clamp output to -1 -> 1 range or not.
		 * @return True when finish synthesized all internal frames.
		 */
		bool Synthesize(FMultichannelBufferView& OutAudio, bool bAddToOutput=false, bool bClampOutput=true);

		static void SynthesizeFull(FResidualSynth& ResidualSynth, FAlignedFloatBuffer& SynthesizedData);

		float GetCurrentTIme() const;

		static void GenerateHannWindow(float* WindowBuffer, int32 NumFrames, bool bIsPeriodic);

		float GetPlaySpeed() const { return PlaySpeed; }
		TArrayView<const float> GetCurrentMagFreq() { return TArrayView<const float>(ErbFrameBuffer); }
		float GetFreqResolution() const { return FreqResolution; }

		float GetMaxDuration() const { return EndTime - StartTime; }
		float GetSamplingRate() const { return SamplingRate; }
		
		float GetCurrentFrameEnergy();
		
		bool IsFinished() const;
		bool IsRunning() const;
		void ForceStop();

		void SetCurrentErbSynthFrame(const float InValue);
		void SetCurrentTime(const float InTime);

		bool GetFFTMagAtFrame(const int32 NthFrame, TArrayView<float> MagBuffer, const float AmpScale, const float InPitchScale);
		void DoIFFT(TArrayView<const float> MagBuffer);

		bool IsInFrameRange() const;

		TArrayView<FAlignedFloatBuffer> GetOutRealBuffer() { return TArrayView<FAlignedFloatBuffer>(OutReals); }
		TArrayView<FAlignedFloatBuffer> GetSynthesizedDataBuffers() { return TArrayView<FAlignedFloatBuffer>(SynthesizedDataBuffers); }

	protected:
		void IniBuffers(const UResidualObj* ResidualObj);
		void InitInterpolateBuffers(const UResidualObj* ResidualObj);
		void SynthesizeOneFrame(const UResidualObj* ResidualObj);

		void GetErbDataByInterpolatingFrames(const UResidualObj* ResidualObj);
		void PutFrameDataToBuffers();
		
		void CalNewPhaseIndex();
		void SetConjBuffer();
		
		void PutFrameDataToMagBuffer();
		
	private:
		float SamplingRate;
		int32 NumFramesPerBlock;
		int32 NumOutChannel;
		FResidualDataAssetProxyRef ResidualDataProxy;
		float AmplitudeScale;

		TUniquePtr<IFFTAlgorithm> FFT;
		TArray<FAlignedFloatBuffer> ConjBuffers;
		FAlignedFloatBuffer ComplexSpectrum;
		TArray<FAlignedFloatBuffer> OutReals;
		FAlignedFloatBuffer WindowBuffer;
		
		float CurrentErbSynthFrame;
		float StartErbSynthFrame;
		int32 EndErbSynthFrame;
		float PlaySpeed;
		TArray<int32> FFTInterpolateIdxs;
		TArray<float> Freqs;
		TArray<int32> PhaseIndexes;
		Audio::FAlignedFloatBuffer ErbFrameBuffer;
		Audio::FAlignedFloatBuffer ErbPitchScaleBuffer;
		Audio::FAlignedFloatBuffer ErbInterpolateBuffer;
		Audio::FAlignedFloatBuffer ErbInterpolateBufferCeil;
		Audio::FAlignedFloatBuffer FFTInterpolateBuffer;
		
		//Buffer to current synthesized data and to be copy to output when requested 
		TArray<Audio::FAlignedFloatBuffer> SynthesizedDataBuffers;
		int32 CurrentSynthBufferIndex;

		bool bIsFinalFrameSynth;
		float FreqResolution;
		float TimeResolution;
		float StartTime;
		float EndTime;
		float CurrentTime;
		float PitchScale;
		FRandomStream RandomStream;
		bool bIsRandWithSeed;
		
		bool bIsLooping;
		float RandomLoop;
		float DelayTime;
		float RandomMagnitudeScale;
		ESynthesizerState CurrentState;

		//Used to track and compare energy in multi impacts
		float LastEnergyFrame;
		float CurrentFrameEnergy;
		
		bool IsPitchScale() const;
	};
}
