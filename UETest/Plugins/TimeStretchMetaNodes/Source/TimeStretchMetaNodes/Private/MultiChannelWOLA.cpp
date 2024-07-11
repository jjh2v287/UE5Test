// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "MultiChannelWOLA.h"

#include "CustomArrayMath.h"
#include "TimeStretchLog.h"
#include "DSP/FloatArrayMath.h"

DECLARE_STATS_GROUP(TEXT("AudioWOLA"), STATGROUP_AudioWOLA, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("WOLA - TotalProccesing"), STAT_OverlapProcessing, STATGROUP_AudioWOLA);
DECLARE_CYCLE_STAT(TEXT("WOLA - FindMaxCorrelation"), STAT_FindMaxCorrelation, STATGROUP_AudioWOLA);

namespace LBSTimeStretch
{
	using namespace Audio;

	FMultichannelWOLA::FMultichannelWOLA(int32 InNumChannels, float InSamplingRate, float InFrameLenghtSec,
										float InFrameRatio, int32 StartFrameIndex, float InDevStepPercent, int32 NumFramesPerBlock)
	: FMultichannelOLA(InNumChannels, InSamplingRate, InFrameLenghtSec, InFrameRatio, StartFrameIndex)
	{
		MaxDeviation = GetHalfWinSize();
		MaxPadLength = 3 * MaxDeviation;
		CurrentDeviation = 0;
		CurrentActualAnalysisSize = -1;		
		StartFrameInWave = StartFrameIndex;
		DevStepSize = FMath::Max(1, FMath::FloorToInt32(InDevStepPercent * MaxDeviation));
		TargetBlockRate = NumFramesPerBlock;
		const int32 NumSegments = FMath::CeilToInt32(static_cast<float>(NumFramesPerBlock) / GetHalfWinSize()) + 2;
		SegmentMaps.Reserve(NumSegments);
		for(int i = 0; i < NumSegments; i++)
			SegmentMaps.Emplace(FInOutSegmentMap(-1, 0, 0));
		CurrentMapSegmentIndex = 0;
		
		SignRightOverlapDeviationBuffer.SetNumUninitialized(GetHalfWinSize() + 2 * MaxDeviation, false);
	}

	float FMultichannelWOLA::MapInputEventToOutputBufferFrame(const FSourceBufferState& SourceState, const int32 InEventIndexInputBuffer, const bool bLoop) const
	{
		checkf(false, TEXT("FMultichannelWOLA::MapInputEventToOutputBufferFrame is not implented for this algorithms. Please use GetPreviousInputEventIndexInOutputBuffer() instead!"));
		return 0;
	}

	int32 FMultichannelWOLA::GetPreviousInputEventIndexInOutputBuffer(const int32 FirstInputIdxToCurrentOutputBuffer, const FSourceBufferState& SourceState, const int32 InputIndexInWave, const bool bLoop) const
	{
		float Distance = static_cast<float>(InputIndexInWave - FirstInputIdxToCurrentOutputBuffer);
		if(Distance < 0 && bLoop)
		{
			const int32 OverShoot = InputIndexInWave - SourceState.GetLoopStartFrameIndexInWave();
			Distance = static_cast<float>(OverShoot + SourceState.GetLoopEndFrameIndexInWave() - FirstInputIdxToCurrentOutputBuffer);
		}
		if(Distance < 0)
			return -1;
		
		int32 TotalOutput = 0;
		int32 TotalInput = 0;
		const int32 SegmentContainInputIdx = FindSegmentMap(FirstInputIdxToCurrentOutputBuffer, TotalInput, TotalOutput);
		if(SegmentContainInputIdx < 0)
			return -1;

		if(TotalOutput <= 0)
			return -1;

		if(Distance < TotalInput)
			return FMath::FloorToInt32(Distance * TotalOutput / TotalInput);

		//Outside of current buffer will always return -1
		//We won't map input index outside of output buffer as it's unpredictable until we find next overlap based on correlation
		return -1;
	}

	int32 FMultichannelWOLA::FindSegmentMap(const int32 StartInputIdxInWave, int32& TotalInput, int32& TotalOutput) const
	{
		int32 SegmentIdx = CurrentMapSegmentIndex;
		int32 SegmentContainInputIdx = -1;
		TotalOutput = 0;
		TotalInput = 0;
		// Find the latest segment contain input frame
		while (true)
		{
			const FInOutSegmentMap Segment = SegmentMaps[SegmentIdx];
			TotalOutput += Segment.NumOutput;
			TotalInput += Segment.NumInput;
			
			if(StartInputIdxInWave == Segment.StartFrame)
			{
				SegmentContainInputIdx = SegmentIdx;
				break;
			}
			
			SegmentIdx--;
			if(SegmentIdx < 0)
				SegmentIdx = SegmentMaps.Num() - 1;
			if(SegmentIdx == CurrentMapSegmentIndex)
				break;
		}
		
		return SegmentContainInputIdx;
	}

	int32 FMultichannelWOLA::GetInputFrameCapacityForMaxFrameRatio(const int32 InNumOutputFrame,
																   const float MaxFrameRatio) const
	{
		if(InNumOutputFrame <= 0)
			return 0;

		const int32 HWSize = GetHalfWinSize();
		
		// +1 since worst case is having an unfinished overlap before finding next overlap
		const int32 NumOverlap = FMath::CeilToInt32(static_cast<float>(InNumOutputFrame) / HWSize) + 1;

		//Worst case will have analysis step + deviation
		const int32 AnalysisStep = FMath::CeilToInt32(HWSize * MaxFrameRatio);
		const int32 EndRightOverlapIdx = AnalysisStep * NumOverlap;
		const int32 EndLeftOverlapIdx = AnalysisStep * (NumOverlap - 1) + HWSize;
		return  FMath::Max(EndRightOverlapIdx, EndLeftOverlapIdx) + GetMaxPaddingLength() + MaxDeviation;;
	}
	
	int32 FMultichannelWOLA::GetNumInputFrameToProduceNumOutPutFrame(const FSourceBufferState& SourceState,
	                                                                 const int32 InNumOutputFrame, const bool bLoop) const
	{
		if(InNumOutputFrame <= 0)
			return 0;

		const int32 HWSize = GetHalfWinSize();
		const int32 EndCurrentLeftOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + HWSize;
		const bool bIsStartOverlap = CurrentPadding == CurrentLeftStartOverlapIndexInBuffer;

		//If start of current overlap then we don't know step size until RightOverlap correlation calculation
		//Thus just assume worse case = max deviation
		const int32 EndCurrentRightOverlapIdxInBuffer = bIsStartOverlap ? (CurrentLeftStartOverlapIndexInBuffer - CurrentDeviation + GetTargetAnalysisSize() + MaxDeviation)
																		: CurrentLeftStartOverlapIndexInBuffer + CurrentActualAnalysisSize;
		//Make sure we always start calculation at complete overlap block
		const int32 NumRemainOutputFrame = (CurrentPadding < EndCurrentLeftOverlapIdx) ? (InNumOutputFrame - (EndCurrentLeftOverlapIdx - CurrentPadding)) : InNumOutputFrame;
		if(NumRemainOutputFrame <= 0)
		{
			if(bIsStartOverlap)
				return  FMath::Max(EndCurrentRightOverlapIdxInBuffer, CurrentLeftStartOverlapIndexInBuffer + HWSize);
			
			const int32 EndRightOverlapIdx =  EndCurrentRightOverlapIdxInBuffer - HWSize + (CurrentPadding - CurrentLeftStartOverlapIndexInBuffer) + InNumOutputFrame;
			return  FMath::Max(EndRightOverlapIdx, CurrentPadding + InNumOutputFrame);
		}
		
		//From here switch to use MaxTargetAnalysisSize as we will update new FrameRatio (if change) on start of next overlap
		
		//StartOffset can be zero for StretchRatio < 1.0. To avoid MaxNumInputNeeded < CurrentPadding set it to always be larger or equal to zero
		const int32 NextDeviation = bIsStartOverlap ? MaxDeviation : CurrentDeviation;
		const int32 StartOffset = FMath::Max3(0, EndCurrentRightOverlapIdxInBuffer - NextDeviation,
												   EndCurrentLeftOverlapIdx - CurrentDeviation);

		const int32 NumOverlap = FMath::CeilToInt32(static_cast<float>(NumRemainOutputFrame) / HWSize);
		const int32 CurrentTargetAnaSize = GetTargetAnalysisSize();
		const int32 EndRightOverlapIdx = CurrentTargetAnaSize * NumOverlap + MaxDeviation;
		const int32 EndLeftOverlapIdx = CurrentTargetAnaSize * (NumOverlap - 1) + MaxDeviation + HWSize;
		const int32 MaxNumInputNeeded = FMath::Max(EndRightOverlapIdx, EndLeftOverlapIdx) + StartOffset;
		return MaxNumInputNeeded;
	}

	int32 FMultichannelWOLA::GetSafeNumOutputFrames(const FSourceBufferState& SourceState, bool bLoop,
	                                                int32 NumAvailableInputFrames) const
	{
		const int32 HWSize = GetHalfWinSize();
		
		const int32 NumNewInputData = NumAvailableInputFrames - CurrentPadding;
		if(NumNewInputData <= 0)
			return 0;
		
		const int32 EndCurrentLeftOverlapIdx = CurrentLeftStartOverlapIndexInBuffer + HWSize;
		const int32 NumIncompleteSamples = FMath::Max(EndCurrentLeftOverlapIdx - CurrentPadding, 0);

		const bool bIsStartOverlap = CurrentPadding == CurrentLeftStartOverlapIndexInBuffer;
		const int32 EndCurrentRightOverlapIdxInWave = bIsStartOverlap ? (CurrentLeftStartOverlapIndexInBuffer - CurrentDeviation + GetTargetAnalysisSize() + MaxDeviation)
																		: CurrentLeftStartOverlapIndexInBuffer + CurrentActualAnalysisSize;
		const int32 StartCurrentRightOverlapIdx = EndCurrentRightOverlapIdxInWave - HWSize;
		const bool bReachEOF = !bLoop && NumAvailableInputFrames >= SourceState.GetEOFFrameIndexInBuffer();
		const int32 CompleteIdx = FMath::Max(EndCurrentRightOverlapIdxInWave, EndCurrentLeftOverlapIdx);
		if(NumAvailableInputFrames <= CompleteIdx)
		{
			const int32 NumRemainSamples = FMath::Min(NumIncompleteSamples, NumNewInputData);
			if(CurrentPadding < EndCurrentLeftOverlapIdx)
			{
				// Is EOF and no loop
				if(bReachEOF)
				{
					const int32 NumEndFrames = FMath::Min(NumRemainSamples, SourceState.GetEOFFrameIndexInBuffer() - CurrentPadding); 
					return FMath::Max(0, NumEndFrames);
				}

				// If not start overlap -> RightOverlap is already calculated so we can use it here
				// If start overlap and (NumAvailableInputFrames < CompleteIdx) then number of samples aren't  enough
				// for max deviation correlation to find the correct right overlap
				if(!bIsStartOverlap || NumAvailableInputFrames == CompleteIdx)
				{
					const int32 StartOffset = CurrentPadding - CurrentLeftStartOverlapIndexInBuffer;
					const int32 NumOut = FMath::Min(NumRemainSamples, NumAvailableInputFrames - (StartCurrentRightOverlapIdx + StartOffset));
					return FMath::Max(0, NumOut);
				}
			}
			
			return 0;
		}
		
		int32 NumOutputFrames = NumIncompleteSamples;
		const int32 CurrentTargetAnaSize = GetTargetAnalysisSize();
		const int32 CurDeviation = bIsStartOverlap ? MaxDeviation : CurrentDeviation;
		int32 NextIdealEndRightOverlapIdx = EndCurrentRightOverlapIdxInWave - CurDeviation + CurrentTargetAnaSize;
		int32 NumCompleteRemainOverlap = 0;
		while (NextIdealEndRightOverlapIdx < NumAvailableInputFrames)
		{
			NextIdealEndRightOverlapIdx += CurrentTargetAnaSize;
			NumCompleteRemainOverlap++;
		}
		
		int32 LastStartLeftOverlapIndex = NextIdealEndRightOverlapIdx - CurrentTargetAnaSize + MaxDeviation;
		int32 LastCompleteEndLeftOverlapIdx = LastStartLeftOverlapIndex - CurrentTargetAnaSize + HWSize;
		while (NumCompleteRemainOverlap > 0 && (LastStartLeftOverlapIndex > NumAvailableInputFrames || LastCompleteEndLeftOverlapIdx > NumAvailableInputFrames))
		{
			LastStartLeftOverlapIndex -= CurrentTargetAnaSize;
			LastCompleteEndLeftOverlapIdx -= CurrentTargetAnaSize;
			NumCompleteRemainOverlap--;
		}

		NumOutputFrames += NumCompleteRemainOverlap * HWSize;
		
		if(bReachEOF && NumAvailableInputFrames > LastStartLeftOverlapIndex)
		{
			NumOutputFrames += FMath::Min(HWSize, NumAvailableInputFrames - LastStartLeftOverlapIndex);
		}
		
		//We truncate (InputEndIdx - LastStartLeftOverlapInde) > HWSize so this can return lower output size
		//than it actually can which is more fail safe than trying to predict the correct number
		return NumOutputFrames;
	}

	int32 FMultichannelWOLA::ProcessAndConsumeAudioInternal(FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio,
															const FSourceBufferState& SourceState, const bool bLoop)
	{
		SCOPE_CYCLE_COUNTER(STAT_OverlapProcessing)
		
		PreviousStartProcessedInputIdxInWave = SourceState.GetStartFrameIndex() + CurrentPadding;
		PreviousNumberOfOutput = FMultichannelOLA::ProcessAndConsumeAudioInternal(InAudio, OutAudio, SourceState, bLoop);

		if(PreviousNumberOfOutput > 0)
		{
			SegmentMaps[CurrentMapSegmentIndex].Append(PreviousStartProcessedInputIdxInWave, NumInputSamplesProcessed, PreviousNumberOfOutput);
			if(SegmentMaps[CurrentMapSegmentIndex].NumOutput >= TargetBlockRate)
			{
				CurrentMapSegmentIndex++;
				if(CurrentMapSegmentIndex >= SegmentMaps.Num())
					CurrentMapSegmentIndex = 0;
				SegmentMaps[CurrentMapSegmentIndex].Reset();
			}
		}
		
		return PreviousNumberOfOutput;
	}

	int32 FMultichannelWOLA::AdvanceCurrentStartLeftOverlapIndex(const FSourceBufferState& SourceState, const bool bLoop)
	{
		const int32 CurrentAna = GetCurrentAnalysisSize();
		const bool bIsNoStretch = FMath::IsNearlyEqual(GetCurrentFrameRatio(), 1.f, EqualErrorTolerance);
		const int32 AnaSize = bIsNoStretch ? CurrentAna : CurrentActualAnalysisSize;

		CurrentLeftStartOverlapIndexInBuffer += AnaSize;
		CurrentLeftStartOverlapIndexInWave += AnaSize;
		if(bLoop && CurrentLeftStartOverlapIndexInWave >= SourceState.GetLoopEndFrameIndexInWave())
		{
			const int32 OverShoot = CurrentLeftStartOverlapIndexInWave - SourceState.GetLoopEndFrameIndexInWave();
			StartFrameInWave = SourceState.GetLoopStartFrameIndexInWave(); //Also update in case first loop start frame is different
			CurrentLeftStartOverlapIndexInWave = OverShoot + StartFrameInWave;
		}
		else if(CurrentLeftStartOverlapIndexInWave < StartFrameInWave) //If negative then we need to check if we step back exceed LoopStartIndex
		{
			const int32 UnderShoot = StartFrameInWave - CurrentLeftStartOverlapIndexInWave;
			CurrentLeftStartOverlapIndexInWave = SourceState.GetLoopEndFrameIndexInWave() - UnderShoot;
		}
		
		return AnaSize;
	}

	int32 FMultichannelWOLA::GetRightOverlapInBuffer(const bool bUpdateOverlap, const FSourceBufferState& SourceState, const bool bLoop, FMultichannelCircularBufferRand& InAudio)
	{
		if(bUpdateOverlap)
		{
			const bool bIsNoStretch = FMath::IsNearlyEqual(GetCurrentFrameRatio(), 1.f, EqualErrorTolerance);
			if(!bIsNoStretch)
			{
				SCOPE_CYCLE_COUNTER(STAT_FindMaxCorrelation);
				FindRightOverlapWithMaxCorrelation(SourceState, bLoop, InAudio);
			}
			else
			{
				CurrentActualAnalysisSize = GetCurrentAnalysisSize();
				CurrentDeviation = 0;
			}
		}
		
		return LeftOverlapInBuffer + CurrentActualAnalysisSize - GetHalfWinSize();
	}

	void FMultichannelWOLA::FindRightOverlapWithMaxCorrelation(const FSourceBufferState& SourceState, const bool bLoop, FMultichannelCircularBufferRand& InAudio)
	{
		//Use only first channel for correlation calculation
		TCircularAudioBufferCustom<float> FirstAudioChannel =  InAudio[0];
		const int32 HWSize = GetHalfWinSize();
		const int32 NumInput = MultiChannelBufferCustom::GetMultichannelBufferNumFrames(InAudio);
		const bool bReachOEF = !bLoop && (SourceState.GetEOFFrameIndexInBuffer() <= NumInput);
		const int32 AnalysisSize = GetCurrentAnalysisSize();
		
		const int32 LeftEndOverlapInBuffer = LeftOverlapInBuffer + HWSize; 
		if(LeftEndOverlapInBuffer > NumInput && bReachOEF)
		{
			CurrentDeviation = 0;
			CurrentActualAnalysisSize = AnalysisSize;
			return;
		}

		//Next overlap will always start without considering Deviation to keep correct stretch ratio
		const int32 RightEndOverlapInBuffer = LeftOverlapInBuffer - CurrentDeviation + AnalysisSize;
		const int32 RightStartOverlapInBuffer = RightEndOverlapInBuffer - HWSize;
		const int32 DeviationSize = GetCurrentFrameRatio() < 1 ? MaxDeviation : (MaxDeviation / 2);
		const int32 MinIdx = FMath::Max(0, RightStartOverlapInBuffer - DeviationSize);
		const int32 EndIdx = FMath::Min(NumInput, RightEndOverlapInBuffer + DeviationSize);
		const int32 CorrelationCalSize = EndIdx - MinIdx;
		const int32 MaxIdx = CorrelationCalSize - HWSize;
		if(MaxIdx < 0 || (RightEndOverlapInBuffer > NumInput && bReachOEF))
		{
			CurrentDeviation = 0;
			CurrentActualAnalysisSize = AnalysisSize;
			return;
		}
		
		checkf(RightEndOverlapInBuffer <= NumInput, TEXT("FMultichannelWOLA::FindRightOverlapWithMaxCorrelation: EndStartOverlapInBuffer = %d > %d"), RightEndOverlapInBuffer, NumInput);
		FirstAudioChannel.Peek(LeftOverlapBuffer.GetData(), LeftOverlapInBuffer,  HWSize);
		CustomArrayMath::ArraySignInPlace(LeftOverlapBuffer);
		
		FirstAudioChannel.Peek(SignRightOverlapDeviationBuffer.GetData(), MinIdx,  CorrelationCalSize);
		CustomArrayMath::ArraySignInPlace(SignRightOverlapDeviationBuffer);
		
		const int32 FirstRightStartOverlapIdx = FMath::Max(0, RightStartOverlapInBuffer) - MinIdx;
		const int32 HWCopySize = HWSize * sizeof(float); 
		FMemory::Memcpy(RightOverlapBuffer.GetData(), &SignRightOverlapDeviationBuffer[FirstRightStartOverlapIdx], HWCopySize);

		float MaxCorr = 0;
		Audio::ArrayMultiplyInPlace(LeftOverlapBuffer, RightOverlapBuffer);
		Audio::ArraySum(RightOverlapBuffer, MaxCorr);
		int32 RealRightStartOverlapInBuffer = FirstRightStartOverlapIdx;
		int32 NextIdx = FirstRightStartOverlapIdx - DevStepSize;
		if(GetCurrentFrameRatio() > 1)
		{
			for(; NextIdx >= 0; NextIdx -= DevStepSize)
			{
				FMemory::Memcpy(RightOverlapBuffer.GetData(), &SignRightOverlapDeviationBuffer[NextIdx], HWCopySize);
				Audio::ArrayMultiplyInPlace(LeftOverlapBuffer, RightOverlapBuffer);
				float Corr;
				Audio::ArraySum(RightOverlapBuffer, Corr);

				if(Corr > MaxCorr)
				{
					MaxCorr = Corr;
					RealRightStartOverlapInBuffer = NextIdx;
				}
			}
		}
		
		NextIdx = FirstRightStartOverlapIdx + DevStepSize;
		for(; NextIdx <= MaxIdx; NextIdx += DevStepSize)
		{
			FMemory::Memcpy(RightOverlapBuffer.GetData(), &SignRightOverlapDeviationBuffer[NextIdx], HWCopySize);
			Audio::ArrayMultiplyInPlace(LeftOverlapBuffer, RightOverlapBuffer);
			float Corr;
			Audio::ArraySum(RightOverlapBuffer, Corr);

			if(Corr > MaxCorr)
			{
				MaxCorr = Corr;
				RealRightStartOverlapInBuffer = NextIdx;
			}
		}

		RealRightStartOverlapInBuffer += MinIdx;
		CurrentDeviation = RealRightStartOverlapInBuffer - RightStartOverlapInBuffer;
		CurrentActualAnalysisSize = RealRightStartOverlapInBuffer + HWSize - LeftOverlapInBuffer;
	}
	
}
