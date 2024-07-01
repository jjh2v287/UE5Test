// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ModalReverb.h"
#include "ImpactSFXSynthLog.h"
#include "ResidualData.h"

#define STRENGTH_MIN (0.001f)

namespace LBSImpactSFXSynth
{
	FModalReverb::FModalReverb(const float InSamplingRate, const FModalReverbParams& InReverbParams,
							   const int32 NumModalsLow, const int32 NumModalsMid, const int32 NumModalsHigh,
	                           const float InAbsorption, const float InRoomSize, const float InOpenFactor, const float InGain)
	: SamplingRate(InSamplingRate), LastInAudioSample(0.f)
	{
		ReverbParams = InReverbParams;
		SetEnvCoefs(InAbsorption, InRoomSize, InOpenFactor, InGain);
		
		TimeStep = 1.0f / SamplingRate;
		
		InitBuffers(NumModalsLow, NumModalsMid, NumModalsHigh);
	}

	void FModalReverb::SetEnvCoefs(const float InAbsorption, const float InRoomSize, const float InOpenRoomFactor, const float InGain)
	{
		const float Absorption = FMath::Max(0.f, InAbsorption);
		const float RoomSize = FMath::Clamp(InRoomSize, 100.f, MaxRoomSize);
		const float OpenFactor = FMath::Clamp(InOpenRoomFactor, 1.f, MaxOpenRoomFactor);
		const float OpenFactorScale = FMath::Sqrt(OpenFactor);
		//Gain is upscale by 1000 to avoid small number round off in modal calculation
		//So at the output we multiply with this value to scale it back to the correct value
		const float GainReduction = FMath::Max(1.f - RoomSize / 10000.f - OpenFactorScale / 20.f, 0.1f);
		AmpScale = InGain * GainReduction * 0.1875e-3f;
		
		BaseDecay = Absorption + 5000.0f * FMath::Sqrt(OpenFactor) / RoomSize;

		//An outdoor environment won't have long decay times, the effect is similar to clipping
		//So this will be used to limit the minimum decay in our calculation
		OutDoorMinDecay = OpenFactor;
	}

	void FModalReverb::InitBuffers(const int32 NumModalsLow, const int32 NumModalsMid, const int32 NumModalsHigh)
	{
		MaxNumModals = FMath::Clamp(NumModalsLow + NumModalsMid + NumModalsHigh, AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER, MaxSupportNumModals);
		//Make sure NumModals is a multiplier of Vector register so all modals can run in SIMD when synthesizing
		const int32 RemVec = MaxNumModals % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER;
		if(RemVec > 0)
			MaxNumModals = MaxNumModals + (AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER - RemVec);
		CurrentNumModals = MaxNumModals;
		const int32 ClearSize = MaxNumModals * sizeof(float);
		
		OutL1Buffer.SetNumUninitialized(MaxNumModals);
		FMemory::Memzero(OutL1Buffer.GetData(), ClearSize);
		OutL2Buffer.SetNumUninitialized(MaxNumModals);
		FMemory::Memzero(OutL2Buffer.GetData(), ClearSize);

		FreqBuffer.SetNumUninitialized(MaxNumModals);
		PhaseBuffer.SetNumUninitialized(MaxNumModals);
		GainBuffer.SetNumUninitialized(MaxNumModals);
		TwoDecayCosBuffer.SetNumUninitialized(MaxNumModals);
		DecaySqrBuffer.SetNumUninitialized(MaxNumModals);
		Activation1DBuffer.SetNumUninitialized(MaxNumModals);
		ActivationBuffer.SetNumUninitialized(MaxNumModals);
		
		InitBufferBand(LowBandFMin, LowBandFMax, NumModalsLow, 0, 1.0f);
		InitBufferBand(MidBandFMin, MidBandFMax, NumModalsMid, NumModalsLow, 1.0f);
		const int32 BufferIndex = NumModalsLow + NumModalsMid;
		//Due to its wide frequency range, need to double the high band gain to balance its power. 
		InitBufferBand(HighBandFMin, HighBandFMax, MaxNumModals - BufferIndex, BufferIndex, 2.0f);
	}

	void FModalReverb::InitBufferBand(const float FMin, const float FMax, const int32 NumModals,
		const float BufferIndex, const float GainScale)
	{
		const float ErbMin = UResidualData::Freq2Erb(FMin);
		const float ErbMax = UResidualData::Freq2Erb(FMax);
		const float ErbStep = (ErbMax - ErbMin) / (FMath::Max(1, NumModals - 1));
		
		const float PiTime = UE_TWO_PI * TimeStep;
		const int32 EndIdx = NumModals + BufferIndex;
		for(int i = BufferIndex, step = 0; i < EndIdx; i++, step++)
		{
			const float Freq = UResidualData::Erb2Freq(step * ErbStep + ErbMin);
			const float FreqScale = Freq / 1000.f;
			const float FreqScaleSqr = FreqScale * FreqScale;
			const float Decay = GetDecayFromFreqScale(FreqScale, FreqScaleSqr);
			const float Amp = GetAmpFromFreqScale(FreqScale, FreqScaleSqr) * GainScale;

			const float Angle = PiTime * Freq;
			
			// const float Phi = FMath::FRand() * UE_TWO_PI;
			const float Phi = Freq * PiTime * 2.0f;
			
			const float DecayRate = FMath::Exp(-Decay * TimeStep);
			FreqBuffer[i] = Freq;
			PhaseBuffer[i] = Phi;
			TwoDecayCosBuffer[i] = 2.f * DecayRate * FMath::Cos(Angle);
			GainBuffer[i] = Amp;
			DecaySqrBuffer[i] = DecayRate * DecayRate;
			Activation1DBuffer[i] = Amp * DecayRate * FMath::Sin(Angle - Phi);
			ActivationBuffer[i] = Amp * FMath::Sin(Phi);
		}
	}

	float FModalReverb::GetDecayFromFreqScale(const float FreqScale, const float FreqScaleSqr) const
	{
		const float NewDecay = BaseDecay + FreqScale * ReverbParams.DecaySlopeB + FreqScaleSqr * ReverbParams.DecaySlopeC;
		return FMath::Max3(NewDecay, ReverbParams.DecayMin, OutDoorMinDecay);
	}
	
	float FModalReverb::GetAmpFromFreqScale(const float FreqScale, const float FreqScaleSqr) const
	{
		// return 2.6f + FreqScale * 0.1f + FreqScaleSqr * 1.0e-2f;
		return 2.6f - FreqScale * 0.2f + FreqScaleSqr * 3e-2f;
	}
	
	bool FModalReverb::CreateReverb(TArrayView<float>& OutAudio, const TArrayView<const float>& InAudio,
									const float InAbsorption, const float InRoomSize, const float InOpenRoomFactor,
									const float InGain, bool bIsInAudioStop)
	{
		if(OutL1Buffer.Num() <= 0 || InAudio.Num() <= 0)
			return true;

		if(InAbsorption >= NoReverbAbsorption)
			//High absorption values = No reverb
			//If this is the first frame -> this will turn off immediately
			//else the reverb is gradually faded out thanks to high decay values
			bIsInAudioStop = true; 

		const int32 NumRequestFrames = OutAudio.Num();
		const int32 NumInAudioFrame = InAudio.Num();
		if(OutAudio.Num() > InAudio.Num())
			UE_LOG(LogImpactSFXSynth, Warning, TEXT("FModalReverb::Synthesize: The number of requested frames != the number of in audio frames!"));
		
		const int32 NumOutputFrames = FMath::Min(NumRequestFrames, NumInAudioFrame);
		if(NumOutputFrames <= 0)
			return bIsInAudioStop;

		SetEnvCoefs(InAbsorption, InRoomSize, InOpenRoomFactor, InGain);
		ScatterFreqAndPhase();
		GetNumUsedModals(bIsInAudioStop);
		if(CurrentNumModals <= 0)
			return true;
		
		ProcessAudio(OutAudio, InAudio, NumOutputFrames);
		return false;
	}

	void FModalReverb::GetNumUsedModals(const bool bIsForceStop)
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
				const float SampleSum = (SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3]) * AmpScale;
				if(SampleSum > STRENGTH_MIN)
					break;
			}
			CurrentNumModals = j + AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER;
			return;
		}
		
		CurrentNumModals = TwoDecayCosBuffer.Num(); 
	}

	void FModalReverb::ScatterFreqAndPhase()
	{
		const float PiTime = UE_TWO_PI * TimeStep;
		for(int i = 0; i < CurrentNumModals; i += 1)
		{
			const float F0 = FreqBuffer[i];
			const float Freq = F0 * (1.0f + (FMath::FRand() - 0.5f) * ReverbParams.FreqScatterScale);
			const float FreqScale = Freq / 1000.f;
			const float FreqScaleSqr = FreqScale * FreqScale;
			const float Decay = GetDecayFromFreqScale(FreqScale, FreqScaleSqr);
			const float Phi = PhaseBuffer[i] + FMath::FRand() * UE_PI * ReverbParams.PhaseScatterScale;
			const float Angle = PiTime * Freq;
			const float DecayRate = FMath::Exp(-Decay * TimeStep);

			TwoDecayCosBuffer[i] = 2.0f * DecayRate * FMath::Cos(Angle);
			Activation1DBuffer[i] = GainBuffer[i] * DecayRate * FMath::Sin(Angle - Phi);
			ActivationBuffer[i] = GainBuffer[i] * FMath::Sin(Phi);
			DecaySqrBuffer[i] = DecayRate * DecayRate;
		}
	}

	void FModalReverb::ProcessAudio(TArrayView<float>& OutAudio, const TArrayView<const float>& InAudio,
	                                     const int32 NumOutputFrames)
	{
		const float* TwoRCosData = TwoDecayCosBuffer.GetData();
		const float* R2Data = DecaySqrBuffer.GetData();
		const float* GainFData = Activation1DBuffer.GetData();
		const float* GainCData = ActivationBuffer.GetData();
		float* OutL1BufferPtr = OutL1Buffer.GetData();
		float* OutL2BufferPtr = OutL2Buffer.GetData();

		const VectorRegister4Float ThresholdReg = VectorSet(1e-5f, 1e-5f, 1e-5f, 1e-5f);

		{ // First frame need to use the last in audio sample
			const VectorRegister4Float InAudio1DReg = VectorLoadFloat1(&LastInAudioSample);
			const VectorRegister4Float InAudioReg = VectorLoadFloat1(&InAudio[0]);
			OutAudio[0] = ProcessOneSample(TwoRCosData, R2Data, GainFData, GainCData,
											OutL1BufferPtr, OutL2BufferPtr, ThresholdReg,
											InAudio1DReg, InAudioReg);
		}
		
		for(int i = 1; i < NumOutputFrames; i++)
		{
			const VectorRegister4Float InAudio1DReg = VectorLoadFloat1(&InAudio[i-1]);
			const VectorRegister4Float InAudioReg = VectorLoadFloat1(&InAudio[i]);
			OutAudio[i] = ProcessOneSample(TwoRCosData, R2Data, GainFData, GainCData,
											OutL1BufferPtr, OutL2BufferPtr, ThresholdReg,
											InAudio1DReg, InAudioReg);
		}
		
		LastInAudioSample = InAudio[InAudio.Num() - 1];
	}

	float FModalReverb::ProcessOneSample(const float* TwoRCosData, const float* R2Data, const float* GainFData,
										const float* GainCData, float* OutL1BufferPtr, float* OutL2BufferPtr,
										const VectorRegister4Float& ThresholdReg, const VectorRegister4Float& InAudio1DReg,
										const VectorRegister4Float& InAudioReg) const
	{
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
			y1 = VectorMultiplyAdd(VectorLoadAligned(&GainFData[j]), InAudio1DReg, y1);
			y1 = VectorMultiplyAdd(VectorLoadAligned(&GainCData[j]), InAudioReg, y1);
				
			SumModalVector = VectorAdd(SumModalVector, y1);
			VectorStore(y1, &OutL1BufferPtr[j]);
		}
			
		float SumVal[4];
		VectorStore(SumModalVector, SumVal);
		//AmpScale here instead of directly in Gain to avoid small number round off
		return (SumVal[0] + SumVal[1] + SumVal[2] + SumVal[3]) * AmpScale;
	}
}

#undef STRENGTH_MIN