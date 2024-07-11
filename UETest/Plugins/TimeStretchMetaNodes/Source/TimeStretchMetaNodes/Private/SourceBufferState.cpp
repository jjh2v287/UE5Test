// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "SourceBufferState.h"

namespace LBSTimeStretch
{
	FSourceBufferState::FSourceBufferState(int32 InStartFrameIndex, int32 InNumFrames, bool bIsLooping,
		int32 InLoopStartFrameIndexInWave, int32 InLoopEndFrameIndexInWave, int32 InEOFFrameIndexInWave)
	{
		check(InStartFrameIndex >= 0);
		check(InNumFrames >= 0);
		check(InLoopStartFrameIndexInWave >= 0);
		check(InLoopEndFrameIndexInWave >= 0);
		check(InEOFFrameIndexInWave >= 0);

		StartFrameIndex = InStartFrameIndex;
		EndFrameIndex = InStartFrameIndex; // Initialize to starting frame index. Will be adjusted during call to Append()
		EOFFrameIndexInBuffer = InEOFFrameIndexInWave - InStartFrameIndex;
		LoopEndFrameIndexInBuffer = InLoopEndFrameIndexInWave - InStartFrameIndex;
		LoopStartFrameIndexInWave = InLoopStartFrameIndexInWave;
		LoopEndFrameIndexInWave = InLoopEndFrameIndexInWave;
		EOFFrameIndexInWave = InEOFFrameIndexInWave;
		Append(InNumFrames, bIsLooping);
	}

	void FSourceBufferState::Advance(int32 InNumFrames, bool bIsLooping)
	{
		check(InNumFrames >= 0);

		StartFrameIndex += InNumFrames;
		if (bIsLooping && StartFrameIndex > LoopEndFrameIndexInWave)
		{
			StartFrameIndex = WrapLoop(StartFrameIndex);
			LoopCount++;
		}

		EOFFrameIndexInBuffer = EOFFrameIndexInWave - StartFrameIndex;
		LoopEndFrameIndexInBuffer = LoopEndFrameIndexInWave - StartFrameIndex;
	}

	void FSourceBufferState::Append(int32 InNumFrames, bool bIsLooping)
	{
		check(InNumFrames >= 0);
		EndFrameIndex += InNumFrames;

		if (bIsLooping)
		{
			EndFrameIndex = WrapLoop(EndFrameIndex);
		}
	}

	void FSourceBufferState::SetLoopFrameIndices(int32 InLoopStartFrameIndexInWave,
		int32 InLoopEndFrameIndexInWave)
	{
		LoopStartFrameIndexInWave = InLoopStartFrameIndexInWave;
		LoopEndFrameIndexInWave = InLoopEndFrameIndexInWave;
		LoopEndFrameIndexInBuffer = LoopEndFrameIndexInWave - StartFrameIndex;
	}

	void FSourceBufferState::SetLoopFrameIndices(const FCustomWaveProxyReader& InProxyReader)
	{
		SetLoopFrameIndices(InProxyReader.GetLoopStartFrameIndex(), InProxyReader.GetLoopEndFrameIndex());
	}

	int32 FSourceBufferState::WrapLoop(int32 InSourceFrameIndex)
	{
		int32 Overshot = InSourceFrameIndex - LoopEndFrameIndexInWave;
		if (Overshot > 0)
		{
			InSourceFrameIndex = LoopStartFrameIndexInWave + Overshot;
		}
		return InSourceFrameIndex;
	}

	int32 GetCuePointFrame(const FSoundWaveCuePoint& InPoint)
	{
		return InPoint.FramePosition;
	}

	void FSourceEvents::Reset()
	{
		OnLoopFrameIndex = INDEX_NONE; 
		OnEOFFrameIndex = INDEX_NONE;
		OnCuePointFrameIndex = INDEX_NONE;
		CuePoint = nullptr;
		CuePointIndex = INDEX_NONE;
	}

	void FInOutSegmentMap::Append(int32 InStartFrame, int32 InNumInput, int32 InNumOutput)
	{
		if(NumOutput == 0)
			StartFrame = InStartFrame;
		
		NumInput += InNumInput;
		NumOutput += InNumOutput;
	}

	void FInOutSegmentMap::Reset()
	{
		StartFrame = -1;
		NumOutput = 0;
		NumInput = 0;
	}
}
