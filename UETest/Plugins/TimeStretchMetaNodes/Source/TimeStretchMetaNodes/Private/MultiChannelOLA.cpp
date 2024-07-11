// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "MultiChannelOLA.h"

#include "TimeStretchLog.h"
#include "MultiChannelBufferCustom.h"
#include "DSP/Dsp.h"
#include "DSP/MultichannelBuffer.h"
#include "HAL/PlatformMath.h"
#include "Math/UnrealMathUtility.h"
#include "DSP/FloatArrayMath.h"

namespace LBSTimeStretch
{
	using namespace Audio;

	TMap<int32, FAlignedFloatBuffer> FMultichannelOLA::ClassWindowBuffers;
	
	FMultichannelOLA::FMultichannelOLA(int32 InNumChannels, float InSamplingRate, float InFrameLenghtSec, float InFrameRatio, int32 StartFrameIndex)
		: NumChannels(InNumChannels), SamplingRate(InSamplingRate), FrameLengthSec(InFrameLenghtSec), TargetFrameRatio(InFrameRatio)
	{
		CurrentWindowBuffer = InitWindowBuffer(InSamplingRate, FrameLengthSec);
		WinSize = CurrentWindowBuffer->Num();
		HalfWinSize = CurrentWindowBuffer->Num() / 2; 
		MaxPadLength = HalfWinSize;

		//By rounding here we accept a very small difference in real stretch ratio compared to expected ratio
		//Even with a sampling rate of 8000 and FrameSec = 0.02, delta = 1 / (0.02 * 8000 / 2) = 0.0125, which is small enough
		//for this to be ignored. 
		TargetAnalysisSize = FMath::RoundToInt32(HalfWinSize * TargetFrameRatio);

		CurrentFrameRatio = TargetFrameRatio;
		CurrentAnalysisSize = TargetAnalysisSize;
		CurrentLeftStartOverlapIndexInWave = StartFrameIndex;
		CurrentLeftStartOverlapIndexInBuffer = 0;
		
		LeftOverlapBuffer.SetNumUninitialized(HalfWinSize, false);
		RightOverlapBuffer.SetNumUninitialized(HalfWinSize, false);
	}

	void FMultichannelOLA::SetFrameRatio(float InRatio, const FSourceBufferState& SourceState)
	{
		if(FMath::IsNearlyEqual(TargetFrameRatio, InRatio, EqualErrorTolerance))
		{
			//Nothing change just return
			return;
		}

		TargetFrameRatio = InRatio;
		TargetAnalysisSize = FMath::RoundToInt32(HalfWinSize * InRatio);
	}

	FAlignedFloatBuffer* FMultichannelOLA::InitWindowBuffer(float InSamplingRate, float InFrameLengthSecond)
    {
    	const int32 ExpectWinSize = GetEvenWindowSize(InSamplingRate, InFrameLengthSecond);
		if(ClassWindowBuffers.Contains(ExpectWinSize))
			return &ClassWindowBuffers[ExpectWinSize];

		ClassWindowBuffers.Add(ExpectWinSize);
		ClassWindowBuffers[ExpectWinSize].SetNumUninitialized(ExpectWinSize);
    	FMultichannelOLA::GenerateHannWindow(ClassWindowBuffers[ExpectWinSize].GetData(), ExpectWinSize, true);
    	return &ClassWindowBuffers[ExpectWinSize];
    }

    FAlignedFloatBuffer* FMultichannelOLA::GetCurrentWindowBuffer()
    {
    	if(CurrentWindowBuffer == nullptr)
    		CurrentWindowBuffer = InitWindowBuffer(SamplingRate, FrameLengthSec);
    	
    	return CurrentWindowBuffer;
    }
	
	int32 FMultichannelOLA::GetNumInputFrameToProduceNumOutPutFrame(const FSourceBufferState& SourceState, const int32 InNumOutputFrame, const bool bLoop) const
	{
		if(InNumOutputFrame <= 0)
			return 0;
		
		const int32 EndCurrentLeftOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + HalfWinSize;

		//Use TargetAnalysisSize if start of next overlap
		const int32 AnalysisSize = (CurrentLeftStartOverlapIndexInBuffer == CurrentPadding) ? TargetAnalysisSize : CurrentAnalysisSize;
		const int32 EndCurrentRightOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + AnalysisSize;
		
		//Make sure we always start calculation at complete overlap block
		const int32 NumRemainOutputFrame = (CurrentPadding < EndCurrentLeftOverlapIdx) ? (InNumOutputFrame - (EndCurrentLeftOverlapIdx - CurrentPadding)) : InNumOutputFrame;
		if(NumRemainOutputFrame <= 0)
		{	
			const int32 EndRightOverlapIdx =  EndCurrentRightOverlapIdx - HalfWinSize + (CurrentPadding - CurrentLeftStartOverlapIndexInBuffer) + InNumOutputFrame;
			// Fit into one overlap so the calculation is much simpler
			// CurrentFrameRatio < 1.0f AnalysisStep < HalfWinSize
			return  FMath::Max(EndRightOverlapIdx, CurrentPadding + InNumOutputFrame);
		}
		
		//From here switch to use TargetAnalysisSize as we will update new FrameRatio (if change) on start of next overlap
		
		//StartOffset can be zero for StretchRatio < 1.0. To avoid MaxNumInputNeeded < CurrentPadding set it to always be larger or equal to zero
		const int32 StartOffset = FMath::Max3(0, EndCurrentRightOverlapIdx, EndCurrentLeftOverlapIdx);
		const int32 NumOverlap = NumRemainOutputFrame / HalfWinSize + 1;
		const int32 EndRemain = NumRemainOutputFrame % HalfWinSize;

		const int32 StartRightOverlapIdx = TargetAnalysisSize * NumOverlap - HalfWinSize;
		const int32 StartLeftOverlapIdx = TargetAnalysisSize * (NumOverlap - 1);
		const int32 MaxNumInputNeeded = FMath::Max(StartRightOverlapIdx, StartLeftOverlapIdx) + StartOffset + EndRemain;
		checkf(MaxNumInputNeeded > 0, TEXT("FMultichannelOLA::GetNumInputFrameToProduceNumOutPutFrame negative num of input?"));
		
		return MaxNumInputNeeded;
	}
	
	int32 FMultichannelOLA::GetInputFrameCapacityForMaxFrameRatio(const int32 InNumOutputFrame, const float MaxFrameRatio) const
	{
		if(InNumOutputFrame <= 0)
			return 0;
		
		// +1 since worst case is having an unfinished overlap before finding next overlap
		const int32 NumOverlap = FMath::CeilToInt32(static_cast<float>(InNumOutputFrame) / HalfWinSize) + 1;

		const int32 AnalysisStep = FMath::CeilToInt32(HalfWinSize * MaxFrameRatio);
		const int32 EndRightOverlapIdx = AnalysisStep * NumOverlap;
		const int32 EndLeftOverlapIdx = AnalysisStep * (NumOverlap - 1) + HalfWinSize;
		return  FMath::Max(EndRightOverlapIdx, EndLeftOverlapIdx) + MaxPadLength + HalfWinSize;
	}

	float FMultichannelOLA::MapInputEventToOutputBufferFrame(const FSourceBufferState& SourceState, const int32 InEventIndexInputBuffer, const bool bLoop) const
	{
		checkf(InEventIndexInputBuffer >= -1.f, TEXT("Frame index mapping function is only for inputs frames greater than or equal to -1"));
		if(InEventIndexInputBuffer < 0)
		{
			UE_LOG(LogTimeStretch, Warning, TEXT("FMultichannelOLA::MapInputEventToOutputBufferFrame: InEventIndexInputBuffer is negative: %d"), InEventIndexInputBuffer);
			return -1.f;
		}
		
		const int32 EndLeftOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + HalfWinSize;
		
		if(InEventIndexInputBuffer < EndLeftOverlapIdx)
			return FMath::Max(-1, InEventIndexInputBuffer - CurrentPadding); //-1 mean outside of buffer -> true for input indexes used in padding

		const int32 AnalysisSize = (CurrentLeftStartOverlapIndexInBuffer == CurrentPadding) ? TargetAnalysisSize : CurrentAnalysisSize;
		const int32 NextLeftOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + AnalysisSize;
		
		//To make sure events are fire correctly to their timeline (e.g no 0.3s events are fired before 0.1s events
		//Map any index after EndLeftOverlap to final index of current overlap
		if(InEventIndexInputBuffer < NextLeftOverlapIdx)
			return EndLeftOverlapIdx - CurrentPadding - 1; 
		
		int32 NumOutput = FMath::Max(0, EndLeftOverlapIdx - CurrentPadding);
		
		const int32 RemainNumSample = InEventIndexInputBuffer - NextLeftOverlapIdx;
		int32 RemainNumOverlap = RemainNumSample / TargetAnalysisSize;
		if(TargetFrameRatio < 1.0 && RemainNumOverlap > 0) //Lower than 1 then we need to make sure RemainNumOverlap is smallest
		{
			int32 PrevEndLeftOverlap = RemainNumOverlap * TargetAnalysisSize + HalfWinSize - TargetAnalysisSize;
			while(RemainNumSample < PrevEndLeftOverlap && RemainNumOverlap > 0)
			{
				RemainNumOverlap--;
				PrevEndLeftOverlap -= TargetAnalysisSize;
			}
		}
		
		NumOutput += RemainNumOverlap * HalfWinSize;
		const int32 LastStartLeftOverlap = RemainNumOverlap * TargetAnalysisSize;

		NumOutput += FMath::Min(HalfWinSize - 1, RemainNumSample - LastStartLeftOverlap);
		return NumOutput;
	}

	int32 FMultichannelOLA::GetEvenWindowSize(float InSamplingRate, float InFrameLengthSecond)
	{
		int32 EvenWinSize = static_cast<int32>(InFrameLengthSecond * InSamplingRate);
		EvenWinSize -= EvenWinSize % 2;
		return EvenWinSize;
	}
	
	int32 FMultichannelOLA::GetSafeNumOutputFrames(const FSourceBufferState& SourceState, const bool bLoop, const int32 NumAvailableInputFrames) const
	{
		const int32 NumNewInputData = NumAvailableInputFrames - CurrentPadding;
		if(NumNewInputData <= 0)
			return 0;
		
		const int32 EndCurrentLeftOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + HalfWinSize;
		const int32 NumIncompleteSamples = FMath::Max(EndCurrentLeftOverlapIdx - CurrentPadding, 0);

		// Switch AnalysisSize if start of new overlap 
		const int32 AnalysisSize = (CurrentLeftStartOverlapIndexInBuffer == CurrentPadding) ? TargetAnalysisSize : CurrentAnalysisSize;
		const int32 EndCurrentRightOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + AnalysisSize;
		const int32 StartCurrentRightOverlapIdx = EndCurrentRightOverlapIdx - HalfWinSize;
		
		const int32 CompleteIdx = FMath::Max(EndCurrentRightOverlapIdx, EndCurrentLeftOverlapIdx);
		if(NumAvailableInputFrames <= CompleteIdx)
		{
			const int32 NumRemainSamples = FMath::Min(NumIncompleteSamples, NumNewInputData);
			if(CurrentPadding < EndCurrentLeftOverlapIdx)
			{
				// Is EOF and no loop
				if(!bLoop && NumAvailableInputFrames >= SourceState.GetEOFFrameIndexInBuffer())
				{
					const int32 NumEndFrames = FMath::Min(NumRemainSamples, SourceState.GetEOFFrameIndexInBuffer() - CurrentPadding); 
					return FMath::Max(0, NumEndFrames);
				}

				const int32 StartOffset = CurrentPadding - CurrentLeftStartOverlapIndexInBuffer;
				const int32 NumOut = FMath::Min(NumRemainSamples, NumAvailableInputFrames - (StartCurrentRightOverlapIdx + StartOffset));
				return FMath::Max(0, NumOut);
			}
			
			// In an unused region
			return 0;
		}

		int32 NumOutputFrames = NumIncompleteSamples;
		const int32 NumInputDataCanUse = NumAvailableInputFrames - EndCurrentRightOverlapIdx;  

		int32 NumFullOverlaps = NumInputDataCanUse / TargetAnalysisSize;
		int32 EndLastLeftOverlapIdx = NumFullOverlaps * TargetAnalysisSize + HalfWinSize;
		//For stretch ratio < 1, we might overshoot so make sure EndLastLeftOverlapIdx is smaller than num of samples left
		while (NumFullOverlaps > 0 && EndLastLeftOverlapIdx > NumInputDataCanUse)
		{
			NumFullOverlaps--;
			EndLastLeftOverlapIdx -= TargetAnalysisSize;
		}

		NumOutputFrames += NumFullOverlaps * HalfWinSize;
		const int32 StartLastLeftOverlapIdx = TargetAnalysisSize * NumFullOverlaps;
		const int32 StartLastRightOverlapIdx = StartLastLeftOverlapIdx + TargetAnalysisSize - HalfWinSize;
		
		// If not looping and reaching EOF we allow final remain overlapping frame to be copy directly from input
		// thus reduce number of samples needed
		if(!bLoop && NumAvailableInputFrames >= SourceState.GetEOFFrameIndexInBuffer())			
			NumOutputFrames += FMath::Min(StartLastLeftOverlapIdx + HalfWinSize, NumInputDataCanUse) - StartLastLeftOverlapIdx;
		else
		{
			const int32 NumRemain = FMath::Min3(NumInputDataCanUse - StartLastLeftOverlapIdx, NumInputDataCanUse - StartLastRightOverlapIdx, HalfWinSize);
			NumOutputFrames += FMath::Max(0, NumRemain);
		}
		
		return NumOutputFrames;
	}
	
	int32 FMultichannelOLA::ProcessAndConsumeAudio(MultiChannelBufferCustom::FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio, const FSourceBufferState& SourceState, const bool bLoop)
	{
		return ProcessAndConsumeAudioInternal(InAudio, OutAudio, SourceState, bLoop);
	}

	void FMultichannelOLA::GenerateHannWindow(float* WindowBuffer, const int32 NumFrames, const bool bIsPeriodic)
	{
		const int32 N = bIsPeriodic ? NumFrames : NumFrames - 1;
		const float PhaseDelta = 2.0f * PI / N;
		float Phase = 0.0f;
		
		for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex++)
		{
			const float Value = 0.5f * (1 - FMath::Cos(Phase));
			Phase += PhaseDelta;
			WindowBuffer[FrameIndex] = Value;
		}
	}
	
	int32 FMultichannelOLA::ProcessAndConsumeAudioInternal(FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio, const FSourceBufferState& SourceState, const bool bLoop)
	{
		checkf(InAudio.Num() == OutAudio.Num(), TEXT("Input/output channel count mismatch."));
		checkf(InAudio.Num() == NumChannels, TEXT("Incorrect audio channel count."));

		int32 NumOutputFrames = MultiChannelBufferCustom::GetMultichannelBufferNumFrames(OutAudio);
		const int32 NumAvailableInputFrames = MultiChannelBufferCustom::GetMultichannelBufferNumFrames(InAudio);
		int32 NumInputFramesRequired = GetNumInputFrameToProduceNumOutPutFrame(SourceState, NumOutputFrames, bLoop);
		
		if (NumAvailableInputFrames < NumInputFramesRequired)
		{
			int32 NumPossibleOutputFrames = GetSafeNumOutputFrames(SourceState, bLoop, NumAvailableInputFrames);
			if(NumPossibleOutputFrames <= 0)
			{
				if(!bLoop && NumAvailableInputFrames >= SourceState.GetEOFFrameIndexInBuffer())
				{ //Reach outside of EOF but we still need to produce some 0 frames to complete one block for MetaSound
					NumPossibleOutputFrames = NumOutputFrames;
				}
			}
			
			//Near EOF we can create more frames with lower number of input data
			//So make sure NumOutputFrames don't exceed Output buffer
			NumOutputFrames = FMath::Min(NumOutputFrames, NumPossibleOutputFrames);
			NumInputFramesRequired = NumAvailableInputFrames;
		}

		if (NumOutputFrames > 0)
		{
			checkf(NumInputFramesRequired <= NumAvailableInputFrames,
				  TEXT("FMultichannelOLA::ProcessAndConsumeAudioInternal: NumInputFramesRequired (%d) is still larger than NumAvailableInputFrames (%d)"),
				  NumInputFramesRequired, NumAvailableInputFrames);
			
			const int32 NextLeftOverlapIndexInBuffer = ProcessChannelAudioInternal(InAudio, OutAudio, SourceState, bLoop, NumOutputFrames);
			
			int32 NumFramesToPop = FMath::Max(0, NextLeftOverlapIndexInBuffer - MaxPadLength);
			NumFramesToPop = FMath::Min(NumAvailableInputFrames, NumFramesToPop);
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
				InAudio[ChannelIndex].Pop(NumFramesToPop);

			CurrentLeftStartOverlapIndexInBuffer -= NumFramesToPop;
			CurrentPadding = NextLeftOverlapIndexInBuffer - NumFramesToPop;
			if(!bLoop && NextLeftOverlapIndexInBuffer >= SourceState.GetEOFFrameIndexInBuffer())
			{
				//Remove all padding if we reach EOF 
				for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
					InAudio[ChannelIndex].Pop(CurrentPadding);
				CurrentPadding = 0;
				CurrentLeftStartOverlapIndexInBuffer = 0;
			}
		}

		return NumOutputFrames;
	}
	
	int32 FMultichannelOLA::ProcessChannelAudioInternal(FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio, const FSourceBufferState& SourceState, const bool bLoop, int32 NumOutputFrames)
	{
		if (NumOutputFrames < 1)
			return 0.f;

		NumInputSamplesProcessed = 0;
		const int32 NumInput = MultiChannelBufferCustom::GetMultichannelBufferNumFrames(InAudio);
		const int32 EndCurrentOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + HalfWinSize;
		if(CurrentPadding >= EndCurrentOverlapIdx)
		{	// Out side of overlap window -> jump to next overlap window. This should rarely happen. Mostly caused by bugs!
			UE_LOG(LogTimeStretch, Warning, TEXT("FMultichannelOLA::ProcessChannelAudioInternal: CurrentFrameIdx is outside of current overlap window."));
			while(true)
			{
				CurrentLeftStartOverlapIndexInBuffer += CurrentAnalysisSize;
				if(CurrentPadding < CurrentLeftStartOverlapIndexInBuffer + HalfWinSize)
					break;
			}
			
			const int32 SkipFrames = FMath::Max(CurrentLeftStartOverlapIndexInBuffer - CurrentPadding, 0);
			CurrentPadding += SkipFrames;
			NumInputSamplesProcessed += SkipFrames;
			CurrentLeftStartOverlapIndexInWave = CurrentLeftStartOverlapIndexInBuffer + SourceState.GetStartFrameIndex();
			if(bLoop && CurrentLeftStartOverlapIndexInWave >= SourceState.GetLoopEndFrameIndexInWave())
			{
				const int32 OverShoot = CurrentLeftStartOverlapIndexInWave - SourceState.GetLoopEndFrameIndexInWave();
				CurrentLeftStartOverlapIndexInWave = OverShoot + SourceState.GetLoopStartFrameIndexInWave();
			}
		}
		
		LeftOverlapInBuffer = CurrentPadding;
		const int32 FrameOffset = LeftOverlapInBuffer - CurrentLeftStartOverlapIndexInBuffer;
		const int32 NumAvailableInputFrames = MultiChannelBufferCustom::GetMultichannelBufferNumFrames(InAudio);
		if(LeftOverlapInBuffer >= NumAvailableInputFrames)
		{
			// If outside of input data output is all zeros so just return
			return LeftOverlapInBuffer;
		}
		
		if (FMath::IsNearlyEqual(CurrentFrameRatio, 1.f, EqualErrorTolerance) && FMath::IsNearlyEqual(CurrentFrameRatio, TargetFrameRatio, EqualErrorTolerance))
		{
			const int32 NumProducedFrame = FMath::Min(NumOutputFrames, NumInput - LeftOverlapInBuffer);
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
			{
				TArrayView<float> OutputBuffer = TArrayView<float>(OutAudio[ChannelIndex].GetData(), NumProducedFrame);
				const int32 NumFramesCopied = InAudio[ChannelIndex].Peek( OutputBuffer.GetData(), LeftOverlapInBuffer,  NumProducedFrame);
				check(NumFramesCopied == NumProducedFrame);
			}
			
			const int32 ConsumedFrameIndexInBuffer = LeftOverlapInBuffer + NumProducedFrame;
			int32 EndOverlapIndex = CurrentLeftStartOverlapIndexInBuffer + HalfWinSize;
			while (ConsumedFrameIndexInBuffer >= EndOverlapIndex)
				EndOverlapIndex += AdvanceCurrentStartLeftOverlapIndex(SourceState, bLoop);
			NumInputSamplesProcessed += NumProducedFrame;
			LeftOverlapInBuffer += NumProducedFrame;
			RightOverlapInBuffer = GetRightOverlapInBuffer(true, SourceState, bLoop, InAudio);
			return LeftOverlapInBuffer;
		}
		
		const FAlignedFloatBuffer* WindowBufferPtr = GetCurrentWindowBuffer();
		checkf(WindowBufferPtr != nullptr, TEXT("FMultichannelOLA::ProcessChannelAudioInternal: WindowBuffer is null!"));
		TArrayView<const float> WindowBuffer = *WindowBufferPtr;
		OutFrameStart = 0;
		
		bool bIsCompletePreviousOverlap = (FrameOffset == 0);
		int32 LastMaxProcessedIdxInBuffer =  LeftOverlapInBuffer;
		//Do we have remain overlap from last calculation?
		if(!bIsCompletePreviousOverlap)
		{
			const int32 NumProcessedFrames = FMath::Min3(NumOutputFrames, HalfWinSize - FrameOffset, NumInput - LeftOverlapInBuffer);
				
			TArrayView<const float> LeftWin = WindowBuffer.Slice(FrameOffset, NumProcessedFrames);
			const int32 StartRightWinIdx = HalfWinSize + FrameOffset;
			TArrayView<const float> RightWin = WindowBuffer.Slice(StartRightWinIdx, NumProcessedFrames);

			RightOverlapInBuffer = GetRightOverlapInBuffer(false, SourceState, bLoop, InAudio);
			OverlapAdd(InAudio, LeftWin, RightWin, NumProcessedFrames, OutAudio);
			LastMaxProcessedIdxInBuffer = FMath::Max(RightOverlapInBuffer + NumProcessedFrames, LeftOverlapInBuffer + NumProcessedFrames);
			
			bIsCompletePreviousOverlap = (FrameOffset + NumProcessedFrames) == HalfWinSize;
			if(bIsCompletePreviousOverlap)
			{ //Complete one overlap so we can start on next LeftOverlap
				const int32 SkipSize = AdvanceCurrentStartLeftOverlapIndex(SourceState, bLoop);

				//Since at least one window must finish to reach this, and Padding = HalfWin. CurrentLeftOverlapInBuffer expects to be always >= 0
				LeftOverlapInBuffer = LeftOverlapInBuffer + NumProcessedFrames + SkipSize - HalfWinSize;
				checkf(LeftOverlapInBuffer >= 0, TEXT("FMultichannelOLA::ProcessChannelAudioInternal: Negative! CurrentLeftOverlapInBuffer = %d"), LeftOverlapInBuffer);
			}
			else
				LeftOverlapInBuffer = LeftOverlapInBuffer + NumProcessedFrames;
			
			OutFrameStart = NumProcessedFrames;
			NumOutputFrames -= NumProcessedFrames;
		}

		if(bIsCompletePreviousOverlap && (CurrentFrameRatio != TargetFrameRatio))
		{
			// After finish previous incomplete overlap, we update FrameRatio to target ones
			CurrentFrameRatio = TargetFrameRatio;
			CurrentAnalysisSize = TargetAnalysisSize;
		}
		
		if(NumOutputFrames >= HalfWinSize && LeftOverlapInBuffer < NumInput)
		{
			TArrayView<const float> LeftWin = WindowBuffer.Slice(0, HalfWinSize);
			TArrayView<const float> RightWin = WindowBuffer.Slice(HalfWinSize, HalfWinSize);
			
			while(NumOutputFrames >= HalfWinSize && LeftOverlapInBuffer < NumInput)
			{
				RightOverlapInBuffer = GetRightOverlapInBuffer(true, SourceState, bLoop, InAudio);
				OverlapAdd(InAudio, LeftWin, RightWin, HalfWinSize, OutAudio);
				LastMaxProcessedIdxInBuffer = FMath::Max3(LastMaxProcessedIdxInBuffer, RightOverlapInBuffer + HalfWinSize, LeftOverlapInBuffer + HalfWinSize);
				OutFrameStart += HalfWinSize;
				NumOutputFrames -= HalfWinSize;
				LeftOverlapInBuffer += AdvanceCurrentStartLeftOverlapIndex(SourceState, bLoop);
			}
		}
		
		//Handle incomplete half window cases 
		if(NumOutputFrames > 0 && LeftOverlapInBuffer < NumInput)
		{
			const int32 NumProcessedFrames = FMath::Min(NumOutputFrames, NumInput - LeftOverlapInBuffer);
			TArrayView<const float> LeftWin = WindowBuffer.Slice(0, NumProcessedFrames);
			TArrayView<const float> RightWin = WindowBuffer.Slice(HalfWinSize, NumProcessedFrames);

			RightOverlapInBuffer = GetRightOverlapInBuffer(true, SourceState, bLoop, InAudio);
			OverlapAdd(InAudio, LeftWin, RightWin, NumProcessedFrames, OutAudio);
			LastMaxProcessedIdxInBuffer = FMath::Max3(LastMaxProcessedIdxInBuffer, RightOverlapInBuffer + NumProcessedFrames, LeftOverlapInBuffer + NumProcessedFrames);
			
			//Do not advance to next overlap as half window is incomplete
			LeftOverlapInBuffer += NumProcessedFrames;
			NumOutputFrames -= NumProcessedFrames;
		}

		NumInputSamplesProcessed += LastMaxProcessedIdxInBuffer - CurrentPadding;
		checkf(NumOutputFrames == 0, TEXT("FMultichannelOLA::ProcessChannelAudioInternal: Failed to process %d output frames"), NumOutputFrames);
		return LeftOverlapInBuffer;
	}
	
	void FMultichannelOLA::OverlapAdd( FMultichannelCircularBufferRand& InAudio, TArrayView<const float>& LeftWin, TArrayView<const float>& RightWin,
									  const int32 NumProcessedFrames, FMultichannelBufferView& OutBuffer)
	{
		const int32 NumAvailableInput = MultiChannelBufferCustom::GetMultichannelBufferNumFrames(InAudio);
		const int32 OutputCapacity = MultiChannelBufferCustom::GetMultichannelBufferNumFrames(OutBuffer);
		checkf(LeftOverlapInBuffer + NumProcessedFrames <= NumAvailableInput, TEXT("FMultichannelOLA::OverlapAdd: EndLeftOverlap (%d) exceed InAudioBuffer (%d)"), LeftOverlapInBuffer + NumProcessedFrames, NumAvailableInput);
		checkf(OutFrameStart + NumProcessedFrames <= OutputCapacity, TEXT("FMultichannelOLA::OverlapAdd: OutFrameStart (%d) exceed OutAudioBuffer (%d)"), OutFrameStart, OutputCapacity);
		
		const int32 EndRightOverlapInBuffer = RightOverlapInBuffer + NumProcessedFrames;
		for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
		{
			TArrayView<float> OutAudio = TArrayView<float>(OutBuffer[ChannelIndex].GetData(), OutputCapacity);
			
			if(RightOverlapInBuffer > NumAvailableInput || EndRightOverlapInBuffer < 0)
			{   //No data for right overlap -> copy input data to output
				float* OutData = OutAudio.Slice(OutFrameStart, NumProcessedFrames).GetData();
				const int32 NumFramesCopied = InAudio[ChannelIndex].Peek(OutData, LeftOverlapInBuffer, NumProcessedFrames);
				check(NumFramesCopied == NumProcessedFrames);
				continue;
			}

			TArrayView<float> LeftOverlap = TArrayView<float>(LeftOverlapBuffer.GetData(), NumProcessedFrames);
			TArrayView<float> RightOverlap = TArrayView<float>(RightOverlapBuffer.GetData(), NumProcessedFrames);
			
			if(RightOverlapInBuffer < 0)
			{
				const int32 ValidNum = -RightOverlapInBuffer;
				const int32 NumFramesCopied = InAudio[ChannelIndex].Peek(OutAudio.Slice(OutFrameStart, ValidNum).GetData(), LeftOverlapInBuffer,  ValidNum);
				check(NumFramesCopied == ValidNum);

				const int32 Remain = NumProcessedFrames - ValidNum;
				TArrayView<const float> RightWinRemain = RightWin.Slice(ValidNum, Remain);
				const int32 NumLeftFramesCopied = InAudio[ChannelIndex].Peek(LeftOverlap.GetData(), LeftOverlapInBuffer + ValidNum,  Remain);
				check(NumLeftFramesCopied == Remain);
				TArrayView<float> LeftInData = LeftOverlap.Slice(0, Remain);
				Audio::ArrayMultiplyInPlace(RightWinRemain, LeftInData);

				const int32 NumRightFramesCopied = InAudio[ChannelIndex].Peek(RightOverlap.GetData(), RightOverlapInBuffer + ValidNum,  Remain);
				check(NumRightFramesCopied == Remain);
				TArrayView<float> RightInData =  RightOverlap.Slice(0, Remain);
				TArrayView<const float> LeftWinRemain = LeftWin.Slice(ValidNum, Remain);
				Audio::ArrayMultiplyInPlace(LeftWinRemain, RightInData);

				Audio::ArraySum(LeftInData, RightInData, OutAudio.Slice(OutFrameStart + ValidNum, Remain));
				continue;
			}
		
			if(EndRightOverlapInBuffer > NumAvailableInput)
			{
				const int32 ValidNum = NumAvailableInput - RightOverlapInBuffer;
				TArrayView<const float> RightWinRemain = RightWin.Slice(0, ValidNum);
				const int32 NumLeftFramesCopied = InAudio[ChannelIndex].Peek( LeftOverlap.GetData(), LeftOverlapInBuffer,  ValidNum);
				check(NumLeftFramesCopied == ValidNum);
				TArrayView<float> LeftInData = LeftOverlap.Slice(0, ValidNum);
				Audio::ArrayMultiplyInPlace(RightWinRemain, LeftInData);

				const int32 NumRightFramesCopied = InAudio[ChannelIndex].Peek(RightOverlap.GetData(), RightOverlapInBuffer,  ValidNum);
				check(NumRightFramesCopied == ValidNum);
				TArrayView<float> RightInData =  RightOverlap.Slice(0, ValidNum);
				TArrayView<const float> LeftWinRemain = LeftWin.Slice(0, ValidNum);
				Audio::ArrayMultiplyInPlace(LeftWinRemain, RightInData);

				Audio::ArraySum(LeftInData, RightInData, OutAudio.Slice(OutFrameStart, ValidNum));

				const int32 Remain = NumProcessedFrames - ValidNum;
				const int32 NumFramesCopied = InAudio[ChannelIndex].Peek(OutAudio.Slice(OutFrameStart + ValidNum, Remain).GetData(),
																			LeftOverlapInBuffer + ValidNum,  Remain);
				check(NumFramesCopied == Remain);
				continue;
			}

			const int32 NumLeftFramesCopied = InAudio[ChannelIndex].Peek(LeftOverlap.GetData(), LeftOverlapInBuffer,  NumProcessedFrames);
			check(NumLeftFramesCopied == NumProcessedFrames);
			const int32 NumRightFramesCopied = InAudio[ChannelIndex].Peek(RightOverlap.GetData(), RightOverlapInBuffer,  NumProcessedFrames);
			check(NumRightFramesCopied == NumProcessedFrames);

			TArrayView<float> LeftInData = LeftOverlap.Slice(0, NumProcessedFrames);
			TArrayView<float> RightInData =  RightOverlap.Slice(0, NumProcessedFrames);
			
			Audio::ArrayMultiplyInPlace(RightWin, LeftInData);
			Audio::ArrayMultiplyInPlace(LeftWin, RightInData);
			Audio::ArraySum(LeftInData, RightInData, OutAudio.Slice(OutFrameStart, NumProcessedFrames));
		}
	}

	int32 FMultichannelOLA::GetCurrentFrameIndexInWavePadding(const FSourceBufferState& SourceState, const bool bLooping) const
	{
		int32 IndexInWave = SourceState.GetStartFrameIndex() + CurrentPadding;
		const int32 EOFFrame = SourceState.GetLoopEndFrameIndexInWave();
		if(bLooping)
		{
			if(IndexInWave >= EOFFrame)
			{
				const int32 Overshot = IndexInWave - EOFFrame;
				const int32 WrapIndex = SourceState.GetLoopStartFrameIndexInWave() + Overshot;
				//Only return wrap index if CurrentLeftStartOverlapIndexInWave has also been wrapped.
				if(WrapIndex >= CurrentLeftStartOverlapIndexInWave)
					return WrapIndex;
			}
			else if(IndexInWave < CurrentLeftStartOverlapIndexInWave)
			{ //Current index is wrapped around loop point before CurrentLeftStartOverlap
				IndexInWave = IndexInWave - SourceState.GetLoopStartFrameIndexInWave() + SourceState.GetLoopEndFrameIndexInWave();
			}
		}

		const int32 EndFrame = bLooping ?  SourceState.GetLoopEndFrameIndexInWave() : SourceState.GetEOFFrameIndexInWave();
		checkf(IndexInWave >= CurrentLeftStartOverlapIndexInWave || (CurrentLeftStartOverlapIndexInWave >= EndFrame), TEXT("FMultichannelOLA::GetCurrentFrameIndexInWavePadding: %d frame index is smaller than CurrentLeftStartOverlapIndexInWave=%d!"), IndexInWave, CurrentLeftStartOverlapIndexInWave);
		return IndexInWave;
	}
	
	int32 FMultichannelOLA::AdvanceCurrentStartLeftOverlapIndex(const FSourceBufferState& SourceState, const bool bLoop)
	{
		CurrentLeftStartOverlapIndexInBuffer += CurrentAnalysisSize;
		CurrentLeftStartOverlapIndexInWave += CurrentAnalysisSize;
		if(bLoop && CurrentLeftStartOverlapIndexInWave >= SourceState.GetLoopEndFrameIndexInWave())
		{
			const int32 OverShoot = CurrentLeftStartOverlapIndexInWave - SourceState.GetLoopEndFrameIndexInWave();
			CurrentLeftStartOverlapIndexInWave = OverShoot + SourceState.GetLoopStartFrameIndexInWave();
		}
		return CurrentAnalysisSize;
	}

	int32 FMultichannelOLA::GetRightOverlapInBuffer(const bool bUpdateOverlap, const FSourceBufferState& SourceState, const bool bLoop, FMultichannelCircularBufferRand& InAudio)
	{
		return LeftOverlapInBuffer + CurrentAnalysisSize - HalfWinSize;
	}
}

