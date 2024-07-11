// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "SourceBufferState.h"
#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"

namespace LBSTimeStretch
{
	using namespace Audio;
	using namespace MultiChannelBufferCustom;
	
	/** OLA working on multichannel buffers */
	class FMultichannelOLA
	{
	public:
		/** Constructor.
		 *
		 * @param InNumChannels - Number of audio channels in input and output buffers.
		 * @param InSamplingRate - Playback sampling rate .
		 * @param InFrameLenghtSec - One overlap frame length in seconds.
		 * @param InFrameRatio - Playback stretch ratio.
		 * @param StartFrameIndex - First frame index in wave to be played.
		 */
		TIMESTRETCHMETANODES_API FMultichannelOLA(int32 InNumChannels, float InSamplingRate, float InFrameLenghtSec, float InFrameRatio, int32 StartFrameIndex);

		TIMESTRETCHMETANODES_API virtual ~FMultichannelOLA() = default;
		
		/** Sets the number of input frames to read per an output frame. 0.5 is 
		 * half speed, 1.f is normal speed, 2.0 is double speed.
		 *
		 * @param InRatio - Ratio of input frames consumed per an output frame produced.
		 * @param SourceState - current state of source buffers.
		 */
		TIMESTRETCHMETANODES_API void SetFrameRatio(float InRatio, const FSourceBufferState& SourceState);

		/** Map Output index directly to LeftOverlap index.
		 * Input indexes in [EndLeftOverlap -> EndRightOverlap) range are mapped together to 1 index: (EndLeftOverlap - StartLeftOverlap - 1)
		 *
		 * @param SourceState - SourceState.
		 * @param InEventIndexInputBuffer - Index of input frame.
		 * @param bLoop - Is Looping?
		 * @return Index of output frame. 
		 */
		TIMESTRETCHMETANODES_API virtual float MapInputEventToOutputBufferFrame(const FSourceBufferState& SourceState, const int32 InEventIndexInputBuffer, const bool bLoop) const;

		/** Get number of input frame needed to produce number of output frame with padding
		 *
		 * @param SourceState - Current buffer state.
		 * @param InNumOutputFrame Number of output frame
		 * @param bLoop - Is Looping?
		 * @return Number of input frame needed
		 */
		TIMESTRETCHMETANODES_API virtual int32 GetNumInputFrameToProduceNumOutPutFrame(const FSourceBufferState& SourceState, const int32 InNumOutputFrame, const bool bLoop) const;

		/** Get number of input frame needed to produce number of output frame with MAXIMUM settings
		 *
		 * @param InNumOutputFrame - Index of output frame.
		 * @param MaxFrameRatio - Stretch frame ratio.		 
		 * @return Number of input frame needed
		 */
		TIMESTRETCHMETANODES_API virtual int32 GetInputFrameCapacityForMaxFrameRatio(const int32 InNumOutputFrame, const float MaxFrameRatio) const;
		
		/** Consumes audio from the input buffer and produces audio in the output buffer.
		 * The desired number of frames to produce is determined by the output audio buffer
		 * size. For the desired number of samples to be produced, the input audio must have the minimum 
		 * number of frames needed to produce the output frames (see `GetNumInputFramesNeededToProduceOutputFrames(...)`).
		 * Input samples which are no longer needed are removed from the input buffer.
		 *
		 * @param InAudio - Multichannel circular buffer of input audio.
		 * @param OutAudio - Multichannel buffer of output audio.
		 * @param SourceState - Current state of the source buffer.
		 *
		 * @return Actual number of frames produced. 
		 */
		TIMESTRETCHMETANODES_API int32 ProcessAndConsumeAudio(MultiChannelBufferCustom::FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio, const FSourceBufferState& SourceState, const bool bLoop);
		
		TIMESTRETCHMETANODES_API static int32 GetEvenWindowSize(float InSamplingRate, float InFrameLengthSecond);
		
		TIMESTRETCHMETANODES_API static void GenerateHannWindow(float* WindowBuffer, const int32 NumFrames, const bool bIsPeriodic);
		
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetCurrentPaddingLength()  const { return CurrentPadding; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetMaxPaddingLength()  const { return MaxPadLength; }
		TIMESTRETCHMETANODES_API FORCEINLINE float GetCurrentFrameRatio()  const { return CurrentFrameRatio; }
		TIMESTRETCHMETANODES_API FORCEINLINE float GetTargetFrameRatio()  const { return TargetFrameRatio; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetCurrentLeftStartOverlapIndexInWave()  const { return CurrentLeftStartOverlapIndexInWave; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetCurrentLeftStartOverlapIndexInBuffer()  const { return CurrentLeftStartOverlapIndexInBuffer; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetCurrentAnalysisSize()  const { return CurrentAnalysisSize; }
		TIMESTRETCHMETANODES_API FORCEINLINE float GetTargetAnalysisSize()  const { return TargetAnalysisSize; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetHalfWinSize()  const { return HalfWinSize; }
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetNumInputSamplesProcessed() const { return NumInputSamplesProcessed; }
		
		TIMESTRETCHMETANODES_API Audio::FAlignedFloatBuffer* GetCurrentWindowBuffer();

		static constexpr float EqualErrorTolerance = 0.0001f;
		
	protected:

		virtual int32 GetSafeNumOutputFrames(const FSourceBufferState& SourceState, bool bLoop, int32 NumAvailableInputFrames) const;

		/**
		 * @brief Get current processed frame index in wave with padding
		 * @param SourceState State of input buffer 
		 * @param bLooping Is looping or not
		 * @return Index in wave of current frame being processed
		 */
		int32 GetCurrentFrameIndexInWavePadding(const FSourceBufferState& SourceState, const bool bLooping) const;
		
		virtual int32 AdvanceCurrentStartLeftOverlapIndex(const FSourceBufferState& SourceState, const bool bLoop);

		virtual int32 GetRightOverlapInBuffer(const bool bUpdateOverlap, const FSourceBufferState& SourceState, const bool bLoop, FMultichannelCircularBufferRand& InAudio);

		virtual int32 ProcessAndConsumeAudioInternal(FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio, const FSourceBufferState& SourceState, const bool bLoop);
		virtual int32 ProcessChannelAudioInternal(FMultichannelCircularBufferRand& InAudio, FMultichannelBufferView& OutAudio, const FSourceBufferState& SourceState, const bool bLoop, int32 NumOutputFrames);
		
		int32 CurrentLeftStartOverlapIndexInWave;
		int32 CurrentLeftStartOverlapIndexInBuffer;
		
		int32 LeftOverlapInBuffer = 0;
		int32 RightOverlapInBuffer = 0;
		
		Audio::FAlignedFloatBuffer LeftOverlapBuffer;
		Audio::FAlignedFloatBuffer RightOverlapBuffer;
		
		int32 CurrentPadding = 0;
		int32 NumInputSamplesProcessed;

		int32 MaxPadLength;
		
	private:
		
		void OverlapAdd(FMultichannelCircularBufferRand& InAudio, TArrayView<const float>& LeftWin, TArrayView<const float>& RightWin,
						const int32 NumProcessedFrames, FMultichannelBufferView& OutBuffer);

		// The first usage of this node will set the default window buffer size
		// Subsequent nodes if differ will have their own window buffer
		Audio::FAlignedFloatBuffer* InitWindowBuffer(float InSamplingRate, float InFrameLengthSecond);
		
		int32 NumChannels = 0;
		float SamplingRate = 0.f;
		float FrameLengthSec = 0.f;

		int32 WinSize;
		int32 HalfWinSize;
		
		float CurrentFrameRatio = 1.f;
		float TargetFrameRatio = 1.f;
		
		int32 CurrentAnalysisSize;
		int32 TargetAnalysisSize;
		
		// Default window buff for all nodes of this class with the same default Window Size which is decided when the first node is initialized
		static TMap<int32, FAlignedFloatBuffer> ClassWindowBuffers;

		// Point to ClassWindowBuffer if the sampling rate is the same as first initialized node. Otherwise, point to NodeWindowBuffer.
		FAlignedFloatBuffer* CurrentWindowBuffer = nullptr;

		int32 OutFrameStart = 0;
	};
}
