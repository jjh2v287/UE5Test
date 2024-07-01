// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CirBufferCustom.h"
#include "DSP/MultichannelBuffer.h"

namespace LBSImpactSFXSynth
{
	using namespace Audio;
	
	class IMPACTSFXSYNTH_API FHRTFModal
	{
		static constexpr float SoundSpeed = 344.0f;
		static constexpr float MaxITDUnit = 0.0075f;

	private:
		static float GlobalHeadRadius;
		
	public:
		FHRTFModal(float InSamplingRate, int32 InNumFramesPerBlock);

		bool TransferToStereo(const TArrayView<const float>& InAudio, float Azimuth, float Elevation,
							  float Gain, FMultichannelBufferView& OutAudio,
							  const bool bIsAudioEnd, bool bClampOutput = false);

		/// Set head radius to be used in all HRTF nodes.
		/// @param InRadius New radius value in meters.
		static void SetGlobalHeadRadius(const float InRadius);
		
	protected:
		FORCEINLINE void SetHeadRadius(const float InRadius);

		/// Interpolating amp and put it into the output buffer
		/// @param bIsLeftChannel is left channel?
		/// @param Azi Azimuth in the range [0, 2*pi].
		/// @param Elev Elevation in the range [0, pi]. Pi/2 is at the ear level. Pi is on the top of the head.
		/// @param OutBuffer the output buffer
		/// @param Gain Global gain scale of all modals.
		void InterpolateAmp(bool bIsLeftChannel, float Azi, float Elev, const TArrayView<float>& OutBuffer, float Gain);

		float GetInterauralTimeDelay(float Azi, float AbsCosElev);
		float GetDecay(bool bIsLefChannel, float Azi, float AbsCosElev);
		
		bool IsFinish(bool bIsAudioEnd);
		float MapElevationToCorrectRange(float Elevation);
		void InitBuffers(float InAzimuth, float InElevation, float Gain);

		void ConvolveModal(FMultichannelBufferView& OutAudio, const TArrayView<const float>& InAudio);
		void ConvolveOneChannel(TArrayView<float>& OutAudio, const TArrayView<const float>& InAudio, const float* TwoRCosData,
             					const float* GainFData, const float* GainPhiData, float* OutD1BufferPtr, float* OutD2BufferPtr,
             					const float R2, const float Threshold = 1e-5f);
		
		float FindDeltaAngleRadians(float A1, float A2);
		
	private:
		static Audio::FAlignedFloatBuffer Freqs;
		static Audio::FAlignedFloatBuffer Phis;
		static Audio::FAlignedFloatBuffer SinPhis;
		
		static Audio::FAlignedFloatBuffer AmpElevAzi;
		static constexpr int32 NumAmpElev = 5;
		static constexpr int32 NumAmpAziPerElev = 4;
		static constexpr float AmpElevStep = UE_PI / 4;
		static constexpr float AmpAziStep = UE_PI / 2;
		
		static constexpr int32 NumModals = 32;
		
		float SamplingRate;
		int32 NumFramesPerBlock;
		int32 NumFramesTempHold;
		float HeadRadius;
		bool bShouldInitBuffers;
		float CurrentAzi;
		float CurrentElev;

		TCircularAudioBufferCustom<float> CirBuffer;
		
		FAlignedFloatBuffer AmpBufferLeft;
		FAlignedFloatBuffer AmpBufferRight;

		FAlignedFloatBuffer AmpTempInterHorBuffer;

		float R2Left;
		FAlignedFloatBuffer TwoRCosLeft;
		FAlignedFloatBuffer GainFLeft;
		FAlignedFloatBuffer GainPhiLeft;
		FAlignedFloatBuffer Out1DLeft;
		FAlignedFloatBuffer Out2DLeft;
		
		float R2Right;
		FAlignedFloatBuffer TwoRCosRight;
		FAlignedFloatBuffer GainFRight;
		FAlignedFloatBuffer GainPhiRight;
		FAlignedFloatBuffer Out1DRight;
		FAlignedFloatBuffer Out2DRight;

		FAlignedFloatBuffer TempBuffer;
		
		int32 NumLeftDelay;
		int32 NumRightDelay;
		int32 MaxDelaySamples;
		bool bIsFirstFrame;
	};
}
