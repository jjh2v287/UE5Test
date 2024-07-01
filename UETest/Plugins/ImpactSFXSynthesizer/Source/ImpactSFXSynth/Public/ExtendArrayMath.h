// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/BufferVectorOperations.h"

#define IMPS_EXP_FACTOR_MAX (15.0f)

namespace ExtendArrayMath
{
	IMPACTSFXSYNTH_API void ArraySin(TArrayView<float> InValues, TArrayView<float> OutFloatBuffer);
	IMPACTSFXSYNTH_API void ArraySinInPlace(TArrayView<float> InValues);
	
	IMPACTSFXSYNTH_API void ArrayExp(TArrayView<float> InValues, TArrayView<float> OutFloatBuffer);
	IMPACTSFXSYNTH_API void ArrayExpInPlace(TArrayView<float> InValues);
	
	IMPACTSFXSYNTH_API void ArrayImpactModal(TArrayView<const float> TimeValues,const float Amp, const float Decay,
											const float PhiSpeed, TArrayView<float> OutFloatBuffer);

	/** Modal calculation by using euler transform. Real and Img buffer size must be a multiple of audio register*/
	IMPACTSFXSYNTH_API void ArrayImpactModalEuler(TArrayView<float> RealBuffer, TArrayView<float> ImgBuffer,
											      TArrayView<const float> PBuffer, TArrayView<const float> QBuffer,
											       int32 NumModal, TArrayView<float> OutputBuffer);

	/** Modal calculation by using euler transform. Real and Img buffer size must be a multiple of audio register*/
	IMPACTSFXSYNTH_API void ArrayImpactModalEuler(TArrayView<float> RealBuffer, TArrayView<float> ImgBuffer,
												  TArrayView<const float> PBuffer, TArrayView<const float> QBuffer,
												   int32 NumModal, const float AmpScale, TArrayView<float> OutputBuffer);

	/** Calculate the total gain of all modals with euler transform buffers. Real and Img buffer size must be a multiple of audio register*/
	IMPACTSFXSYNTH_API float ArrayModalTotalGain(TArrayView<const float> RealBuffer, TArrayView<const float> ImgBuffer, int32 NumModal);
	
	FORCEINLINE float CalModalEuler(int32 NumModal, float* RealData, float* ImgData, const float* PData, const float* QData, const VectorRegister4Float& ThresholdReg);
	
	/** Separate Time in Decay and Sin calculation. Use this when Amp is updated with decay after each frame. */
	IMPACTSFXSYNTH_API void ArrayImpactModalDeltaDecay(TArrayView<const float> TimeValues,const float Amp, const float Decay,
													const float PhiSpeed, TArrayView<float> OutFloatBuffer);
												
	/** Output is added with the synthesized chirp signal */
	IMPACTSFXSYNTH_API void ArrayLinearChirpSynth(TArrayView<const float> TimeValues, const float ChirpRate,
												 const float Amp, const float Decay,
 												 const float F0, TArrayView<float> OutFloatBuffer);

	/** Output data is replaced with the synthesized chirp signal instead of adding which gives a small performance boost*/
	IMPACTSFXSYNTH_API void ArrayLinearChirpSynthSingle(TArrayView<const float> TimeValues, const float ChirpRate,
														const float Amp, const float Decay, 
														const float F0, TArrayView<float> OutFloatBuffer);

	IMPACTSFXSYNTH_API void ArraySigmoidChirpSynth(TArrayView<const float> TimeValues, const float ChirpRate,
												   const float Amp, const float Decay, const float F0,
												   const float FRange, const float Bias, TArrayView<float> OutFloatBuffer);

	IMPACTSFXSYNTH_API void ArrayExponentChirpSynth(TArrayView<const float> TimeValues, const float ChirpRate,
												   const float Amp, const float Decay, const float F0,
												   const float FRange, const float Bias, TArrayView<float> OutFloatBuffer);
	
	IMPACTSFXSYNTH_API void ArrayImpactModalFastSin(TArrayView<const float> TimeValues, TArrayView<const float> SinValues, const float Amp, const float Decay,
											TArrayView<float> OutFloatBuffer);
	
	IMPACTSFXSYNTH_API float ArrayAbsSum(TArrayView<float> InputValues);

	IMPACTSFXSYNTH_API void ArrayDeltaTimeDecayInPlace(TArrayView<const float> TimeValues, const float ExpConst, TArrayView<float> OutputValue);
	
	template <typename T>
	IMPACTSFXSYNTH_API void ArrayCircularRightShift(TArrayView<T> InArray, const int32 Shift)
	{
		const int32 NumSamples = InArray.Num();
		check(Shift >= 0 && Shift < NumSamples);
		
		TArray<T> TempArray;
		TempArray.SetNumUninitialized(NumSamples);
		T* TempBuffer = TempArray.GetData();
		T* InBuffer = InArray.GetData();
		FMemory::Memcpy(TempBuffer, InBuffer, NumSamples * sizeof(T));		
		
		const int32 NumRight = NumSamples - Shift;
		FMemory::Memcpy(&InBuffer[0], &TempBuffer[Shift], NumRight * sizeof(T));

		const int32 Remain = NumSamples - NumRight;
		FMemory::Memcpy(&InBuffer[NumRight], &TempBuffer[0], Remain * sizeof(T));
	}

};
