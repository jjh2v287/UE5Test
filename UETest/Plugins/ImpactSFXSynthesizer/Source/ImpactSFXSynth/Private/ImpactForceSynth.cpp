// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ImpactForceSynth.h"
#include "ImpactSFXSynthLog.h"
#include "ModalSynth.h"
#include "DSP/FloatArrayMath.h"

#define STRENGTH_MIN (0.001f)

namespace LBSImpactSFXSynth
{
	FImpactForceSynth::FImpactForceSynth(const FImpactModalObjAssetProxyPtr& ModalsParams, const float InSamplingRate,
	                                     const int32 InNumOutChannel,  const FImpactForceSpawnParams& ForceSpawnParams,
	                                     const int32 NumUsedModals, const float AmplitudeScale,
	                                     const float DecayScale, const float FreqScale, const int32 InSeed,
	                                     const bool bInForceReset)
		: SamplingRate(InSamplingRate), NumOutChannel(InNumOutChannel),
		  LastForceStrength(0.f), LastForceDecayRate(0.f), CurrentForceStrength(0.f), LastImpactSpawnTime(0.f),
		  CurrentTime(0.f), LastImpactCenterTime(0.f), FrameTime(0.f), bNoForceReset(!bInForceReset)
	{
		if(NumOutChannel > 2)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FImpactForceSynth: Only mono and stereo output are supported!"));
			return;	
		}

		TimeStep = 1.0f / SamplingRate;
		
		// Set scale differently so we will init buffer values them on runtime
		CurAmpScale = AmplitudeScale - 1.f;
		CurDecayScale = DecayScale - 1.f;
		CurFreqScale = FreqScale - 1.f;
		
		if(ModalsParams.IsValid())
		{
			TArrayView<const float> Params = ModalsParams->GetParams();
			if(Params.Num() % FModalSynth::NumParamsPerModal == 0)
			{
				Seed = InSeed < 0 ? FMath::Rand() : InSeed;
				RandomStream = FRandomStream(Seed);
				InitBuffers(Params, NumUsedModals);

				AmpImpDurationScale =  1e-2f / FMath::Sqrt(ForceSpawnParams.ImpactDuration);
				ImpactStrength = GetImpactStrengthRand(ForceSpawnParams);
				ImpactDecayDuration = ForceSpawnParams.ImpactDuration / 2.0f;
				if(ImpactStrength > 1e-5f)
				{
					ImpactCenterTime = ImpactDecayDuration;
				}
				else //Set to not generate force if strength is nearly zero to avoid pop
				{
					ImpactCenterTime = -ForceSpawnParams.ImpactDuration;
				}
				return;
			}
		}
		
		UE_LOG(LogImpactSFXSynth, Error, TEXT("FImpactForceSynth: Input modal is incorrect!"));		
	}

	void FImpactForceSynth::InitBuffers(const TArrayView<const float>& ModalsParams, const int32 NumUsedModals)
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
	}

	bool FImpactForceSynth::Synthesize(FMultichannelBufferView& OutAudio, const FImpactForceSpawnParams& ForceSpawnParams,
	                                   const FImpactModalObjAssetProxyPtr& ModalsParams,	
	                                   const float AmplitudeScale, const float DecayScale, const float FreqScale,
	                                   const bool bIsAutoStop, bool bClampOutput, bool bIsForceTrigger)
	{
		if(OutL1Buffer.Num() <= 0)
			return true;
		
		if(NumOutChannel != OutAudio.Num())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FImpactForceSynth::Synthesize: the number of output channel is changed while running!"));
			return true;
		}

		const int32 NumOutputFrames = Audio::GetMultichannelBufferNumFrames(OutAudio);
		if(NumOutputFrames <= 0)
			return bIsAutoStop;
		
		FrameTime = NumOutputFrames * TimeStep;
		
		if(ModalsParams->IsParamChanged() 
			|| !FMath::IsNearlyEqual(AmplitudeScale, CurAmpScale, 1e-2)
			|| !FMath::IsNearlyEqual(DecayScale, CurDecayScale, 1e-2)
			|| !FMath::IsNearlyEqual(FreqScale, CurFreqScale, 1e-2))
		{
			InitPreCalBuffers(ModalsParams->GetParams(), AmplitudeScale, DecayScale, FreqScale);
		}
		
		const bool bCanSpawnNewImpact = bIsForceTrigger || ForceSpawnParams.IsCanSpawnNewImpact(CurrentTime);
		if(bCanSpawnNewImpact)
			SpawnNewImpact(ForceSpawnParams, bIsForceTrigger);
		
		const int32 NumModals =  TwoDecayCosBuffer.Num();

		if(FMath::IsNearlyZero(CurAmpScale, 1e-5f))
		{
			FMemory::Memzero(OutL1Buffer.GetData(), OutL1Buffer.Num() * sizeof(float));
			FMemory::Memzero(OutL2Buffer.GetData(), OutL2Buffer.Num() * sizeof(float));
			if(NumOutChannel > 1)
			{
				FMemory::Memzero(OutR1Buffer.GetData(), OutR1Buffer.Num() * sizeof(float));
				FMemory::Memzero(OutR2Buffer.GetData(), OutR2Buffer.Num() * sizeof(float));
			}
			CurrentTime += FrameTime;
			return bIsAutoStop && !bCanSpawnNewImpact;
		}
		
		if(CurrentTime - ImpactCenterTime > ImpactDecayDuration && CurrentForceStrength < STRENGTH_MIN)
		{
			const float* OutL1BufferPtr = OutL1Buffer.GetData();
			const float* OutL2BufferPtr = OutL2Buffer.GetData();
			
			VectorRegister4Float SumModalVector = VectorZeroFloat();
			for(int j = 0; j < NumModals; j+= AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
			{
				SumModalVector = VectorAdd(VectorAbs(VectorLoadAligned(&OutL1BufferPtr[j])), SumModalVector);
				SumModalVector = VectorAdd(VectorAbs(VectorLoadAligned(&OutL2BufferPtr[j])), SumModalVector);
			}

			float SumVal[4];
			VectorStore(SumModalVector, SumVal);
			const float AbsSum = SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
			if(AbsSum < STRENGTH_MIN)
			{
				CurrentTime += FrameTime;
				return bIsAutoStop && !bCanSpawnNewImpact;
			}
		}
		
		StartSynthesizing(OutAudio, NumOutputFrames, NumModals, ForceSpawnParams);

		if(bClampOutput)
		{
			for(int32 Channel = 0; Channel < NumOutChannel; Channel++)
			{
				Audio::ArrayClampInPlace(OutAudio[Channel], -1.f, 1.f);
			}
		}
		
		return false;
	}

	void FImpactForceSynth::InitPreCalBuffers(const TArrayView<const float>& ModalsParams, const float AmplitudeScale, const float DecayScale, const float FreqScale)
	{
		NumUsedParams = FMath::Min(NumUsedParams, ModalsParams.Num());		
		//Scale all amplitude down based on impact duration to keep the synthesizing signals in range
		const float TrueAmpScale = AmplitudeScale * AmpImpDurationScale; 
		for(int i = 0, j = 0; i < NumUsedParams; i++, j++)
		{
			const float Amp = FMath::Clamp(TrueAmpScale * ModalsParams[i], -1.f, 1.f);
			i++;
			const float Decay = DecayScale * ModalsParams[i];
			i++;
			const float Freq = FMath::Clamp(FreqScale * ModalsParams[i], FModalSynth::FMin, FModalSynth::FMax);

			const float Angle = UE_TWO_PI * Freq * TimeStep;
			const float DecayRate = FMath::Exp(-Decay * TimeStep);
			TwoDecayCosBuffer[j] = 2.f * DecayRate * FMath::Cos(Angle);
			DecaySqrBuffer[j] = DecayRate * DecayRate;
			ForceGainBuffer[j] = Amp * DecayRate * FMath::Sin(Angle);
		}
		
		CurAmpScale = AmplitudeScale;
		CurDecayScale = DecayScale;
		CurFreqScale = FreqScale;
	}
	
	void FImpactForceSynth::StartSynthesizing(FMultichannelBufferView& OutAudio, const int32 NumOutputFrames, const int32 NumModals, const FImpactForceSpawnParams& ForceSpawnParams)
	{
		const float* TwoRCosData = TwoDecayCosBuffer.GetData();
		const float* R2Data = DecaySqrBuffer.GetData();
		const float* GainFData = ForceGainBuffer.GetData();
		float* OutL1BufferPtr = OutL1Buffer.GetData();
		float* OutL2BufferPtr = OutL2Buffer.GetData();
		const bool isStartDecaying = (CurrentForceStrength < STRENGTH_MIN) && (CurrentTime - ImpactCenterTime > ImpactDecayDuration);
		if(NumOutChannel < 2)
		{
			TArrayView<float> OutBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
			if(isStartDecaying)
			{
				CurrentForceStrength = 0.f;
				CurrentTime += FrameTime;
				VectorRegister4Float ThresholdReg = VectorSet(1e-5f, 1e-5f, 1e-5f, 1e-5f);
				for(int i = 0; i < NumOutputFrames; i++)
				{
					VectorRegister4Float SumModalVector = VectorZeroFloat();
					float SumVal[4];
					for(int j = 0; j < NumModals; j += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
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
					
						SumModalVector = VectorAdd(SumModalVector, y1);
						VectorStore(y1, &OutL1BufferPtr[j]);
					}
										
					VectorStore(SumModalVector, SumVal);
					const float SampleSum = SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
				
					OutBuffer[i] = SampleSum;
				}
			}
			else
			{
				for(int i = 0; i < NumOutputFrames; i++)
				{
					if(FrameTime > ForceSpawnParams.SpawnInterval && ForceSpawnParams.IsCanSpawnNewImpact(CurrentTime))
						SpawnNewImpact(ForceSpawnParams, false);
					
					UpdateCurrentForceStrength();
				
					const float ForceRand = CurrentForceStrength * (RandomStream.FRand() - 0.5f);
					VectorRegister4Float ForceReg = VectorLoadFloat1(&ForceRand);
					VectorRegister4Float SumModalVector = VectorZeroFloat();
					for(int j = 0; j < NumModals; j += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
					{
						VectorRegister4Float y1 = VectorLoadAligned(&OutL1BufferPtr[j]);
						VectorRegister4Float y2 = VectorLoadAligned(&OutL2BufferPtr[j]);

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
		}
		else
		{
			float* OutR1BufferPtr = OutR1Buffer.GetData();
			float* OutR2BufferPtr = OutR2Buffer.GetData();

			TArrayView<float> LeftChannelBuffer = TArrayView<float>(OutAudio[0].GetData(), NumOutputFrames);
			TArrayView<float> RightChannelBuffer = TArrayView<float>(OutAudio[1].GetData(), NumOutputFrames);
			if(isStartDecaying)
			{
				CurrentForceStrength = 0.f;
				CurrentTime += FrameTime;
				VectorRegister4Float ThresholdReg = VectorSet(1e-5f, 1e-5f, 1e-5f, 1e-5f);
				for(int i = 0; i < NumOutputFrames; i++)
				{
					VectorRegister4Float LeftSumVector = VectorZeroFloat();
					VectorRegister4Float RightSumVector = VectorZeroFloat();
					
					float SumVal[4];
					for(int j = 0; j < NumModals; j += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
					{						
						VectorRegister4Float TwoRCos =VectorLoadAligned(&TwoRCosData[j]);
						VectorRegister4Float R2 = VectorLoadAligned(&R2Data[j]);
					
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
						RightSumVector = VectorAdd(RightSumVector, y1);
						VectorStore(y1, &OutR1BufferPtr[j]);	
					}
					
					VectorStore(LeftSumVector, SumVal);
					LeftChannelBuffer[i] =  SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
				
					VectorStore(RightSumVector, SumVal);
					RightChannelBuffer[i] =  SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3];
				}
			}
			else
			{
				for(int i = 0; i < NumOutputFrames; i++)
				{
					if(FrameTime > ForceSpawnParams.SpawnInterval && ForceSpawnParams.IsCanSpawnNewImpact(CurrentTime))
						SpawnNewImpact(ForceSpawnParams, false);
					
					UpdateCurrentForceStrength();

					const float LeftRand = RandomStream.FRand();
					const float LeftForceRand = CurrentForceStrength * (LeftRand - 0.5f);
					const float RandSign = FMath::Sign(RandomStream.FRand() - 0.5f);
					const float RightRand = FMath::Clamp(LeftRand + RandSign * LeftRand * RandomStream.FRand() * 0.5f, 0.f, 1.f);
					const float RightForceRand = CurrentForceStrength * (RightRand - 0.5f);				
					VectorRegister4Float LeftForceReg = VectorLoadFloat1(&LeftForceRand);
					VectorRegister4Float RightForceReg = VectorLoadFloat1(&RightForceRand);
					VectorRegister4Float LeftSumVector = VectorZeroFloat();
					VectorRegister4Float RightSumVector = VectorZeroFloat();
					for(int j = 0; j < NumModals; j += AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER)
					{
						VectorRegister4Float TwoRCos =VectorLoadAligned(&TwoRCosData[j]);
						VectorRegister4Float R2 = VectorLoadAligned(&R2Data[j]);
						VectorRegister4Float GainF = VectorLoadAligned(&GainFData[j]);
					
						VectorRegister4Float y1 = VectorLoadAligned(&OutL1BufferPtr[j]);
						VectorRegister4Float y2 = VectorLoadAligned(&OutL2BufferPtr[j]);
						VectorStore(y1, &OutL2BufferPtr[j]);
						y1 = VectorMultiply(TwoRCos, y1);
						y2 = VectorMultiply(R2, y2);
						y1 = VectorSubtract(y1, y2);
						y1 = VectorMultiplyAdd(GainF, LeftForceReg, y1);
						LeftSumVector = VectorAdd(LeftSumVector, y1);
						VectorStore(y1, &OutL1BufferPtr[j]);

						y1 = VectorLoadAligned(&OutR1BufferPtr[j]);
						y2 = VectorLoadAligned(&OutR2BufferPtr[j]);
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

	void FImpactForceSynth::SpawnNewImpact(const FImpactForceSpawnParams& ForceSpawnParams, bool bForceTrigger)
	{		
		if(bForceTrigger || (CurrentTime - LastImpactSpawnTime > ForceSpawnParams.SpawnInterval))
		{
			LastImpactSpawnTime = CurrentTime;
			const float SpawnChance = ForceSpawnParams.SpawnChance * FMath::Exp(-ForceSpawnParams.SpawnChanceDecayRate * CurrentTime);
			if(RandomStream.FRand() <= SpawnChance)
			{
				LastForceStrength = CurrentForceStrength;
				LastForceDecayRate = ImpactStrength * TimeStep / ImpactDecayDuration;
				LastImpactCenterTime = ImpactCenterTime;
				ImpactStrength = GetImpactStrengthRand(ForceSpawnParams);
				ImpactDecayDuration = ForceSpawnParams.ImpactDuration * FMath::Pow(ImpactStrength, -ForceSpawnParams.StrengthDurationFactor) / 2.0f;
				ImpactCenterTime = CurrentTime + ImpactDecayDuration;
			}
		}
	}

	void FImpactForceSynth::UpdateCurrentForceStrength()
	{
		const float DeltaTime = CurrentTime - ImpactCenterTime;
		CurrentForceStrength = DeltaTime > ImpactDecayDuration ? 0.f
								   : ImpactStrength * (1.0f + FMath::Cos(UE_PI * DeltaTime / ImpactDecayDuration)); 
			
		if(bNoForceReset && LastForceStrength > 1e-3f)
		{
			CurrentForceStrength = FMath::Min(ImpactStrength, CurrentForceStrength + LastForceStrength);
			const float LastDeltaTime = CurrentTime - LastImpactCenterTime;
			LastForceStrength = LastDeltaTime > 0.f ? (LastForceStrength - LastForceDecayRate)
									: LastForceStrength + LastForceDecayRate; 
		}
			
		CurrentTime += TimeStep;
	}

	float FImpactForceSynth::GetImpactStrengthRand(const FImpactForceSpawnParams& ForceSpawnParams) const
	{
		return ForceSpawnParams.StrengthMin + ForceSpawnParams.StrengthRange * RandomStream.FRand();
	}
}

namespace LBSImpactSFXSynth
{
	FImpactExponentForceSynth::FImpactExponentForceSynth(const FImpactModalObjAssetProxyPtr& ModalsParams,
			const float InSamplingRate, const int32 InNumOutChannel, const FImpactForceSpawnParams& ForceSpawnParams,
			const int32 NumUsedModals, const float AmplitudeScale, const float DecayScale, const float FreqScale,
			const int32 InSeed, const bool bInForceReset)
	: FImpactForceSynth(ModalsParams, InSamplingRate, InNumOutChannel, ForceSpawnParams,
						NumUsedModals, AmplitudeScale, DecayScale, FreqScale,
						InSeed, bInForceReset)
	{
		ImpactDecayDuration = ForceSpawnParams.ImpactDuration - TimeStep;
		ImpactCenterTime = ImpactStrength > 1e-5f ? TimeStep : -ForceSpawnParams.ImpactDuration;
		CurrentForceDecayRate = GetDecaySamplingRate(ForceSpawnParams.ImpactDuration, SamplingRate);
		ImpactStrength = ImpactStrength * 2.0f / CurrentForceDecayRate;
		AmpImpDurationScale =  0.1f / FMath::Sqrt(ImpactDecayDuration);
	}

	float FImpactExponentForceSynth::GetDecaySamplingRate(const float Duration, const float SamplingRate, const float MinValue)
	{
		const float DecayRate = FMath::Loge(FMath::Max(MinValue, 1e-7f)) / FMath::Max(Duration, 1e-7f);		
		return FMath::Exp(DecayRate / SamplingRate);
	}
	
	void FImpactExponentForceSynth::SpawnNewImpact(const FImpactForceSpawnParams& ForceSpawnParams, bool bForceTrigger)
	{
		if(bForceTrigger || (CurrentTime - LastImpactSpawnTime > ForceSpawnParams.SpawnInterval))
		{
			LastImpactSpawnTime = CurrentTime;
			const float SpawnChance = ForceSpawnParams.SpawnChance * FMath::Exp(-ForceSpawnParams.SpawnChanceDecayRate * CurrentTime);
			if(RandomStream.FRand() <= SpawnChance)
			{
				LastForceStrength = ImpactStrength * static_cast<int32>(bNoForceReset);
				LastForceDecayRate = CurrentForceDecayRate;
				LastImpactCenterTime = ImpactCenterTime;
				
				CurrentForceDecayRate = GetDecaySamplingRate(ForceSpawnParams.ImpactDuration, SamplingRate);
				// Divide here will cancel out with multiplier in UpdateCurrentForceStrength() for the first frame
				ImpactStrength = GetImpactStrengthRand(ForceSpawnParams) * 2.0 / CurrentForceDecayRate; 
				ImpactDecayDuration = ForceSpawnParams.ImpactDuration - TimeStep;
				ImpactCenterTime = CurrentTime + TimeStep;
			}
		}
	}

	void FImpactExponentForceSynth::UpdateCurrentForceStrength()
	{
		ImpactStrength *= CurrentForceDecayRate;
		LastForceStrength *= LastForceDecayRate; 
		
		CurrentForceStrength =  ImpactStrength + LastForceStrength;
		CurrentTime += TimeStep;
	}
}

#undef STRENGTH_MIN
