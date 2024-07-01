// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "HadamardDiffusion.h"

#include "ImpactSFXSynthLog.h"

namespace LBSImpactSFXSynth
{
	FHadamardDiffusion::FHadamardDiffusion(float InSamplingRate, int32 InNumStages, float InMaxDelay, bool bDoubleDelayEachStage)
		: SamplingRate(InSamplingRate) 
	{
		NumStages = FMath::Max(1, InNumStages);
		MaxDelay = FMath::Max(InMaxDelay, 0.001f);
		NumBuffers = NumChannels * NumStages;
		DelayBuffers.Empty(NumBuffers);
		ChannelWriteIndex.SetNumUninitialized(NumBuffers);
		
		const int32 DelayScale = bDoubleDelayEachStage ? 2 : 1;
		const int32 MaxDelaySample = FMath::RoundToInt32(SamplingRate * MaxDelay);
		const int32 MinDelaySample = FMath::RoundToInt32(SamplingRate * 0.001f);
		
		for(int i = 0; i < NumStages; i++)
		{
			const int32 MaxChannelDelay = MaxDelaySample * DelayScale / NumChannels;
			const int32 MinChannelDelay = FMath::Max(MinDelaySample, MaxChannelDelay / 3);
			for(int j = 0; j < NumChannels; j++)
			{
				const int32 NumDelaySample = FMath::RandRange(MinChannelDelay, MaxChannelDelay); 
				FAlignedFloatBuffer Buffer = FAlignedFloatBuffer();
				Buffer.SetNumUninitialized(NumDelaySample);
				DelayBuffers.Emplace(Buffer);
			}
		}
		ResetBuffers();
	}

	void FHadamardDiffusion::ResetBuffers()
	{
		for(int i = 0; i < DelayBuffers.Num(); i++)
		{
			FMemory::Memzero(DelayBuffers[i].GetData(), DelayBuffers[i].Num() * sizeof(float));
			ChannelWriteIndex[i] = 0;
		}
	}

	void FHadamardDiffusion::ProcessAudio(FMultichannelBufferView& OutAudio, const TArrayView<const float>& InAudio)
	{
		if(OutAudio.Num() != NumChannels)
		{
			UE_LOG(LogImpactSFXSynth, Error, TEXT("FHadamardDiffusion::ProcessAudio: the number of output channels is incorrect!"));
			return;
		}
		
		const int32 NumOutputFrames = GetMultichannelBufferNumFrames(OutAudio);
		for(int i = 0; i < NumOutputFrames; i++)
		{
			int32 CurStageIdx = NumChannels * (NumStages - 1);
			int32 SecondStageIdx = CurStageIdx + 1;
			int32 ThirdStageIdx = CurStageIdx + 2;
			int32 FourthStageIdx = CurStageIdx + 3;
			
			int32 PrevStageIdx = NumChannels * (NumStages - 2);
			
			float FirstChannel = DelayBuffers[CurStageIdx][ChannelWriteIndex[CurStageIdx]];
			float SecondChannel = -DelayBuffers[SecondStageIdx][ChannelWriteIndex[SecondStageIdx]];
			float ThirdChannel = DelayBuffers[ThirdStageIdx][ChannelWriteIndex[ThirdStageIdx]];
			float FourChannel = -DelayBuffers[FourthStageIdx][ChannelWriteIndex[FourthStageIdx]];
			OutAudio[0][i] = FirstChannel + SecondChannel + ThirdChannel + FourChannel;
			OutAudio[1][i] = FirstChannel - SecondChannel + ThirdChannel - FourChannel;
			OutAudio[2][i] = FirstChannel + SecondChannel - ThirdChannel - FourChannel;
			OutAudio[3][i] = FirstChannel - SecondChannel - ThirdChannel + FourChannel;
	
			while(PrevStageIdx > 0)
			{
				const int32 PrevSecondStageIdx = PrevStageIdx + 1;
                const int32 PrevThirdStageIdx = PrevStageIdx + 2;
                const int32 PrevFourthStageIdx = PrevStageIdx + 3;
                			
				FirstChannel = DelayBuffers[PrevStageIdx][ChannelWriteIndex[PrevStageIdx]];
				SecondChannel = -DelayBuffers[PrevSecondStageIdx][ChannelWriteIndex[PrevSecondStageIdx]];
				ThirdChannel = DelayBuffers[PrevThirdStageIdx][ChannelWriteIndex[PrevThirdStageIdx]];
				FourChannel = -DelayBuffers[PrevFourthStageIdx][ChannelWriteIndex[PrevFourthStageIdx]];
				
				DelayBuffers[CurStageIdx][ChannelWriteIndex[CurStageIdx]] = FirstChannel + SecondChannel + ThirdChannel + FourChannel;
				DelayBuffers[SecondStageIdx][ChannelWriteIndex[SecondStageIdx]] = FirstChannel - SecondChannel + ThirdChannel - FourChannel;
				DelayBuffers[ThirdStageIdx][ChannelWriteIndex[ThirdStageIdx]] = FirstChannel + SecondChannel - ThirdChannel - FourChannel;
				DelayBuffers[FourthStageIdx][ChannelWriteIndex[FourthStageIdx]] = FirstChannel - SecondChannel - ThirdChannel + FourChannel;
	
				CurStageIdx -= NumChannels;
				SecondStageIdx = CurStageIdx + 1;
				ThirdStageIdx = CurStageIdx + 2;
				FourthStageIdx = CurStageIdx + 3;
				
				PrevStageIdx -= NumChannels;
			}
			
			DelayBuffers[FourthStageIdx][ChannelWriteIndex[FourthStageIdx]] = ThirdChannel;
			DelayBuffers[ThirdStageIdx][ChannelWriteIndex[ThirdStageIdx]] = -SecondChannel;
			DelayBuffers[SecondStageIdx][ChannelWriteIndex[SecondStageIdx]] = FirstChannel;
			DelayBuffers[CurStageIdx][ChannelWriteIndex[CurStageIdx]] = InAudio[i];

			for(int j = 0; j < NumBuffers; j++)
			{
				ChannelWriteIndex[j] = (ChannelWriteIndex[j] + 1) % DelayBuffers[j].Num();
			}
		}
	}

	
}
