// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactModalObj.h"
#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;

	struct FChirpSynthEulerParams
	{
		float RampDuration;
		float ChirpRate;
		float AmpScale;
		float DecayScale;
		float PitchScale;
		float RandomRate;
		float AmplitudeScaleRandomRange;
		float PitchShiftRandomRange;

		FChirpSynthEulerParams() : FChirpSynthEulerParams(1.f, 0.f, 1.f,
												1.f, 1.f,
												0.f, 0.f, 0.f) {};
		
		FChirpSynthEulerParams(const float InRampDuration, const float InChirpRate, const float InAmpScale,
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
	
	class IMPACTSFXSYNTH_API FChirpSynthEuler
	{
		
	public:
		static constexpr float FreqAmpThreshold = 20.0f;
		static constexpr float FreqRandChance = 0.5f;
		static constexpr float FreqRandThreshold = 1000.f;
		
		FChirpSynthEuler(float InSamplingRate, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
					     const FChirpSynthEulerParams& Params, const int32 NumUsedModals = -1, const float InMaxDuration = -1.f);

		virtual ~FChirpSynthEuler() = default;

		virtual bool Synthesize(FMultichannelBufferView& OutAudio, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
								const FChirpSynthEulerParams& Params, const bool bClampOutput = true);

		float GetDuration() const { return Duration; }

	protected:
		float GetCurrentChirpDuration();
		
		virtual void UpdateChirpParamsIfNeeded(TArrayView<const float> ModalsParams, const FChirpSynthEulerParams& Params);
		virtual bool UpdateBuffers(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const TArrayView<const float>& ModalData, const FChirpSynthEulerParams& Params, const float FrameTime);
		virtual void UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthEulerParams& Params);

		virtual void UpdateCurrentChirpParams(const FChirpSynthEulerParams& Params);
		virtual float GetRampDurationClamp(const float InValue);

		virtual float GetChirpPhiSampling(const int32 ModalIdx, const FChirpSynthEulerParams& Params, const float ChirpDuration);
		
		virtual bool IsNoChirpChange(const FChirpSynthEulerParams& Params);

		FORCEINLINE float GetCurrentEffectiveFreq(const float F0, const float ChirpRate, const float ChirpDuration);
		
		bool IsRandomAmplitude(const FChirpSynthEulerParams& Params);

		float SamplingRate;
		
		float Duration;
		float TimeResolution;

		float MaxDuration;
		float CurrentChirpRate;
		float CurrentPitchScale;
		float CurrentMaxRampDuration;
		
		int32 NumUsedParams;
		int32 NumTrueModal;
		float LastRandTime;

		FAlignedFloatBuffer RealBuffer;
		FAlignedFloatBuffer ImgBuffer;
		FAlignedFloatBuffer QBuffer;
		FAlignedFloatBuffer PBuffer;
		FAlignedFloatBuffer FreqBuffer;
		FAlignedFloatBuffer DecayBuffer;

		TArray<int32> UpwardGainModalIdx;
		TArray<float> UpwardGainModalTime;

		float LastTimeUpdateChirpParams;
	};

	class IMPACTSFXSYNTH_API FSigmoidChirpSynthEuler : public FChirpSynthEuler
	{
	public:
		FSigmoidChirpSynthEuler(float InSamplingRate, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr,
					  const FChirpSynthEulerParams& Params, const int32 NumUsedModals = -1, const float InMaxDuration = -1.f);

	protected:
		virtual bool Synthesize(FMultichannelBufferView& OutAudio, const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const FChirpSynthEulerParams& Params, const bool bClampOutput) override;
		
		virtual float GetChirpPhiSampling(const int32 ModalIdx, const FChirpSynthEulerParams& Params, const float ChirpDuration) override;
		virtual float CalculateEffectiveFrequency(float F0, const float FreqRange);
		
		virtual void UpdateModalBuffers(TArrayView<const float> ModalsParams, const FChirpSynthEulerParams& Params) override;

		virtual float CalculateChirpRate(const float ChirpRate, const float RampDuration);
		virtual float GetRampDurationClamp(const float InValue) override;

		float RealChirpRate;
		float ExpFactor;		
		
		Audio::FAlignedFloatBuffer TargetFreqBuffer;
	};
}
