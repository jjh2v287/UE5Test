// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ImpactExternalForceSynth.h"
#include "ImpactSFXSynthLog.h"
#include "ModalSynth.h"
#include "DSP/FloatArrayMath.h"

#define STRENGTH_MIN (0.0001f)

namespace LBSImpactSFXSynth
{
	FImpactExternalForceSynth::FImpactExternalForceSynth(const FImpactModalObjAssetProxyPtr& ModalsParams, const float InSamplingRate,
	                                     const int32 InNumOutChannel, 
	                                     const int32 NumUsedModals, const float AmplitudeScale,
	                                     const float DecayScale, const float FreqScale)
		: SamplingRate(InSamplingRate), NumOutChannel(InNumOutChannel)
	{
		if(NumOutChannel > 2)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FImpactExternalForceSynth: Only mono and stereo output are supported!"));
			return;	
		}

		TimeStep = 1.0f / SamplingRate;
		FrameTime = 0.f;
		
		if(ModalsParams.IsValid())
		{
			TArrayView<const float> Params = ModalsParams->GetParams();
			if(Params.Num() % FModalSynth::NumParamsPerModal == 0)
			{
				InitBuffers(Params, NumUsedModals, AmplitudeScale, DecayScale, FreqScale);
				CurrentNumModals = TwoDecayCosBuffer.Num();
				return;
			}
		}
		
		UE_LOG(LogImpactSFXSynth, Error, TEXT("FImpactExternalForceSynth: Input modal is incorrect!"));		
	}

	void FImpactExternalForceSynth::InitBuffers(const TArrayView<const float>& ModalsParams, const int32 NumUsedModals,
	                                    const float AmplitudeScale, const float DecayScale, const float FreqScale)
	{
		int32 NumModals = ModalsParams.Num() / FModalSynth::NumParamsPerModal;
		NumModals = NumUsedModals > 0 ? FMath::Min(NumModals, NumUsedModals) : NumModals; 
		checkf(NumModals > 0, TEXT("NumNodals must be larger than 0!"));

		NumUsedParams = FModalSynth::NumParamsPerModal * NumModals;

		//Make sure NumModals is a multiplier of Vector register so all modals can run in SIMD when synthesizing
		const int32 RemVec = NumModals % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER;
		if(RemVec > 0)
			NumModals = NumModals + (AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER - RemVec);
			
		const int32 ClearSize = NumModals * sizeof(float);
		OutL1Buffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(OutL1Buffer.GetData(), ClearSize);
		OutL2Buffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(OutL2Buffer.GetData(), ClearSize);
		if(NumOutChannel > 1)
		{
			OutR1Buffer.SetNumUninitialized(NumModals);
			FMemory::Memzero(OutR1Buffer.GetData(), ClearSize);
			OutR2Buffer.SetNumUninitialized(NumModals);
			FMemory::Memzero(OutR2Buffer.GetData(), ClearSize);
		}
			
		TwoDecayCosBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(TwoDecayCosBuffer.GetData(), ClearSize);
		DecaySqrBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(DecaySqrBuffer.GetData(), ClearSize);
		ForceGainBuffer.SetNumUninitialized(NumModals);
		FMemory::Memzero(ForceGainBuffer.GetData(), ClearSize);

		InitPreCalBuffers(ModalsParams, AmplitudeScale, DecayScale, FreqScale);
	}

	void FImpactExternalForceSynth::InitPreCalBuffers(const TArrayView<const float>& ModalsParams, const float AmplitudeScale, const float DecayScale, const float FreqScale)
	{
		NumUsedParams = FMath::Min(NumUsedParams, ModalsParams.Num());
		const float PiTime = UE_TWO_PI * TimeStep;
		for(int i = 0, j = 0; i < NumUsedParams; i++, j++)
		{
			const float Amp = FMath::Clamp(AmplitudeScale * ModalsParams[i], -1.f, 1.f);
			i++;
			const float Decay = DecayScale * ModalsParams[i];
			i++;
			const float Freq = FMath::Clamp(FreqScale * ModalsParams[i], FModalSynth::FMin, FModalSynth::FMax);

			const float Angle = PiTime * Freq;
			const float DecayRate = FMath::Exp(-Decay * TimeStep);
			TwoDecayCosBuffer[j] = 2.f * DecayRate * FMath::Cos(Angle);
			DecaySqrBuffer[j] = DecayRate * DecayRate;
			ForceGainBuffer[j] = Amp * DecayRate * FMath::Sin(Angle);
		}
		
		CurAmpScale = AmplitudeScale;
		CurDecayScale = DecayScale;
		CurFreqScale = FreqScale;
	}

	bool FImpactExternalForceSynth::Synthesize(FMultichannelBufferView& OutAudio, const TArray<TArrayView<const float>>& InForces,
	                                           const FImpactModalObjAssetProxyPtr& ModalsParams, const float AmplitudeScale, const float DecayScale,
	                                           const float FreqScale, const bool bIsForceStop, bool bClampOutput)
	{
		if(OutL1Buffer.Num() <= 0 || InForces.Num() <= 0)
			return true;
		
		if(NumOutChannel != OutAudio.Num() || NumOutChannel != InForces.Num())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FImpactExternalForceSynth::Synthesize: the number of in/out channel isn't correct!"));
			return true;
		}

		const int32 NumRequestFrames = GetMultichannelBufferNumFrames(OutAudio);
		const int32 NumForceFrame = InForces[0].Num();
		if(NumForceFrame != NumRequestFrames)
			UE_LOG(LogImpactSFXSynth, Warning, TEXT("The number of requested frames != the number of force frames!"));
		
		const int32 NumOutputFrames = FMath::Min(NumRequestFrames, NumForceFrame);
		if(NumOutputFrames <= 0)
			return bIsForceStop;

		FrameTime = NumOutputFrames * TimeStep;
		ReInitBuffersIfNeeded(ModalsParams, AmplitudeScale, DecayScale, FreqScale);

		if(FMath::IsNearlyZero(CurAmpScale, 1e-5f))
		{
			FMemory::Memzero(OutL1Buffer.GetData(), OutL1Buffer.Num() * sizeof(float));
			FMemory::Memzero(OutL2Buffer.GetData(), OutL2Buffer.Num() * sizeof(float));
			if(NumOutChannel > 1)
			{
				FMemory::Memzero(OutR1Buffer.GetData(), OutR1Buffer.Num() * sizeof(float));
				FMemory::Memzero(OutR2Buffer.GetData(), OutR2Buffer.Num() * sizeof(float));
			}
			return bIsForceStop;
		}

		GetNumUsedModals(bIsForceStop);
		if(CurrentNumModals <= 0)
			return true;
		
		// if(bIsForceStop)
		// {
		// 	const float* OutL1BufferPtr = OutL1Buffer.GetData();
		// 	const float* OutL2BufferPtr = OutL2Buffer.GetData();
		// 	
		// 	VectorRegister4Float SumModalVector = VectorZeroFloat();
		// 	for(int j = 0; j < CurrentNumModals; j+= AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
		// 	{
		// 		SumModalVector = VectorAdd(VectorAbs(VectorLoadAligned(&OutL1BufferPtr[j])), SumModalVector);
		// 		SumModalVector = VectorAdd(VectorAbs(VectorLoadAligned(&OutL2BufferPtr[j])), SumModalVector);
		// 	}
		//
		// 	float SumVal[4];
		// 	VectorStore(SumModalVector, SumVal);
		// 	const float AbsSum = SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
		// 	if(AbsSum < STRENGTH_MIN)
		// 		return true;
		// }
		
		StartSynthesizing(OutAudio, InForces, NumOutputFrames);

		if(bClampOutput)
		{
			for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
			{
				Audio::ArrayClampInPlace(OutAudio[Channel], -1.f, 1.f);
			}
		}
		
		return false;
	}

	void FImpactExternalForceSynth::ReInitBuffersIfNeeded(const FImpactModalObjAssetProxyPtr& ModalsParamsPtr, const float AmplitudeScale, const float DecayScale, const float FreqScale)
	{
		if(ModalsParamsPtr->IsParamChanged() || !FMath::IsNearlyEqual(AmplitudeScale, CurAmpScale, 1e-2)
			|| !FMath::IsNearlyEqual(DecayScale, CurDecayScale, 1e-2)
			|| !FMath::IsNearlyEqual(FreqScale, CurFreqScale, 1e-2))
		{
			InitPreCalBuffers(ModalsParamsPtr->GetParams(), AmplitudeScale, DecayScale, FreqScale);
		}
	}

	void FImpactExternalForceSynth::GetNumUsedModals(const bool bIsForceStop)
	{
		if(bIsForceStop)
		{
			const float* OutL1BufferPtr = OutL1Buffer.GetData();
			const float* OutL2BufferPtr = OutL2Buffer.GetData();

			int32 j = CurrentNumModals - AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER;
			for(; j > -1; j -= AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				VectorRegister4Float y1 = VectorLoadAligned(&OutL1BufferPtr[j]);
				VectorRegister4Float y2 = VectorLoadAligned(&OutL2BufferPtr[j]);
				VectorRegister4Float TotalMagVec = VectorAdd(VectorAbs(y1), VectorAbs(y2));
				
				float SumVal[4];
				VectorStore(TotalMagVec, SumVal);
				const float SampleSum = SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
				if(SampleSum > STRENGTH_MIN)
					break;
			}
			CurrentNumModals = j + AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER;
			return;
		}
		
		CurrentNumModals = TwoDecayCosBuffer.Num(); 
	}
	
	void FImpactExternalForceSynth::StartSynthesizing(FMultichannelBufferView& OutAudio, const TArray<TArrayView<const float>>& InForces,
																	const int32 NumOutputFrames)
	{
		const float* TwoRCosData = TwoDecayCosBuffer.GetData();
		const float* R2Data = DecaySqrBuffer.GetData();
		const float* GainFData = ForceGainBuffer.GetData();
		float* OutL1BufferPtr = OutL1Buffer.GetData();
		float* OutL2BufferPtr = OutL2Buffer.GetData();

		TArrayView<const float> LeftForceBuffer = InForces[0];
		VectorRegister4Float ThresholdReg = VectorSet(1e-5f, 1e-5f, 1e-5f, 1e-5f);
		
		if(NumOutChannel < 2)
		{
			TArrayView<float> OutBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
			for(int i = 0; i < NumOutputFrames; i++)
			{
				VectorRegister4Float ForceReg = VectorLoadFloat1(&LeftForceBuffer[i]);
				VectorRegister4Float SumModalVector = VectorZeroFloat();
				for(int j = 0; j < CurrentNumModals; j += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float y1 = VectorLoadAligned(&OutL1BufferPtr[j]);
					VectorRegister4Float y2 = VectorLoadAligned(&OutL2BufferPtr[j]);
					VectorRegister4Float DecayLowNumReg = VectorAdd(VectorAbs(y1), VectorAbs(y2));
					DecayLowNumReg = VectorCompareGE(DecayLowNumReg, ThresholdReg);
					y1 = VectorBitwiseAnd(y1, DecayLowNumReg);
					y2 = VectorBitwiseAnd(y2, DecayLowNumReg);
					
					VectorRegister4Float TwoRCos = VectorMultiply(VectorLoadAligned(&TwoRCosData[j]), y1);
					VectorRegister4Float R2 = VectorMultiply(VectorLoadAligned(&R2Data[j]), y2);
				
					VectorStore(y1, &OutL2BufferPtr[j]);
					y1 = VectorSubtract(TwoRCos, R2);
					y1 = VectorMultiplyAdd(VectorLoadAligned(&GainFData[j]), ForceReg, y1);
				
					SumModalVector = VectorAdd(SumModalVector, y1);
					VectorStore(y1, &OutL1BufferPtr[j]);
				}
				
				float SumVal[4];
				VectorStore(SumModalVector, SumVal);
				const float SampleSum = SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
			
				OutBuffer[i] = SampleSum;
			}
		}
		else
		{
			float* OutR1BufferPtr = OutR1Buffer.GetData();
			float* OutR2BufferPtr = OutR2Buffer.GetData();

			TArrayView<float> LeftChannelBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
			TArrayView<float> RightChannelBuffer = TArrayView<float>(OutAudio[1].GetData(), NumOutputFrames);
			TArrayView<const float> RightForceBuffer = InForces[1];
			for(int i = 0; i < NumOutputFrames; i++)
			{
				VectorRegister4Float LeftForceReg = VectorLoadFloat1(&LeftForceBuffer[i]);
				VectorRegister4Float RightForceReg = VectorLoadFloat1(&RightForceBuffer[i]);
				VectorRegister4Float LeftSumVector = VectorZeroFloat();
				VectorRegister4Float RightSumVector = VectorZeroFloat();
				for(int j = 0; j < CurrentNumModals; j += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
				{
					VectorRegister4Float TwoRCos =VectorLoadAligned(&TwoRCosData[j]);
					VectorRegister4Float R2 = VectorLoadAligned(&R2Data[j]);
					VectorRegister4Float GainF = VectorLoadAligned(&GainFData[j]);
					
					VectorRegister4Float y1 = VectorLoadAligned(&OutL1BufferPtr[j]);
					VectorRegister4Float y2 = VectorLoadAligned(&OutL2BufferPtr[j]);
					VectorRegister4Float DecayLowNumReg = VectorAdd(VectorAbs(y1), VectorAbs(y2));
					DecayLowNumReg = VectorCompareGE(DecayLowNumReg, ThresholdReg);
					y1 = VectorBitwiseAnd(y1, DecayLowNumReg);
					y2 = VectorBitwiseAnd(y2, DecayLowNumReg);
					
					VectorStore(y1, &OutL2BufferPtr[j]);
					y1 = VectorMultiply(TwoRCos, y1);
					y2 = VectorMultiply(R2, y2);
					y1 = VectorSubtract(y1, y2);
					y1 = VectorMultiplyAdd(GainF, LeftForceReg, y1);
					LeftSumVector = VectorAdd(LeftSumVector, y1);
					VectorStore(y1, &OutL1BufferPtr[j]);

					y1 = VectorLoadAligned(&OutR1BufferPtr[j]);
					y2 = VectorLoadAligned(&OutR2BufferPtr[j]);
					DecayLowNumReg = VectorAdd(VectorAbs(y1), VectorAbs(y2));
					DecayLowNumReg = VectorCompareGE(DecayLowNumReg, ThresholdReg);
					y1 = VectorBitwiseAnd(y1, DecayLowNumReg);
					y2 = VectorBitwiseAnd(y2, DecayLowNumReg);
					
					VectorStore(y1, &OutR2BufferPtr[j]);
					y1 = VectorMultiply(TwoRCos, y1);
					y2 = VectorMultiply(R2, y2);
					y1 = VectorSubtract(y1, y2);
					y1 = VectorMultiplyAdd(GainF, RightForceReg, y1);
					RightSumVector = VectorAdd(RightSumVector, y1);
					VectorStore(y1, &OutR1BufferPtr[j]);	
				}
		
				float SumVal[4];
				VectorStore(LeftSumVector, SumVal);
				LeftChannelBuffer[i] =  SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
				
				VectorStore(RightSumVector, SumVal);
				RightChannelBuffer[i] =  SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
			}
		}
	}
}

#undef STRENGTH_MIN