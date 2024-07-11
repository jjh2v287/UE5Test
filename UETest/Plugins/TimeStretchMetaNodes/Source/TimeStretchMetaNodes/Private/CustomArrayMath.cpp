// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "CustomArrayMath.h"
#include "CoreMinimal.h"
#include "Math/UnrealMathVectorConstants.h"
#include "SignalProcessingModule.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "DSP/FloatArrayMath.h"

CSV_DEFINE_CATEGORY(Audio_CustomArrayMatch, false);

namespace CustomArrayMath
{
	namespace MathIntrinsics
	{
		const float Loge10 = FMath::Loge(10.f);
		const int32 SimdMask = 0xFFFFFFFC;
		const int32 NotSimdMask = 0x00000003;
	}
	
	void ArraySign(TArrayView<const float> InBuffer, TArrayView<float> OutBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_CustomArrayMatch, ArraySign);
		const int32 Num = InBuffer.Num();
		check(OutBuffer.Num() == Num);

		const float* InData = InBuffer.GetData();
		float* OutData = OutBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float Input = VectorLoad(&InData[i]);
				VectorStore(VectorSign(Input), &OutData[i]);
			}
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; ++i)
			{
				OutData[i] = FMath::Sign(InData[i]);
			}
		}
	}

	void ArraySignInPlace(TArrayView<float> InBuffer)
	{
		CSV_SCOPED_TIMING_STAT(Audio_CustomArrayMatch, ArraySignInPlace);
		const int32 Num = InBuffer.Num();

		float* InData = InBuffer.GetData();
		
		const int32 NumToSimd = Num & MathIntrinsics::SimdMask;
		const int32 NumNotToSimd = Num & MathIntrinsics::NotSimdMask;

		if (NumToSimd)
		{
			for (int32 i = 0; i < NumToSimd; i += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float Input = VectorLoad(&InData[i]);
				VectorStore(VectorSign(Input), &InData[i]);
			}
		}

		if (NumNotToSimd)
		{
			for (int32 i = NumToSimd; i < Num; ++i)
			{
				InData[i] = FMath::Sign(InData[i]);
			}
		}
	}
}
