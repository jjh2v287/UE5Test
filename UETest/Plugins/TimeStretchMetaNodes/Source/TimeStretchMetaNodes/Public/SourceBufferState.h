// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "CustomWaveProxyReader.h"
#include "MultiChannelBufferCustom.h"

namespace LBSTimeStretch
{
	using namespace Audio;
	using namespace MultiChannelBufferCustom;

	/** FSourceBufferState tracks the current frame and loop indices held in 
			 * a circular buffer. It describes how the content of a circular buffer
			 * relates to the frame indices of an FCustomWaveProxyReader 
			 *
			 * FSourceBufferState is tied to the implementation of the FCustomWaveProxyReader
			 * and TCircularAudioBuffer<>, and thus does not serve much purpose outside
			 * of this wave player node.
			 *
			 * However, it does provide a convenient place to perform frame counting
			 * arithmetic that would otherwise make code more difficult to read.
			 */
	struct FSourceBufferState
	{
		TIMESTRETCHMETANODES_API FSourceBufferState() = default;

		/** Construct a FSourceBufferState
		 *
		 * @param InStartFrameIndex - The frame index in the wave corresponding to the first frame in the circular buffer.
		 * @param InNumFrames - The number of frames in the circular buffer.
		 * @param bIsLooping - True if the wave player is looping, false if not.
		 * @param InLoopStartFrameIndexInWave - Frame index in the wave corresponding to a loop start.
		 * @param InLoopEndFrameIndexInWave - Frame index in the wave corresponding to a loop end.
		 * @param InEOFFrameIndexInWave - Frame index in the wave corresponding to the end of the file.
		 */
		TIMESTRETCHMETANODES_API FSourceBufferState(int32 InStartFrameIndex, int32 InNumFrames, bool bIsLooping,
		                                            int32 InLoopStartFrameIndexInWave, int32 InLoopEndFrameIndexInWave,
		                                            int32 InEOFFrameIndexInWave);

		/** Construct an FSourceBufferState
		 *
		 * @param ProxyReader - The wave proxy reader producing the audio.
		 * @parma InSourceBuffer - The audio buffer holding a range of samples popped from the reader.
		 */
		TIMESTRETCHMETANODES_API FSourceBufferState(const FCustomWaveProxyReader& ProxyReader,
		                                            FMultichannelCircularBufferRand& InSourceBuffer)
			: FSourceBufferState(ProxyReader.GetFrameIndex(), GetMultichannelBufferNumFrames(InSourceBuffer),
			                     ProxyReader.IsLooping(), ProxyReader.GetLoopStartFrameIndex(),
			                     ProxyReader.GetLoopEndFrameIndex(), ProxyReader.GetNumFramesInWave())
		{
		}

		/** Track frames removed from the circular buffer. This generally coincides
		 * with a Pop(...) call to the circular buffer. */
		TIMESTRETCHMETANODES_API void Advance(int32 InNumFrames, bool bIsLooping);

		/** Track frames appended to the source buffer. This generally coincides
		 * with a Push(...) call to the circular buffer. */
		TIMESTRETCHMETANODES_API void Append(int32 InNumFrames, bool bIsLooping);

		/** Update loop frame indices. */
		TIMESTRETCHMETANODES_API void SetLoopFrameIndices(int32 InLoopStartFrameIndexInWave,
		                                                  int32 InLoopEndFrameIndexInWave);

		/** Update loop frame indices. */
		TIMESTRETCHMETANODES_API void SetLoopFrameIndices(const FCustomWaveProxyReader& InProxyReader);

		/** Returns the corresponding frame index in the wave which corresponds
		 * to the first frame in the circular buffer. 
		 */
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetStartFrameIndex() const { return StartFrameIndex; }

		/** Returns the corresponding frame index in the wave which corresponds
		 * to the end frame in the circular buffer (non-inclusive).
		 */
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetEndFrameIndex() const { return EndFrameIndex; }

		/** Returns the frame index in the wave where the loop starts */
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetLoopStartFrameIndexInWave() const
		{
			return LoopStartFrameIndexInWave;
		}

		/** Returns the frame index in the wave where the loop end*/
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetLoopEndFrameIndexInWave() const
		{
			return LoopEndFrameIndexInWave;
		}

		/** Returns the end-of-file frame index in the wave. Equal to number of wave frame if loop end is negative/*/
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetEOFFrameIndexInWave() const { return EOFFrameIndexInWave; }

		/** Returns the frame index in the circular buffer which represents
		 * the end of file in the wave. */
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetEOFFrameIndexInBuffer() const { return EOFFrameIndexInBuffer; }

		/** Returns the frame index in the circular buffer which represents
		 * the ending loop frame index in the wave. */
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetLoopEndFrameIndexInBuffer() const
		{
			return LoopEndFrameIndexInBuffer;
		}

		/** Returns the total number of looping times during current play back. */
		TIMESTRETCHMETANODES_API FORCEINLINE int32 GetLoopCount() const { return LoopCount; }

		/** Returns the ratio of the current frame index divided by the total
		 * number of frames in the wave. */
		TIMESTRETCHMETANODES_API FORCEINLINE float GetPlaybackFraction() const
		{
			const float PlaybackFraction = static_cast<float>(StartFrameIndex) / FMath::Max(
				static_cast<float>(EOFFrameIndexInWave), 1.f);
			return FMath::Max(0.f, PlaybackFraction);
		}

		/** Returns the ratio of the relative position of the current frame 
		 * index to the start loop frame index, divided by the number of frames
		 * in the loop.
		 * This value can be negative if the current frame index is less
		 * than the first loop frame index. 
		 */
		TIMESTRETCHMETANODES_API FORCEINLINE float GetLoopFraction() const
		{
			const float LoopNumFrames = static_cast<float>(FMath::Max(
				1, LoopEndFrameIndexInWave - LoopStartFrameIndexInWave));
			const float LoopRelativeLocation = static_cast<float>(StartFrameIndex - LoopStartFrameIndexInWave);

			return LoopRelativeLocation / LoopNumFrames;
		}

		/** Map a index representing a frame in a wave file to an index representing
		 * a frame in the associated circular buffer. */
		TIMESTRETCHMETANODES_API FORCEINLINE int32 MapFrameInWaveToFrameInBuffer(
			int32 InFrameIndexInWave, bool bIsLooping) const
		{
			if (!bIsLooping || (InFrameIndexInWave >= StartFrameIndex))
			{
				return InFrameIndexInWave - StartFrameIndex;
			}
			else
			{
				const int32 NumFramesFromStartToLoopEnd = LoopEndFrameIndexInWave - StartFrameIndex;
				const int32 NumFramesFromLoopStartToFrameIndex = InFrameIndexInWave - LoopStartFrameIndexInWave;
				return NumFramesFromStartToLoopEnd + NumFramesFromLoopStartToFrameIndex;
			}
		}

		/** Is loop from first frame to final frame of wave */
		TIMESTRETCHMETANODES_API FORCEINLINE bool IsPerfectLoop() const
		{
			return (LoopStartFrameIndexInWave == 0) && (LoopEndFrameIndexInWave == EOFFrameIndexInWave);
		}

	private:
		int32 WrapLoop(int32 InSourceFrameIndex);

		int32 StartFrameIndex = INDEX_NONE;
		int32 EndFrameIndex = INDEX_NONE;
		int32 EOFFrameIndexInBuffer = INDEX_NONE;
		int32 LoopEndFrameIndexInBuffer = INDEX_NONE;
		int32 EOFFrameIndexInWave = INDEX_NONE;
		int32 LoopStartFrameIndexInWave = INDEX_NONE;
		int32 LoopEndFrameIndexInWave = INDEX_NONE;
		int32 LoopCount = 0;
	};

	int32 GetCuePointFrame(const FSoundWaveCuePoint& InPoint);

	/** FSourceEvents contains the frame indices of wave events. 
	 * Indices are INDEX_NONE if they are unset. 
	 */
	struct FSourceEvents
	{
		/** Frame index of a loop end. */
		int32 OnLoopFrameIndex = INDEX_NONE;
		/** Frame index of an end of file. */
		int32 OnEOFFrameIndex = INDEX_NONE;
		/** Frame index of a cue points. */
		int32 OnCuePointFrameIndex = INDEX_NONE;
		/** Cue point associated with OnCuePointFrameIndex. */
		const FSoundWaveCuePoint* CuePoint = nullptr;
		/** Cue point index associated with a stored array of CuePoint we find from. */
		int32 CuePointIndex;
			
		/** Clear all frame indices and associated data. */
		void Reset();
	};
	
	struct FInOutSegmentMap
	{
		//Start proccessed frame
		int32 StartFrame;
		//Index of non-inclusive end frame
		int32 NumInput;
		//Number of output data with this segment
		int32 NumOutput;

		TIMESTRETCHMETANODES_API FInOutSegmentMap(int32 InStartFrame, int32 InNumInput, int32 InNumOutput)
		: StartFrame(InStartFrame), NumInput(InNumInput), NumOutput(InNumOutput) { }

		TIMESTRETCHMETANODES_API void Append(int32 InStartFrame, int32 InNumInput, int32 InNumOutput);
		TIMESTRETCHMETANODES_API void Reset();
	};
}
