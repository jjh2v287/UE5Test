// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "MultiDelayReverbMix.h"

#include "ImpactSFXSynthLog.h"

namespace LBSImpactSFXSynth
{
	
	FMultiDelayReverbMix::FMultiDelayReverbMix(float InSamplingRate, float InGain, float InMinDelay, float InMaxDelay)
	: SamplingRate(InSamplingRate)
	{
		SetDecayGain(InGain);
		const int32 MinDelaySample = FMath::FloorToInt32(InMinDelay * InSamplingRate);
		const int32 MaxDelaySample = FMath::CeilToInt32(InMaxDelay * InSamplingRate);
		const int32 DelayStep = (MaxDelaySample - MinDelaySample) / NumChannels;
		ChannelDelaySample.Empty(NumChannels);
		DelayBuffers.Empty(NumChannels);
		ChannelWriteIndex.Empty(NumChannels);
		
		ChannelDelaySample.Add(MinDelaySample);
		FAlignedFloatBuffer FirstBuffer = FAlignedFloatBuffer();
		FirstBuffer.SetNumUninitialized(MinDelaySample);
		DelayBuffers.Emplace(FirstBuffer);
		ChannelWriteIndex.SetNumUninitialized(NumChannels);
		
		for (int i = 1; i < NumChannels; i++)
		{
			const int32 CurDelaySample = DelayStep * i + MinDelaySample;
			const int32 DelaySample = FMath::RandRange(CurDelaySample, CurDelaySample + DelayStep);

			ChannelDelaySample.Add(DelaySample);
			FAlignedFloatBuffer Buffer = FAlignedFloatBuffer();
			Buffer.SetNumUninitialized(DelaySample);
			DelayBuffers.Emplace(Buffer);
		}

		ResetBuffers();
	}

	void FMultiDelayReverbMix::SetDecayGain(float InGain)
	{
		DecayGain = FMath::Min(InGain / 2.0f, 0.49f);
	}

	void FMultiDelayReverbMix::ResetBuffers()
	{
		for(int i = 0; i < NumChannels; i++)
		{
			const int32 DelayS = ChannelDelaySample[i];
			FMemory::Memzero(DelayBuffers[i].GetData(), DelayS * sizeof(float));
			ChannelWriteIndex[i] = 0;
		}
	}
	
	void FMultiDelayReverbMix::ProcessAudio(TArrayView<float>& OutAudio, const TArrayView<const float>& InAudio, const float FeedbackGain)
	{
		if(OutAudio.Num() != InAudio.Num())
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FMultiDelayReverbMix::ProcessAudio: The number of requested frames != the number of input audio frames!"));
			return;
		}
		
		const int32 NumOutFrame = OutAudio.Num();
		SetDecayGain(FeedbackGain);
		for(int i = 0; i < NumOutFrame; i++)
		{
			const float InSample = InAudio[i];
			ProcessOneSample(OutAudio, i, InSample);
		}
	}

	void FMultiDelayReverbMix::ProcessOneSample(TArrayView<float>& OutAudio, int i,const float InSample)
	{
		float FirstChannel = DelayBuffers[0][ChannelWriteIndex[0]];
		float SecondChannel = DelayBuffers[1][ChannelWriteIndex[1]];
		float ThirdChannel = DelayBuffers[2][ChannelWriteIndex[2]];
		float FourthChannel = DelayBuffers[3][ChannelWriteIndex[3]];
			
		OutAudio[i] = (FirstChannel + SecondChannel + ThirdChannel + FourthChannel) / 4.f;
			
		FirstChannel = FirstChannel * DecayGain;
		SecondChannel = SecondChannel * DecayGain;
		ThirdChannel = ThirdChannel * DecayGain;
		FourthChannel = FourthChannel * DecayGain;
			
		DelayBuffers[0][ChannelWriteIndex[0]] = FirstChannel - SecondChannel - ThirdChannel - FourthChannel + InSample;
		DelayBuffers[1][ChannelWriteIndex[1]] = -FirstChannel + SecondChannel - ThirdChannel - FourthChannel + InSample;
		DelayBuffers[2][ChannelWriteIndex[2]] = -FirstChannel - SecondChannel + ThirdChannel - FourthChannel + InSample;
		DelayBuffers[3][ChannelWriteIndex[3]] = -FirstChannel - SecondChannel - ThirdChannel + FourthChannel + InSample;

		ChannelWriteIndex[0] = (ChannelWriteIndex[0] + 1) % ChannelDelaySample[0];
		ChannelWriteIndex[1] = (ChannelWriteIndex[1] + 1) % ChannelDelaySample[1];
		ChannelWriteIndex[2] = (ChannelWriteIndex[2] + 1) % ChannelDelaySample[2];
		ChannelWriteIndex[3] = (ChannelWriteIndex[3] + 1) % ChannelDelaySample[3];
	}
}
