// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "MultiChannelOLA.h"

namespace LBSTimeStretch
{
	class FMultichannelWOLA : public FMultichannelOLA
	{
	public:
		/** Constructor.
		 *
		 * @param InNumChannels - Number of audio channels in input and output buffers.
		 * @param InSamplingRate - Playback sampling rate .
		 * @param InFrameLenghtSec - One overlap frame length in seconds.
		 * @param InFrameRatio - Playback stretch ratio.
		 * @param StartFrameIndex - First frame index in wave to be played.
		 * @param InDevStepPercent - Step size percent for each correlation calculation in deviation range.
		 * @param NumFramesPerBlock - Block rate.
		 */
		TIMESTRETCHMETANODES_API FMultichannelWOLA(int32 InNumChannels, float InSamplingRate, float InFrameLenghtSec,
												  float InFrameRatio, int32 StartFrameIndex, float InDevStepPercent, int32 NumFramesPerBlock);

		TIMESTRETCHMETANODES_API virtual float MapInputEventToOutputBufferFrame(const FSourceBufferState& SourceState, const int32 InEventIndexInputBuffer, const bool bLoop) const override;
		
		TIMESTRETCHMETANODES_API int32 GetPreviousInputEventIndexInOutputBuffer(const int32 FirstInputIdxToCurrentOutputBuffer, const FSourceBufferState& SourceState, const int32 InputIndexInWave, const bool bLoop) const;

		TIMESTRETCHMETANODES_API virtual int32 GetNumInputFrameToProduceNumOutPutFrame(const FSourceBufferState& SourceState, const int32 InNumOutputFrame, const bool bLoop) const override;
		
		TIMESTRETCHMETANODES_API virtual int32 GetInputFrameCapacityForMaxFrameRatio(const int32 InNumOutputFrame, const float MaxFrameRatio) const override;

		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetPreviousStartProcessInputIdxInWave() const { return PreviousStartProcessedInputIdxInWave; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetPreviousNumberOfOutput() const { return PreviousNumberOfOutput; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetPreviousNumInputSamplesProcessed() const { return NumInputSamplesProcessed; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetMaxDeviation() const { return MaxDeviation; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetCurrentDeviation() const { return CurrentDeviation; }
		TIMESTRETCHMETANODES_API FORCEINLINE FInOutSegmentMap GetSegment(const int32 Index) const { return SegmentMaps[Index]; }
		
		TIMESTRETCHMETANODES_API int32 FindSegmentMap(const int32 StartInputIdxInWave, int32& TotalInput, int32& TotalOutput) const;
		
	protected:
		virtual int32 GetSafeNumOutputFrames(const FSourceBufferState& SourceState, bool bLoop, int32 NumAvailableInputFrames) const override;

		virtual int32 AdvanceCurrentStartLeftOverlapIndex(const FSourceBufferState& SourceState, const bool bLoop) override;

		virtual int32 GetRightOverlapInBuffer(const bool bUpdateOverlap, const FSourceBufferState& SourceState, const bool bLoop, FMultichannelCircularBufferRand& InAudio) override;

		virtual int32 ProcessAndConsumeAudioInternal(FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio, const FSourceBufferState& SourceState, const bool bLoop) override;
		
	private:

		void FindRightOverlapWithMaxCorrelation(const FSourceBufferState& SourceState, const bool bLoop, FMultichannelCircularBufferRand& InAudio);

		Audio::FAlignedFloatBuffer SignRightOverlapDeviationBuffer;
		
		int32 MaxDeviation;
		int32 CurrentDeviation;
		int32 CurrentActualAnalysisSize;
		int32 DevStepSize;

		int32 PreviousStartProcessedInputIdxInWave;
		int32 PreviousNumberOfOutput;

		int32 StartFrameInWave;
		
		//Stored previously processed segment to map input events to output
		TArray<FInOutSegmentMap> SegmentMaps;
		int32 CurrentMapSegmentIndex;
		int32 TargetBlockRate;
	};
}
