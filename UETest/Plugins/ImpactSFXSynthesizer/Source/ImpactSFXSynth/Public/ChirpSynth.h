// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	struct FChirpSynthParams
	{
		float RampDuration;
		float ChirpRate;
		float AmpScale;
		float DecayScale;
		float PitchScale;
		float RandomRate;
		float AmplitudeScaleRandomRange;
		float PitchShiftRandomRange;

		FChirpSynthParams() : FChirpSynthParams(1.f, 0.f, 1.f,
												1.f, 1.f,
												0.f, 0.f, 0.f) {};
		
		FChirpSynthParams(const float InRampDuration, const float InChirpRate, const float InAmpScale,
						  const float InDecayScale, const float InPitchScale, const float InRandomRate,
						  const float AmpRandRange, const float PitchShiftRandRange)
		: RampDuration(InRampDuration)
		, ChirpRate(InChirpRate)
		, AmpScale(InAmpScale)
		, RandomRate(InRandomRate)
		, AmplitudeScaleRandomRange(AmpRandRange)
		, PitchShiftRandomRange(PitchShiftRandRange)
		{
			DecayScale = FMath::Max(0.f, InDecayScale);
			PitchScale = FMath::Max(1e-3f, InPitchScale);
		}
	};
	
	class IMPACTSFXSYNTH_API FChirpSynth
	{
		
	public:
		static constexpr float FreqAmpThreshold = 20.0f;
		static constexpr float FreqRandChance = 0.5f;
		static constexpr float FreqRandThreshold = 1000.f;
		
		FChirpSynth(float InSamplingRate, int32 InNumFramesPerBlock, TArrayView<const float> ModalsParams,
					const FChirpSynthParams& Params, const int32 NumUsedModals = -1, const float InMaxDuration = -1.f);
		
		virtual ~FChirpSynth() = default;

		virtual bool Synthesize(FMultichannelBufferView& OutAudio, TArrayView<const float> ModalsParams,
								const FChirpSynthParams& Params, const bool bClampOutput = true);

		float GetDuration() const { return Duration; }

	protected:
		virtual bool StartSynthesize(const TArrayView<float>& OutAudio, const FChirpSynthParams& Params);

		virtual void UpdateChirpRateIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params);
		virtual void UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params);
		virtual void ResetTimeBuffer();
		virtual void UpdateCurrentChirpParams(const FChirpSynthParams& Params);
		virtual float GetRampDurationClamp(const float InValue);

		virtual void UpdateRandBufferIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params, int32 NumModals);
		virtual float GetCurrentRandAlpha() const;
		
		virtual bool IsNoChirpChange(const FChirpSynthParams& Params);

		FORCEINLINE float GetCurrentEffectiveFreq(const float F0, const float ChirpRate, const float ChirpDuration);
		
		int32 InitBuffers(TArrayView<const float> ModalsParams, const int32 NumUsedModals, const FChirpSynthParams& Params);

		bool IsRandomAmplitude(const FChirpSynthParams& Params);
		bool IsReachRamDuration();

		float SamplingRate;
		int32 NumFramesPerBlock;
		
		float Duration;
		float TimeResolution;
		float RandAccumTime;
		float LastRandTime;
		float CurrentRandRate;
		float MaxDuration;
		float CurrentChirpRate;
		float CurrentPitchScale;
		float CurrentMaxRampDuration;
		bool bNotAllSameDecayParam;
		Audio::FAlignedFloatBuffer TimeBuffer;
		Audio::FAlignedFloatBuffer ModalParamsBuffer;

		Audio::FAlignedFloatBuffer LastRandAmpBuffer;
		Audio::FAlignedFloatBuffer CurrentRandAmpBuffer;
	};

	class IMPACTSFXSYNTH_API FSigmoidChirpSynth : public FChirpSynth
	{
	public:
		static constexpr float TargetFrequencyTolerance = 1e-3f;
		
		FSigmoidChirpSynth(float InSamplingRate, int32 InNumFramesPerBlock, TArrayView<const float> ModalsParams,
					  const FChirpSynthParams& Params, const int32 NumUsedModals = -1, const float InMaxDuration = -1.f);

	protected:
		virtual bool IsNearlyReachTargetFrequency(float F0, float FTarget, float ExpFactor);
		virtual bool StartSynthesize(const TArrayView<float>& OutAudio, const FChirpSynthParams& Params) override;
		
		virtual float CalculateEffectiveFrequency(float F0, float FreqRange, float ExpFactor);
		virtual void ArrayChirpSynthesize(const TArrayView<float>& OutAudio, float ChirpRate, float Amp, float Decay, float F0,
										 float FreqRange, const float Bias);
		
		virtual void UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params) override;
		virtual void UpdateRandBufferIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthParams& Params, int32 NumModals) override;

		virtual float CalculateChirpRate(const float ChirpRate, const float RampDuration);
		virtual float GetRampDurationClamp(const float InValue) override;
		
		void InitRampBuffers(const FChirpSynthParams& Params);
		
		bool bIsAllFrequencyReachTarget;
		float LastChangeFreqTime;
		Audio::FAlignedFloatBuffer TargetFreqBuffer;
		Audio::FAlignedFloatBuffer CurrentFreqBuffer;
	};

	class IMPACTSFXSYNTH_API FExponentChirpSynth : public FSigmoidChirpSynth
	{
	public:
		static constexpr float TargetFrequencyTolerance = 1e-3f;
		
		FExponentChirpSynth(float InSamplingRate, int32 InNumFramesPerBlock, TArrayView<const float> ModalsParams,
					  const FChirpSynthParams& Params, const int32 NumUsedModals = -1, const float InMaxDuration = -1.f);
		

	protected:
		virtual bool IsNearlyReachTargetFrequency(float F0, float FTarget, float ExpFactor) override;
		virtual float CalculateEffectiveFrequency(float F0, float FreqRange, float ExpFactor) override;
		virtual void ArrayChirpSynthesize(const TArrayView<float>& OutAudio, float ChirpRate, float Amp, float Decay, float F0, float FreqRange, const float Bias) override;
		virtual float CalculateChirpRate(const float ChirpRate, const float RampDuration) override;
	};
}
