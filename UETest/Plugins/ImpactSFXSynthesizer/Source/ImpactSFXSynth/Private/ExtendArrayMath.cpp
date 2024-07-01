// Copyright 2023-2024, Le Binh Son, All rights reserved.

#include "ExtendArrayMath.h"

#include "CoreMinimal.h"
#include "Math/UnrealMathVectorConstants.h"
#include "SignalProcessingModule.h"
#include "DSP/FloatArrayMath.h"
#include "ProfilingDebugging/CsvProfiler.h"

#define  LOW_THRESH (1e-6f)

CSV_DEFINE_CATEGORY(Audio_ExendArrayMatch, false);

namespace ExtendArrayMath
{
	namespace MathIntrinsics
	{
		const int32 SimdMask = 0xFFFFFFFC;
		const int32 NotSimdMask = 0x00000003;
	}
	
	void ArraySin(TArrayView<float> InValues, TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArraySin);

		const int32 Num = InValues.Num();
		float* InValuesData = InValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float VectorData = VectorLoad(&InValuesData[i]);
				VectorData = VectorSin(VectorData);
				VectorStore(VectorData, &OutBufferPtr[i]);
			}
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; i++)
			{
				OutBufferPtr[i] = FMath::Sin(InValuesData[i]);
			}
		}
	}

	void ArraySinInPlace(TArrayView<float> InValues)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArraySinInPlace);

		const int32 Num = InValues.Num();
		float* InValuesData = InValues.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float VectorData = VectorLoad(&InValuesData[i]);
				VectorData = VectorSin(VectorData);
				VectorStore(VectorData, &InValuesData[i]);
			}
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; i++)
			{
				InValuesData[i] = FMath::Sin(InValuesData[i]);
			}
		}
	}

	void ArrayExp(TArrayView<float> InValues, TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayExp);

		const int32 Num = InValues.Num();
		float* InValuesData = InValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float VectorData = VectorLoad(&InValuesData[i]);
				VectorData = VectorExp(VectorData);
				VectorStore(VectorData, &OutBufferPtr[i]);
			}
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; i++)
			{
				OutBufferPtr[i] = FMath::Exp(InValuesData[i]);
			}
		}
	}

	void ArrayExpInPlace(TArrayView<float> InValues)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayExpInPlace);

		const int32 Num = InValues.Num();
		float* InValuesData = InValues.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float VectorData = VectorLoad(&InValuesData[i]);
				VectorData = VectorExp(VectorData);
				VectorStore(VectorData, &InValuesData[i]);
			}
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; i++)
			{
				InValuesData[i] = FMath::Exp(InValuesData[i]);
			}
		}
	}

	void ArrayImpactModal(TArrayView<const float> TimeValues, const float Amp, const float Decay, const float PhiSpeed, TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayImpactModal);

		const int32 Num = OutFloatBuffer.Num();
		check(TimeValues.Num() >= Num);
		
		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			if(FMath::IsNearlyZero(Decay))
			{
				VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
				VectorRegister4Float FreqVector = VectorLoadFloat1(&PhiSpeed);
			
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
				
					VectorRegister4Float VectorModal = VectorMultiply(TimeVectorData, FreqVector);
					VectorModal = VectorSin(VectorModal);
					
					VectorModal = VectorMultiplyAdd(AmpVector, VectorModal, VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorModal, &OutBufferPtr[i]);
				}
			}
			else
			{
				VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
				VectorRegister4Float DecayVector = VectorLoadFloat1(&Decay);
				VectorRegister4Float FreqVector = VectorLoadFloat1(&PhiSpeed);
			
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
				
					VectorRegister4Float VectorModal = VectorMultiply(TimeVectorData, FreqVector);
					VectorModal = VectorSin(VectorModal);

					VectorRegister4Float VectorTemp = VectorMultiply(TimeVectorData, DecayVector);
					VectorTemp = VectorExp(VectorTemp);
					VectorModal = VectorMultiply(VectorTemp, VectorModal);
				
					VectorTemp = VectorLoad(&OutBufferPtr[i]);
					VectorModal = VectorMultiplyAdd(AmpVector, VectorModal, VectorTemp);
					VectorStore(VectorModal, &OutBufferPtr[i]);
				}
			}
		}

		if (NumNotToSimd)
		{
			if(FMath::IsNearlyZero(Decay))
			{
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Sin(PhiSpeed * Time);
				}
			}
			else
			{
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Exp(Decay * Time) * FMath::Sin(PhiSpeed * Time);
				}
			}
		}
	}

	void ArrayImpactModalEuler(TArrayView<float> RealBuffer, TArrayView<float> ImgBuffer,
	                           TArrayView<const float> PBuffer, TArrayView<const float> QBuffer,
	                           int32 NumModal, TArrayView<float> OutputBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayImpactModalEuler);

		const int32 NumData = RealBuffer.Num();
		checkf((NumData % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER) == 0, TEXT("NumModal must be a multiple of register size"))
		
		NumModal = NumModal > 0 ? FMath::Min(NumData, NumModal) : NumData;
		
		float* RealData = RealBuffer.GetData();
		float* ImgData = ImgBuffer.GetData();
		const float* PData = PBuffer.GetData();
		const float* QData = QBuffer.GetData();

		const int32 NumOutputFrames = OutputBuffer.Num();
		
		const VectorRegister4Float ThresholdReg = VectorSet(LOW_THRESH, LOW_THRESH, LOW_THRESH, LOW_THRESH);
		for(int outFrame = 0; outFrame < NumOutputFrames; outFrame++)
		{
			const float TotalSum = CalModalEuler(NumModal, RealData, ImgData, PData, QData, ThresholdReg);
			OutputBuffer[outFrame] = TotalSum;
		}
	}

	void ArrayImpactModalEuler(TArrayView<float> RealBuffer, TArrayView<float> ImgBuffer,
		TArrayView<const float> PBuffer, TArrayView<const float> QBuffer, int32 NumModal, const float AmpScale,
		TArrayView<float> OutputBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayImpactModalEuler);
		
		const int32 NumData = RealBuffer.Num();
		checkf((NumData % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER) == 0, TEXT("NumModal must be a multiple of register size"))
		
		NumModal = NumModal > 0 ? FMath::Min(NumData, NumModal) : NumData;
		
		float* RealData = RealBuffer.GetData();
		float* ImgData = ImgBuffer.GetData();
		const float* PData = PBuffer.GetData();
		const float* QData = QBuffer.GetData();

		const int32 NumOutputFrames = OutputBuffer.Num();
		const VectorRegister4Float ThresholdReg = VectorSet(LOW_THRESH, LOW_THRESH, LOW_THRESH, LOW_THRESH);
		for(int outFrame = 0; outFrame < NumOutputFrames; outFrame++)
		{
			const float TotalSum = CalModalEuler(NumModal, RealData, ImgData, PData, QData, ThresholdReg);
			OutputBuffer[outFrame] = TotalSum * AmpScale;
		}
	}

	float CalModalEuler(int32 NumModal, float* RealData, float* ImgData, const float* PData, const float* QData, const VectorRegister4Float& ThresholdReg)
	{
		VectorRegister4Float SumVector = VectorZeroFloat();
		
		for (int32 i = 0; i < NumModal; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
		{
			VectorRegister4Float RealVectorData = VectorLoadAligned(&RealData[i]);
			VectorRegister4Float ImgVectorData = VectorLoadAligned(&ImgData[i]);
			VectorRegister4Float DecayLowNumReg = VectorAdd(VectorAbs(RealVectorData), VectorAbs(ImgVectorData));
			DecayLowNumReg = VectorCompareGE(DecayLowNumReg, ThresholdReg);
			RealVectorData = VectorBitwiseAnd(RealVectorData, DecayLowNumReg);
			ImgVectorData = VectorBitwiseAnd(ImgVectorData, DecayLowNumReg);
			
			VectorRegister4Float PVector = VectorLoadAligned(&PData[i]);
			VectorRegister4Float QVector = VectorLoadAligned(&QData[i]);

			VectorRegister4Float RealTempVector = VectorSubtract(VectorMultiply(RealVectorData, PVector), VectorMultiply(ImgVectorData, QVector));
			VectorRegister4Float ImgTempVector = VectorMultiply(RealVectorData, QVector);
			ImgTempVector = VectorMultiplyAdd(ImgVectorData, PVector, ImgTempVector);
					
			VectorStore(RealTempVector, &RealData[i]);
			VectorStore(ImgTempVector, &ImgData[i]);

			SumVector = VectorAdd(SumVector, RealTempVector);
		}

		float SumVal[4];
		VectorStore(SumVector, SumVal);
		return  SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
	}

	float ArrayModalTotalGain(TArrayView<const float> RealBuffer, TArrayView<const float> ImgBuffer, int32 NumModal)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayModalTotalGain);

		const int32 NumData = RealBuffer.Num();
		checkf((NumData % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER) == 0, TEXT("NumModal must be a multiple of register size"))
		
		NumModal = NumModal > 0 ? FMath::Min(NumData, NumModal) : NumData;
		
		const float* RealData = RealBuffer.GetData();
		const float* ImgData = ImgBuffer.GetData();
		
		VectorRegister4Float SumVector = VectorZeroFloat();
		for (int32 i = 0; i < NumModal; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
		{
			VectorRegister4Float RealVectorData = VectorAbs(VectorLoadAligned(&RealData[i]));
			VectorRegister4Float ImgVectorData = VectorAbs(VectorLoadAligned(&ImgData[i]));

			SumVector = VectorAdd(VectorAdd(RealVectorData, ImgVectorData), SumVector);
		}

		float SumVal[4];
		VectorStore(SumVector, SumVal);
		return SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
	}

	void ArrayImpactModalDeltaDecay(TArrayView<const float> TimeValues, const float Amp, const float Decay,
	                                const float PhiSpeed, TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayImpactModalDeltaDecay);

		const int32 Num = OutFloatBuffer.Num();
		check(TimeValues.Num() >= Num);
		
		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
			VectorRegister4Float FreqVector = VectorLoadFloat1(&PhiSpeed);
			
			if(FMath::IsNearlyZero(Decay))
			{
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					VectorRegister4Float VectorModal = VectorMultiply(TimeVectorData, FreqVector);
					VectorModal = VectorMultiplyAdd(AmpVector, VectorSin(VectorModal), VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorModal, &OutBufferPtr[i]);
				}
			}
			else
			{
				VectorRegister4Float DecayVector = VectorLoadFloat1(&Decay);
				VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValues[0]);
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					VectorRegister4Float VectorTemp = VectorMultiply(VectorSubtract(TimeVectorData, DurationVector), DecayVector);
					
					VectorRegister4Float VectorModal = VectorMultiply(TimeVectorData, FreqVector);
					VectorModal = VectorMultiply(VectorExp(VectorTemp), VectorSin(VectorModal));
				
					VectorTemp = VectorLoad(&OutBufferPtr[i]);
					VectorModal = VectorMultiplyAdd(AmpVector, VectorModal, VectorTemp);
					VectorStore(VectorModal, &OutBufferPtr[i]);
				}
			}
		}

		if (NumNotToSimd)
		{
			if(FMath::IsNearlyZero(Decay))
			{
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Sin(PhiSpeed * Time);
				}
			}
			else
			{
				const float Duration = TimeValues[0];
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Exp(Decay * (Time - Duration)) * FMath::Sin(PhiSpeed * Time);
				}
			}
		}
	}

	void ArrayLinearChirpSynth(TArrayView<const float> TimeValues, const float ChirpRate,
							  const float Amp, const float Decay,
							  const float F0, TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayChirpSynth);

		const int32 Num = OutFloatBuffer.Num();
		check(TimeValues.Num() >= Num);
		
		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
			VectorRegister4Float FreqVector = VectorLoadFloat1(&F0);
			VectorRegister4Float ChirpRateVector = VectorLoadFloat1(&ChirpRate);
			VectorRegister4Float TwoPi = MakeVectorRegisterFloat(UE_TWO_PI, UE_TWO_PI, UE_TWO_PI, UE_TWO_PI);
			
			if(FMath::IsNearlyZero(Decay)) 
			{ //Don't calculate exp(Decay * t) 
				VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValues[0]);
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
				
					VectorRegister4Float VectorChirp = VectorMultiply(TimeVectorData, ChirpRateVector);
					VectorChirp = VectorAdd(VectorChirp, FreqVector);
					VectorChirp = VectorMultiply(TimeVectorData, VectorChirp);
					VectorChirp = VectorSin(VectorMultiply(TwoPi, VectorChirp));
					
					VectorChirp = VectorMultiplyAdd(AmpVector, VectorChirp, VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorChirp, &OutBufferPtr[i]);
				}
			}
			else
			{
				VectorRegister4Float DecayVector = VectorLoadFloat1(&Decay);
				VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValues[0]);
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					
					VectorRegister4Float VectorTemp = VectorMultiply(VectorSubtract(TimeVectorData, DurationVector), DecayVector);
					VectorTemp = VectorExp(VectorTemp);
					
					VectorRegister4Float VectorChirp = VectorMultiply(TimeVectorData, ChirpRateVector);
					VectorChirp = VectorAdd(VectorChirp, FreqVector);
					VectorChirp = VectorMultiply(TimeVectorData, VectorChirp);
					VectorChirp = VectorSin(VectorMultiply(TwoPi, VectorChirp));
					
					VectorChirp = VectorMultiply(VectorTemp, VectorChirp);
				
					VectorTemp = VectorLoad(&OutBufferPtr[i]);
					VectorChirp = VectorMultiplyAdd(AmpVector, VectorChirp, VectorTemp);
					VectorStore(VectorChirp, &OutBufferPtr[i]);
				}
			}
		}

		if (NumNotToSimd)
		{
			if(FMath::IsNearlyZero(Decay))
			{
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Sin(UE_TWO_PI * Time * (F0 + ChirpRate * Time));
				}
			}
			else
			{
				const float Duration = TimeValues[0];
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Exp(Decay * (Time - Duration)) * FMath::Sin(UE_TWO_PI * Time * (F0 + ChirpRate * Time));
				}	
			}
		}
	}

	void ArrayLinearChirpSynthSingle(TArrayView<const float> TimeValues, const float ChirpRate,
									const float Amp, const float Decay,
									const float F0, TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayChirpSynthSingle);

		const int32 Num = OutFloatBuffer.Num();
		check(TimeValues.Num() >= Num);
		
		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
			VectorRegister4Float FreqVector = VectorLoadFloat1(&F0);
			VectorRegister4Float ChirpRateVector = VectorLoadFloat1(&ChirpRate);
			VectorRegister4Float TwoPi = MakeVectorRegisterFloat(UE_TWO_PI, UE_TWO_PI, UE_TWO_PI, UE_TWO_PI);
			
			if(FMath::IsNearlyZero(Decay)) 
			{ //Don't calculate exp(Decay * t) 
				VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValues[0]);
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
				
					VectorRegister4Float VectorChirp = VectorMultiply(TimeVectorData, ChirpRateVector);
					VectorChirp = VectorAdd(VectorChirp, FreqVector);
					VectorChirp = VectorMultiply(TimeVectorData, VectorChirp);
					VectorChirp = VectorSin(VectorMultiply(TwoPi, VectorChirp));
					
					VectorChirp = VectorMultiplyAdd(AmpVector, VectorChirp, VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorChirp, &OutBufferPtr[i]);
				}
			}
			else
			{
				VectorRegister4Float DecayVector = VectorLoadFloat1(&Decay);
				VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValues[0]);
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					
					VectorRegister4Float VectorTemp = VectorMultiply(VectorSubtract(TimeVectorData, DurationVector), DecayVector);
					VectorTemp = VectorExp(VectorTemp);
					
					VectorRegister4Float VectorChirp = VectorMultiply(TimeVectorData, ChirpRateVector);
					VectorChirp = VectorAdd(VectorChirp, FreqVector);
					VectorChirp = VectorMultiply(TimeVectorData, VectorChirp);
					VectorChirp = VectorSin(VectorMultiply(TwoPi, VectorChirp));
					
					VectorChirp = VectorMultiply(VectorTemp, VectorChirp);					
					VectorStore(VectorMultiply(AmpVector, VectorChirp), &OutBufferPtr[i]);
				}
			}
		}

		if (NumNotToSimd)
		{
			if(FMath::IsNearlyZero(Decay))
			{
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = Amp * FMath::Sin(UE_TWO_PI * Time * (F0 + ChirpRate * Time));
				}
			}
			else
			{
				const float Duration = TimeValues[0];
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					OutFloatBuffer[i] = Amp * FMath::Exp(Decay * (Time - Duration)) * FMath::Sin(UE_TWO_PI * Time * (F0 + ChirpRate * Time));
				}	
			}
		}
	}

	void ArraySigmoidChirpSynth(TArrayView<const float> TimeValues, const float ChirpRate, const float Amp,
								const float Decay, const float F0, const float FRange, const float Bias,
								TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArraySigmoidChirpSynth);

		const int32 Num = OutFloatBuffer.Num();
		check(TimeValues.Num() >= Num);
		
		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;
		if (NumToSimd)
		{
			VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
			VectorRegister4Float FreqVector = VectorLoadFloat1(&F0);
			VectorRegister4Float FRangeVector = VectorLoadFloat1(&FRange);
			VectorRegister4Float ChirpRateVector = VectorLoadFloat1(&ChirpRate);
			VectorRegister4Float TwoPi = MakeVectorRegisterFloat(UE_TWO_PI, UE_TWO_PI, UE_TWO_PI, UE_TWO_PI);
			VectorRegister4Float OneVector = VectorOneFloat();
			VectorRegister4Float TwoVector = MakeVectorRegisterFloat(2.f, 2.f, 2.f, 2.f);
			// Disable this until we find a better way to sync phi bias when changing frequency
			// VectorRegister4Float BiasVector = VectorLoadFloat1(&Bias);
			
			if(FMath::IsNearlyZero(Decay)) 
			{ //Don't calculate exp(Decay * t) 
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					VectorRegister4Float VectorChirp = VectorMultiply(VectorNegate(TimeVectorData), ChirpRateVector);
					VectorChirp = VectorExp(VectorChirp);
					VectorChirp = VectorAdd(VectorChirp, OneVector);
					VectorChirp = VectorDivide(TwoVector, VectorChirp);
					VectorChirp = VectorSubtract(VectorChirp, OneVector);
					VectorChirp = VectorMultiply(VectorChirp, FRangeVector);
					
					VectorChirp = VectorMultiply(VectorAdd(VectorChirp,FreqVector), TimeVectorData);
					//VectorChirp = VectorSin(VectorAdd(BiasVector, VectorMultiply(TwoPi, VectorChirp)));
					VectorChirp = VectorSin(VectorMultiply(TwoPi, VectorChirp));
					
					VectorChirp = VectorMultiplyAdd(AmpVector, VectorChirp, VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorChirp, &OutBufferPtr[i]);
				}
			}
			else
			{
				VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValues[0]);
				VectorRegister4Float DecayVector = VectorLoadFloat1(&Decay);
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					VectorRegister4Float VectorChirp = VectorMultiply(VectorNegate(TimeVectorData), ChirpRateVector);
					VectorChirp = VectorExp(VectorChirp);
					VectorChirp = VectorAdd(VectorChirp, OneVector);
					VectorChirp = VectorDivide(TwoVector, VectorChirp);
					VectorChirp = VectorSubtract(VectorChirp, OneVector);
					VectorChirp = VectorMultiply(VectorChirp, FRangeVector);
					
					VectorChirp = VectorMultiply(VectorAdd(VectorChirp,FreqVector), TimeVectorData);
					VectorChirp = VectorSin(VectorMultiply(TwoPi, VectorChirp));

					VectorRegister4Float VectorTemp = VectorMultiply(VectorSubtract(TimeVectorData, DurationVector), DecayVector);
					VectorTemp = VectorExp(VectorTemp);
					VectorChirp = VectorMultiplyAdd(AmpVector, VectorMultiply(VectorChirp, VectorTemp), VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorChirp, &OutBufferPtr[i]);
				}
			}
		}

		if (NumNotToSimd)
		{
			if(FMath::IsNearlyZero(Decay))
			{
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					float Freq = FRange * (-1.f + 2.0f / (1.0f + FMath::Exp(-Time * ChirpRate)));
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Sin(UE_TWO_PI * ((F0 + Freq) * Time));
				}
			}
			else
			{
				const float Duration = TimeValues[0];
				for (int32 i = NumToSimd; i < Num; i++)
				{
					float Time = TimeValuesData[i];
					float Freq = FRange * (-1.f + 2.0f / (1.0f + FMath::Exp(-Time * ChirpRate)));
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Exp(Decay * (Time - Duration)) * FMath::Sin(UE_TWO_PI * ((F0 + Freq) * Time));
				}	
			}
		}
	}

	void ArrayExponentChirpSynth(TArrayView<const float> TimeValues, const float ChirpRate, const float Amp,
		const float Decay, const float F0, const float FRange, const float Bias,
		TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayExponentChirpSynth);

		const int32 Num = OutFloatBuffer.Num();
		check(TimeValues.Num() >= Num);
		
		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		const float DeltaF0Range = (F0 - FRange) * UE_TWO_PI;
		const float ExpConst = 2.0f * FRange / ChirpRate * UE_TWO_PI;
		if (NumToSimd)
		{
			VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
			VectorRegister4Float DeltaF0Vector = VectorLoadFloat1(&DeltaF0Range);

			VectorRegister4Float ChirpRateVector = VectorLoadFloat1(&ChirpRate);
			VectorRegister4Float OneVector = VectorOneFloat();
			VectorRegister4Float ExpConstVector = VectorLoadFloat1(&ExpConst);
			//Disable this until we find a better and more performance way to sync phi when changing frequency 
			//VectorRegister4Float BiasVector = VectorLoadFloat1(&Bias);
			
			if(FMath::IsNearlyZero(Decay)) 
			{ //Don't calculate exp(Decay * t)
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					VectorRegister4Float VectorChirp = VectorMultiply(TimeVectorData, ChirpRateVector);
					VectorChirp = VectorAdd(VectorExp(VectorChirp), OneVector);
					VectorChirp = VectorMultiply(VectorLog(VectorChirp), ExpConstVector);
			
					VectorChirp = VectorAdd(VectorMultiply(DeltaF0Vector, TimeVectorData), VectorChirp);
					//VectorChirp = VectorSin(VectorAdd(BiasVector, VectorChirp));
					VectorChirp = VectorSin(VectorChirp);
			
					VectorChirp = VectorMultiplyAdd(AmpVector, VectorChirp, VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorChirp, &OutBufferPtr[i]);
				}
			}
			else
			{
				VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValues[0]);
				VectorRegister4Float DecayVector = VectorLoadFloat1(&Decay);
				
				for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
					VectorRegister4Float VectorChirp = VectorMultiply(TimeVectorData, ChirpRateVector);
					VectorChirp = VectorAdd(VectorExp(VectorChirp), OneVector);
					VectorChirp = VectorMultiply(VectorLog(VectorChirp), ExpConstVector);
					
					VectorChirp = VectorAdd(VectorMultiply(DeltaF0Vector, TimeVectorData), VectorChirp);
					VectorChirp = VectorSin(VectorChirp);

					VectorRegister4Float VectorTemp = VectorMultiply(VectorSubtract(TimeVectorData, DurationVector), DecayVector);
					VectorTemp = VectorExp(VectorTemp);
					VectorChirp = VectorMultiplyAdd(AmpVector, VectorMultiply(VectorChirp, VectorTemp), VectorLoad(&OutBufferPtr[i]));
					VectorStore(VectorChirp, &OutBufferPtr[i]);
				}
			}
		}

		if (NumNotToSimd)
		{
			if(FMath::IsNearlyZero(Decay))
			{
				for (int32 i = NumToSimd; i < Num; i++)
				{
					const float Time = TimeValuesData[i];
					const float PhiSpeed = DeltaF0Range * Time + ExpConst * FMath::Loge(FMath::Exp(Time * ChirpRate) + 1.0f);
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Sin(PhiSpeed);
				}
			}
			else
			{
				const float Duration = TimeValues[0];
				for (int32 i = NumToSimd; i < Num; i++)
				{
					const float Time = TimeValuesData[i];
					const float PhiSpeed = DeltaF0Range * Time + ExpConst * FMath::Loge(FMath::Exp(Time * ChirpRate) + 1.0f);
					OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Exp(Decay * (Time - Duration)) * FMath::Sin(PhiSpeed);
				}	
			}
		}
	}

	void ArrayImpactModalFastSin(TArrayView<const float> TimeValues,  TArrayView<const float> SinValues, const float Amp,
	                             const float Decay, TArrayView<float> OutFloatBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayImpactModalFastSin);

		const int32 Num = OutFloatBuffer.Num();
		check(TimeValues.Num() >= Num);
		
		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutFloatBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			VectorRegister4Float AmpVector = VectorLoadFloat1(&Amp);
			VectorRegister4Float DecayVector = VectorLoadFloat1(&Decay);
			
			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float VectorDecay = VectorMultiply(VectorLoadAligned(&TimeValuesData[i]), DecayVector);
				VectorDecay = VectorExp(VectorDecay);
				
				VectorRegister4Float VectorModal = VectorLoadAligned(&SinValues[i]);
				VectorModal = VectorMultiply(VectorDecay, VectorModal);
				VectorModal = VectorMultiplyAdd(AmpVector, VectorModal, VectorLoad(&OutBufferPtr[i]));
				VectorStore(VectorModal, &OutBufferPtr[i]);
			}
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; i++)
			{
				OutFloatBuffer[i] = OutFloatBuffer[i] + Amp * FMath::Exp(Decay * TimeValuesData[i]) * SinValues[i];
			}
		}
	}

	float ArrayAbsSum(TArrayView<float> InputValues)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayAbsSum);
		const int32 Num = InputValues.Num();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		float Sum = 0.0f;

		if (NumToSimd)
		{
			VectorRegister4Float VectorSum = VectorZero();

			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float Input = VectorAbs(VectorLoad(&InputValues[i]));
				VectorSum = VectorAdd(VectorSum, Input);
			}

			float PartionedSums[4];
			VectorStore(VectorSum, PartionedSums);

			Sum += PartionedSums[0] + PartionedSums[1] + PartionedSums[2] + PartionedSums[3];
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; ++i)
			{
				Sum += FMath::Abs(InputValues[i]);
			}
		}

		return Sum;
	}

	void ArrayDeltaTimeDecayInPlace(TArrayView<const float> TimeValues, const float ExpConst, TArrayView<float> OutputValue)
	{
		CSV_SCOPED_TIMING_STAT(Audio_ExendArrayMatch, ArrayExpMulInPlace);
		const int32 Num = OutputValue.Num();
		check(Num <= TimeValues.Num())
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		const float* TimeValuesData = TimeValues.GetData();
		float* OutBufferPtr = OutputValue.GetData();

		if (NumToSimd)
		{
			VectorRegister4Float ExpConstVector = VectorLoadFloat1(&ExpConst);
			VectorRegister4Float DurationVector = VectorLoadFloat1(&TimeValuesData[0]);

			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float TimeVectorData = VectorLoadAligned(&TimeValuesData[i]);
				TimeVectorData = VectorSubtract(TimeVectorData, DurationVector);
				TimeVectorData = VectorExp(VectorMultiply(TimeVectorData, ExpConstVector));
				TimeVectorData = VectorMultiply(TimeVectorData, VectorLoad(&OutBufferPtr[i]));
				VectorStore(TimeVectorData, &OutBufferPtr[i]);
			}
		}

		if (NumNotToSimd)
		{
			const float Duration = TimeValues[0]; 
			for (int32 i = NumToSimd; i < Num; ++i)
			{
				OutputValue[i] = OutputValue[i] * FMath::Exp(ExpConst * (TimeValues[i] - Duration));
			}
		}
	}
}
