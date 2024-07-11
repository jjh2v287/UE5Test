// Copyright 2023 Le Binh Son, All Rights Reserved.

#include "CustomEngineNodesName.h"
#include "CustomWaveProxyReader.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/ConvertDeinterleave.h"
#include "DSP/MultichannelBuffer.h"
#include "MetasoundBuildError.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundLog.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "MetasoundWave.h"
#include "MetasoundStandardNodesCategories.h"
#include "MultiChannelWOLA.h"
#include "SourceBufferState.h"
#include "TimeStretchLog.h"
#include "MultiChannelBufferCustom.h"

#define LOCTEXT_NAMESPACE "LBS_TimeStretchSimple"

namespace LBSTimeStretch
{
	using namespace Metasound;
	
	namespace TimeStretchWOLAVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Play the wave player.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "Stop the wave player.")
		METASOUND_PARAM(InputWaveAsset, "Wave Asset", "The wave asset to be real-time decoded.")
		METASOUND_PARAM(InputStartTime, "Start Time", "Time into the wave asset to start (seek) the wave asset.")
		METASOUND_PARAM(InputStretchRatio, "Stretch Ratio", "The time stretch ratio. Min = 0.5. Max = 2. >1 mean faster playback rate.")
		//METASOUND_PARAM(InputFrameLengthSec, "Frame Length", "The length of each processing frame in seconds. Higher values generally produce better quality but require more memory. Min = 0.04. Max = 0.12")
		METASOUND_PARAM(InputDeviationStepPercent, "Deviation Step Percent", "The step size when calculating correlation. Higher values reduce quality but have faster processing speed. Min = 0 (highest quality with step size = 1). Max = 0.25.")
		METASOUND_PARAM(InputLoop, "Loop", "Whether or not to loop between the start and specified end times.")
		METASOUND_PARAM(InputLoopStart, "Loop Start", "When to start the loop.")
		METASOUND_PARAM(InputLoopDuration, "Loop Duration", "The duration of the loop when wave player is enabled for looping. A negative value will loop the whole wave asset.")
		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the wave played has finished playing or Stop is triggered.")
		METASOUND_PARAM(OutputTriggerOnNearlyDone, "On Nearly Finished", "Triggers when the wave played has almost finished playing (the block before it finishes). Allows time for logic to trigger different variations to play seamlessly.")
		METASOUND_PARAM(OutputTriggerOnLooped, "On Looped", "Triggers when the wave player has looped.")
		METASOUND_PARAM(OutputTriggerOnCuePoint, "On Cue Point", "Triggers when a wave cue point was hit during playback.")
		METASOUND_PARAM(OutputCuePointID, "Cue Point ID", "The cue point ID that was triggered.")
		METASOUND_PARAM(OutputCuePointLabel, "Cue Point Label", "The cue point label that was triggered (if there was a label parsed in the imported .wav file).")
		METASOUND_PARAM(OutputLoopRatio, "Loop Percent", "Returns the current playback location as a ratio of the loop (0-1) if looping is enabled.")
		METASOUND_PARAM(OutputPlaybackLocation, "Playback Location", "Returns the absolute position of the wave playback as a ratio of wave duration (0-1).")
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
		METASOUND_PARAM(OutputAudioFrontRight, "Out Front Right", "The front right channel audio output.")
		METASOUND_PARAM(OutputAudioFrontLeft, "Out Front Left", "The front left channel audio output.")
		METASOUND_PARAM(OutputAudioFrontCenter, "Out Front Center", "The front center channel audio output.")
		METASOUND_PARAM(OutputAudioLowFrequency, "Out Low Frequency", "The low frequency channel audio output.")
		METASOUND_PARAM(OutputAudioSideRight, "Out Side Right", "The side right channel audio output.")
		METASOUND_PARAM(OutputAudioSideLeft, "Out Side Left", "The side left channel audio output.")
		METASOUND_PARAM(OutputAudioBackRight, "Out Back Right", "The back right channel audio output.")
		METASOUND_PARAM(OutputAudioBackLeft, "Out Back Left", "The back left channel audio output.")
	}

	struct FTimeStretchWOLAOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		FWaveAssetReadRef WaveAsset;
		FTimeReadRef StartTime;
		FFloatReadRef StretchRatio;
		//FFloatReadRef FrameLengthSec;
		FFloatReadRef DeviationStepPercent;
		FBoolReadRef bLoop;
		FTimeReadRef LoopStartTime;
		FTimeReadRef LoopDuration;
	};

	/** MetaSound operator for the wave player node. */
	class FTimeStretchWOLAOperator : public TExecutableOperator<FTimeStretchWOLAOperator>
	{	
	public:
		//MinStretchRatio < 0.5f can cause same cue to be broadcasted multiple times. 
		static constexpr float MinStretchRatio = 0.5f;
		static constexpr float MaxStretchRatio = 2.0f;

		static constexpr float MinTotalStretchRatio = 0.25f;
		static constexpr float MaxTotalStretchRatio = 4.0f;

		static constexpr float MinFrameLengthSec = 0.04f;
		static constexpr float MaxFrameLengthSec = 0.12f;
		
		static constexpr float MinDeviationStepPercent = 0.f;
		static constexpr float MaxDeviationStepPercent = 0.25f;
		
		// Maximum decode size in frames. 
		static constexpr int32 MaxDecodeSizeInFrames = 8192;
		// Block size for deinterleaving audio. 
		static constexpr int32 DeinterleaveBlockSizeInFrames = 512;

		FTimeStretchWOLAOperator(const FTimeStretchWOLAOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, WaveAsset(InArgs.WaveAsset)
			, StartTime(InArgs.StartTime)
			, StretchRatio(InArgs.StretchRatio)
			//, FrameLengthSec(InArgs.FrameLengthSec)
			, DeviationStepPercent(InArgs.DeviationStepPercent)
			, bLoop(InArgs.bLoop)
			, LoopStartTime(InArgs.LoopStartTime)
			, LoopDuration(InArgs.LoopDuration)
			, TriggerOnDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
			, TriggerOnNearlyDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
			, TriggerOnLooped(FTriggerWriteRef::CreateNew(InArgs.Settings))
			, TriggerOnCuePoint(FTriggerWriteRef::CreateNew(InArgs.Settings))
			, CuePointID(FInt32WriteRef::CreateNew(0))
			, CuePointLabel(FStringWriteRef::CreateNew(TEXT("")))
			, LoopPercent(FFloatWriteRef::CreateNew(0.0f))
			, PlaybackLocation(FFloatWriteRef::CreateNew(0.0f))
		{
			NumOutputChannels = InArgs.OutputAudioVertices.Num();
			SamplingRate = OperatorSettings.GetSampleRate();
			for (const FOutputDataVertex& OutputAudioVertex : InArgs.OutputAudioVertices)
			{
				OutputAudioBufferVertexNames.Add(OutputAudioVertex.VertexName);

				FAudioBufferWriteRef AudioBuffer = FAudioBufferWriteRef::CreateNew(InArgs.Settings);
				OutputAudioBuffers.Add(AudioBuffer);

				// Hold on to a view of the output audio. Audio buffers are only writable
				// by this object and will not be reallocated. 
				OutputAudioView.Emplace(AudioBuffer->GetData(), AudioBuffer->Num());
			}
		}

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace TimeStretchWOLAVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputWaveAsset), WaveAsset);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputStartTime), StartTime);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputStretchRatio), StretchRatio);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDeviationStepPercent), DeviationStepPercent);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputLoop), bLoop);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputLoopStart), LoopStartTime);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputLoopDuration), LoopDuration);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace TimeStretchWOLAVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnNearlyDone), TriggerOnNearlyDone);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnLooped), TriggerOnLooped);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnCuePoint), TriggerOnCuePoint);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputCuePointID), CuePointID);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputCuePointLabel), CuePointLabel);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputLoopRatio), LoopPercent);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputPlaybackLocation), PlaybackLocation);
			
			check(OutputAudioBuffers.Num() == OutputAudioBufferVertexNames.Num());

			for (int32 i = 0; i < OutputAudioBuffers.Num(); i++)
			{
				InOutVertexData.BindReadVertex(OutputAudioBufferVertexNames[i], OutputAudioBuffers[i]);
			}
		}

		virtual FDataReferenceCollection GetInputs() const override
		{
			// This should never be called. Bind(...) is called instead. This method
			// exists as a stop-gap until the API can be deprecated and removed.
			checkNoEntry();
			return {};
		}

		virtual FDataReferenceCollection GetOutputs() const override
		{
			// This should never be called. Bind(...) is called instead. This method
			// exists as a stop-gap until the API can be deprecated and removed.
			checkNoEntry();
			return {};
		}

		void Execute()
		{
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::FWavePlayerOperator::Execute);

			// Advance all triggers owned by this operator. 
			TriggerOnDone->AdvanceBlock();
			TriggerOnNearlyDone->AdvanceBlock();
			TriggerOnCuePoint->AdvanceBlock();
			TriggerOnLooped->AdvanceBlock();

			// Update wave proxy reader with any new looping bounds. 
			if (WaveProxyReader.IsValid())
			{
				WaveProxyReader->SetIsLooping(*bLoop);
				WaveProxyReader->SetLoopStartTime(LoopStartTime->GetSeconds());
				WaveProxyReader->SetLoopDuration(LoopDuration->GetSeconds());
				SourceState.SetLoopFrameIndices(*WaveProxyReader);
			}

			// Update resampler with new frame ratio. 
			if (Resampler.IsValid())
			{
				Resampler->SetFrameRatio(GetFrameRatio(), SourceState);
			}

			// zero output buffers
			for (const FAudioBufferWriteRef& OutputBuffer : OutputAudioBuffers)
			{
				FMemory::Memzero(OutputBuffer->GetData(), OperatorSettings.GetNumFramesPerBlock() * sizeof(float));
			}

			// Performs execution per sub block based on triggers.
			ExecuteSubblocks();

			// Updates output playhead information
			UpdatePlaybackLocation();
		}
		
		void Reset(const IOperator::FResetParams& InParams)
		{
			TriggerOnDone->Reset();
			TriggerOnNearlyDone->Reset();
			TriggerOnLooped->Reset();
			TriggerOnCuePoint->Reset();

			*CuePointID = 0;
			*CuePointLabel = TEXT("");
			*LoopPercent = 0;
			*PlaybackLocation = 0;
			
			for (const FAudioBufferWriteRef& BufferRef : OutputAudioBuffers)
			{
				BufferRef->Zero();
			}

			WaveProxyReader.Reset();
			ConvertDeinterleave.Reset();
			Resampler.Reset();

			SortedCuePoints.Reset();
			for (auto& Buffer : SourceCircularBuffer)
			{
				Buffer.SetNum(0);
			}

			SourceState = FSourceBufferState();
			SampleRateFrameRatio = 1.f;
			bOnNearlyDoneTriggeredForWave = false;
			bIsPlaying = false;
		}

	private:

		void ExecuteSubblocks()
		{
			// Parse triggers and render audio
			int32 PlayTrigIndex = 0;
			int32 NextPlayFrame = 0;
			const int32 NumPlayTrigs = PlayTrigger->NumTriggeredInBlock();

			int32 StopTrigIndex = 0;
			int32 NextStopFrame = 0;
			const int32 NumStopTrigs = StopTrigger->NumTriggeredInBlock();

			int32 CurrAudioFrame = 0;
			int32 NextAudioFrame = 0;
			const int32 LastAudioFrame = OperatorSettings.GetNumFramesPerBlock() - 1;
			const int32 NoTrigger = OperatorSettings.GetNumFramesPerBlock() << 1; //Same as multiply by 2

			while (NextAudioFrame < LastAudioFrame)
			{
				// get the next Play and Stop indices
				// (play)
				if (PlayTrigIndex < NumPlayTrigs)
				{
					NextPlayFrame = (*PlayTrigger)[PlayTrigIndex];
				}
				else
				{
					NextPlayFrame = NoTrigger;
				}

				// (stop)
				if (StopTrigIndex < NumStopTrigs)
				{
					NextStopFrame = (*StopTrigger)[StopTrigIndex];
				}
				else
				{
					NextStopFrame = NoTrigger;
				}

				// determine the next audio frame we are going to render up to
				NextAudioFrame = FMath::Min(NextPlayFrame, NextStopFrame);

				// no more triggers, rendering to the end of the block
				if (NextAudioFrame == NoTrigger)
				{
					NextAudioFrame = OperatorSettings.GetNumFramesPerBlock();
				}

				// render audio (while loop handles looping audio)
				while (CurrAudioFrame != NextAudioFrame)
				{
					if (bIsPlaying)
					{
						RenderFrameRange(CurrAudioFrame, NextAudioFrame);
					}
					CurrAudioFrame = NextAudioFrame;
				}

				// execute the next trigger
				if (CurrAudioFrame == NextPlayFrame)
				{
					StartPlaying();
					++PlayTrigIndex;
				}

				if (CurrAudioFrame == NextStopFrame)
				{
					bIsPlaying = false;
					TriggerOnDone->TriggerFrame(CurrAudioFrame);
					++StopTrigIndex;
				}
			}
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			using namespace Audio;

			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::FWavePlayerOperator::RenderFrameRange);

			// Assume this is set to true and checked by outside callers
			check(bIsPlaying);

			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate > 0)
			{
				// Generate audio
				FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);

				//Store first frame to be processed so we can map events later
				const int32 CurrentFirstProcessFrame = SourceState.GetStartFrameIndex() + Resampler->GetCurrentPaddingLength();
				
				GeneratePitchedAudio(BufferToGenerate);

				// Trigger any events that occur within this frame range
				TriggerIncomingEvents(CurrentFirstProcessFrame, StartFrame, NumFramesToGenerate);
				
				// Check if the source is empty.
				if (!(*bLoop))
				{
					bIsPlaying = (SourceState.GetStartFrameIndex() <= SourceState.GetEOFFrameIndexInWave());
				}
			}
		}

		void UpdatePlaybackLocation()
		{
			*PlaybackLocation = SourceState.GetPlaybackFraction();

			if (*bLoop)
			{
				*LoopPercent = SourceState.GetLoopFraction();
			}
			else
			{
				*LoopPercent = 0.f;
			}
		}

		float GetStretchRatioClamped() const
		{
			return FMath::Clamp(*StretchRatio, MinStretchRatio, MaxStretchRatio);
		}

		float GetFrameLenghtSecClamped() const
		{
			return FMath::Clamp(FrameLengthSec, MinFrameLengthSec, MaxFrameLengthSec);
		}
		
		float GetDeviationStepPercentClamped() const
		{
			return FMath::Clamp(*DeviationStepPercent, MinDeviationStepPercent, MaxDeviationStepPercent);
		}
		
		// Updates the sample rate frame ratio. Used when a new wave proxy reader
		// is created. 
		void UpdateSampleRateFrameRatio() 
		{
			SampleRateFrameRatio = 1.f;

			if (WaveProxyReader.IsValid())
			{
				float SourceSampleRate = WaveProxyReader->GetSampleRate();
				if (SourceSampleRate > 0.f)
				{
					float TargetSampleRate = OperatorSettings.GetSampleRate();
					if (TargetSampleRate > 0.f)
					{
						SampleRateFrameRatio = SourceSampleRate / OperatorSettings.GetSampleRate();
					}
				}
			}
		}

		float GetSampleRateFrameRatio() const
		{
			return SampleRateFrameRatio;
		}

		float GetFrameRatio() const
		{
			return FMath::Clamp(GetSampleRateFrameRatio() * GetStretchRatioClamped(), MinTotalStretchRatio, MaxTotalStretchRatio);
		}

		float GetMaxTimeStretchRatio() const
		{
			return MaxStretchRatio;
		}

		float GetMaxFrameRatio() const
		{
			return FMath::Min(GetSampleRateFrameRatio() * GetMaxTimeStretchRatio(), MaxTotalStretchRatio);
		}

		// Start playing the current wave by creating a wave proxy reader and
		// recreating the DSP stack.
		void StartPlaying()
		{
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::FWavePlayerOperator::StartPlaying);

			// MetasoundWavePlayerNode DSP Stack
			//
			// Legend:
			// 	[ObjectName] - An Object which generates or process audio.
			// 	(BufferName) - A buffer which holds audio.
			//
			// [WaveProxyReader]->(InterleavedBuffer)->[ConvertDeinterleave]->(DeinterleavedBuffer)->(SourceCircularBuffer)->[Resampler]->(AudioOutputView)
			//

			// Copy the wave asset off on init in case the user changes it while we're playing it.
			// We'll only check for new wave assets when the current one finishes for sample accurate concantenation
			CurrentWaveAsset = *WaveAsset;
			FSoundWaveProxyPtr WaveProxy = CurrentWaveAsset.GetSoundWaveProxy();

			bOnNearlyDoneTriggeredForWave = false;
			bIsPlaying = false;

			// Reset dsp stack.
			ResetSourceBufferAndState();
			WaveProxyReader.Reset();
			ConvertDeinterleave.Reset();
			Resampler.Reset();
			SortedCuePoints.Reset();

			if (WaveProxy.IsValid())
			{
				UE_LOG(LogMetaSound, Verbose, TEXT("Starting Sound: '%s'"), *CurrentWaveAsset->GetFName().ToString());

				// Create local sorted copy of cue points.
				SortedCuePoints = WaveProxy->GetCuePoints();
				Algo::SortBy(SortedCuePoints, LBSTimeStretch::GetCuePointFrame);
				
				// Create the wave proxy reader.
				FCustomWaveProxyReader::FSettings WaveReaderSettings;
				WaveReaderSettings.MaxDecodeSizeInFrames = MaxDecodeSizeInFrames;
				WaveReaderSettings.StartTimeInSeconds = StartTime->GetSeconds();
				WaveReaderSettings.LoopStartTimeInSeconds = LoopStartTime->GetSeconds();
				WaveReaderSettings.LoopDurationInSeconds = LoopDuration->GetSeconds(); 
				WaveReaderSettings.bIsLooping = *bLoop;

				WaveProxyReader = FCustomWaveProxyReader::Create(WaveProxy.ToSharedRef(), WaveReaderSettings, 2.0f * GetFrameLenghtSecClamped());
				
				if (WaveProxyReader.IsValid())
				{
					UpdateSampleRateFrameRatio();
					int32 WaveProxyNumChannels = WaveProxyReader->GetNumChannels();

					if (WaveProxyNumChannels > 0)
					{
						// Create buffer for interleaved audio
						int32 InterleavedBufferNumSamples = WaveProxyNumChannels * DeinterleaveBlockSizeInFrames;
						InterleavedBuffer.Reset(InterleavedBufferNumSamples);
						InterleavedBuffer.AddUninitialized(InterleavedBufferNumSamples);

						NumDeinterleaveChannels = NumOutputChannels;

						// Create algorithm for channel conversion and deinterleave 
						Audio::FConvertDeinterleaveParams ConvertDeinterleaveParams;
						ConvertDeinterleaveParams.NumInputChannels = WaveProxyReader->GetNumChannels();
						ConvertDeinterleaveParams.NumOutputChannels = NumDeinterleaveChannels;
						// Original implementation of MetaSound WavePlayer upmixed 
						// mono using FullVolume. In the future, the mono upmix 
						// method may be exposed as a node input to facilitate 
						// better control.
						ConvertDeinterleaveParams.MonoUpmixMethod = Audio::EChannelMapMonoUpmixMethod::FullVolume;
						ConvertDeinterleave = Audio::IConvertDeinterleave::Create(ConvertDeinterleaveParams);
						Audio::SetMultichannelBufferSize(NumDeinterleaveChannels, DeinterleaveBlockSizeInFrames, DeinterleavedBuffer);

						// Create a resampler.
						Resampler = MakeUnique<FMultichannelWOLA>(NumDeinterleaveChannels, SamplingRate, GetFrameLenghtSecClamped(), GetFrameRatio(),
																WaveProxyReader->GetFrameIndex(), GetDeviationStepPercentClamped(), OperatorSettings.GetNumFramesPerBlock());
						
						// Initialize source buffer
						int32 FrameCapacity = Resampler->GetInputFrameCapacityForMaxFrameRatio(OperatorSettings.GetNumFramesPerBlock(), GetMaxFrameRatio());
						LBSTimeStretch::SetMultichannelCircularBufferCapacity(NumOutputChannels, FrameCapacity + DeinterleaveBlockSizeInFrames, SourceCircularBuffer);
						SourceState = FSourceBufferState(*WaveProxyReader, SourceCircularBuffer);

						// Need to add upmixing if this is not true
						check(NumDeinterleaveChannels == NumOutputChannels);
					}
				}
			}

			// If everything was created successfully, start playing.
			bIsPlaying = WaveProxyReader.IsValid() && ConvertDeinterleave.IsValid() && Resampler.IsValid();
		}

		/** Removes all samples from the source buffer and resets SourceState. */
		void ResetSourceBufferAndState()
		{
			using namespace Audio;

			SourceState = FSourceBufferState();
			for (TCircularAudioBufferCustom<float>& ChannelCircularBuffer : SourceCircularBuffer)
			{
				ChannelCircularBuffer.SetNum(0);
			}
		}

		/** Generates audio from the wave proxy reader.
		 *
		 * @param OutBuffer - Buffer to place generated audio.
		 * @param OutSourceState - Source state for tracking state of OutBuffer.
		 */
		void GenerateSourceAudio(FMultichannelCircularBufferRand& OutBuffer, FSourceBufferState& OutSourceState)
		{
			if (bIsPlaying)
			{
				const int32 NumExistingFrames = GetMultichannelBufferNumFrames(OutBuffer);
				const int32 NumSamplesToGenerate = DeinterleaveBlockSizeInFrames * WaveProxyReader->GetNumChannels();
 				check(NumSamplesToGenerate == InterleavedBuffer.Num())

				WaveProxyReader->PopAudio(InterleavedBuffer);
				ConvertDeinterleave->ProcessAudio(InterleavedBuffer, DeinterleavedBuffer);

				for (int32 ChannelIndex = 0; ChannelIndex < NumDeinterleaveChannels; ChannelIndex++)
				{
					OutBuffer[ChannelIndex].Push(DeinterleavedBuffer[ChannelIndex]);
				}
				OutSourceState.Append(DeinterleaveBlockSizeInFrames, *bLoop);
			}
			else
			{
				OutSourceState = FSourceBufferState();
			}
		}


		/** Map if input events to output buffer if they are in processed samples range. 
		 */
		void MapSourceEventsIfInRange(const int32 FirstProcessFrame, LBSTimeStretch::FSourceEvents& OutEvents)
		{
			OutEvents.Reset();
			// Loop end
			if (*bLoop)
			{
				OutEvents.OnLoopFrameIndex = Resampler->GetPreviousInputEventIndexInOutputBuffer(FirstProcessFrame, SourceState, SourceState.GetLoopEndFrameIndexInWave() - 1, *bLoop);
			}

			// End of file
			OutEvents.OnEOFFrameIndex = Resampler->GetPreviousInputEventIndexInOutputBuffer(FirstProcessFrame, SourceState, SourceState.GetEOFFrameIndexInWave() - 1, *bLoop);
			
			// Map Cue point. Since only one can be mapped, map the first one found.
			// The first cue point found has the best chance of being rendered.
			int32 NumInput;
			int32 NumOutput;
			const int32 PrevSegmentIdx = Resampler->FindSegmentMap(FirstProcessFrame, NumInput, NumOutput);
			const FInOutSegmentMap PrevSegment = Resampler->GetSegment(PrevSegmentIdx);
			int32 SearchStartFrameIndexInWave = PrevSegment.StartFrame;
			int32 SearchEndFrameIndexInWave = SearchStartFrameIndexInWave + NumInput;

			const bool bFramesCrossLoopBoundary = *bLoop && (OutEvents.OnLoopFrameIndex != INDEX_NONE);
			if (bFramesCrossLoopBoundary)
			{
				SearchEndFrameIndexInWave = SearchStartFrameIndexInWave + SourceState.GetLoopEndFrameIndexInBuffer();
			}

			FindCuePoint(SearchStartFrameIndexInWave, SearchEndFrameIndexInWave, OutEvents);

			if (bFramesCrossLoopBoundary)
			{
				SearchStartFrameIndexInWave = SourceState.GetLoopStartFrameIndexInWave();
				const int32 RemainingFrames = NumInput - SourceState.GetLoopEndFrameIndexInBuffer();
				SearchEndFrameIndexInWave = RemainingFrames;

				// Only override OutEvents.CuePoint if one exists in this subsection
				// of the buffer.
				if (OutEvents.CuePoint == nullptr)
					FindCuePoint(SearchStartFrameIndexInWave, SearchEndFrameIndexInWave, OutEvents);
			}

			if (OutEvents.CuePoint != nullptr)
			{
				OutEvents.OnCuePointFrameIndex = Resampler->GetPreviousInputEventIndexInOutputBuffer(FirstProcessFrame, SourceState, OutEvents.CuePoint->FramePosition, *bLoop);
			}
		}

		// Search for cue points in frame range. Return the first cue point in the frame range.
		void FindCuePoint(int32 InStartFrameInWave, int32 InEndFrameInWave, LBSTimeStretch::FSourceEvents& Events)
		{
			int32 LowerBoundIndex = Algo::LowerBoundBy(SortedCuePoints, InStartFrameInWave, LBSTimeStretch::GetCuePointFrame);
			int32 UpperBoundIndex = Algo::LowerBoundBy(SortedCuePoints, InEndFrameInWave, LBSTimeStretch::GetCuePointFrame);
			
			if (LowerBoundIndex < UpperBoundIndex)
			{
				// Inform about skipped cue points. 1
				for (int32 i = LowerBoundIndex + 1; i < UpperBoundIndex; i++)
				{
					const FSoundWaveCuePoint& CuePoint = SortedCuePoints[i];

					UE_LOG(LogMetaSound, Verbose, TEXT("Skipping cue point \"%s\" at frame %d due to multiple cue points in same render block"), *CuePoint.Label, CuePoint.FramePosition);
				}
				Events.CuePointIndex = LowerBoundIndex;
				Events.CuePoint = &SortedCuePoints[LowerBoundIndex];
			}
		}

		// Trigger input events if they present in output buffer 
		void TriggerIncomingEvents(const int32 FirstProcessFrame, int32 InOperatorStartFrame, int32 InNumOutputFrames)
		{
			LBSTimeStretch::FSourceEvents Events;
			
			// Selectively map events in the source buffer to frame indices in the
			// resampled output buffer. 
			MapSourceEventsIfInRange(FirstProcessFrame, Events);

			// Check whether to trigger loops based on actual number of output frames 
			if (*bLoop)
			{
				if (FMath::IsWithin(Events.OnLoopFrameIndex, 0, InNumOutputFrames))
				{
					TriggerOnLooped->TriggerFrame(InOperatorStartFrame + Events.OnLoopFrameIndex);
				}
			}
			else
			{
				if (FMath::IsWithin(Events.OnEOFFrameIndex, 0, InNumOutputFrames))
				{
					TriggerOnDone->TriggerFrame(InOperatorStartFrame + Events.OnEOFFrameIndex);
				}
				else if(!bOnNearlyDoneTriggeredForWave)
				{
					//Use End Right of current Overlap to determine if we near EOF or not
					//This is more deterministic and safer than trying to map input index to outside of current output buffer
					const int32 EndOverlapFrameIdx = Resampler->GetCurrentLeftStartOverlapIndexInBuffer() - Resampler->GetCurrentDeviation()
													 + Resampler->GetCurrentAnalysisSize() + Resampler->GetMaxDeviation();
					if (SourceState.GetEOFFrameIndexInBuffer() <= EndOverlapFrameIdx)
					{
						TriggerOnNearlyDone->TriggerFrame(InOperatorStartFrame + OperatorSettings.GetNumFramesPerBlock());
						bOnNearlyDoneTriggeredForWave = true;
					}
				}
			}

			if (Events.CuePoint != nullptr)
			{
				if (FMath::IsWithin(Events.OnCuePointFrameIndex, 0, InNumOutputFrames))
				{
					if (!TriggerOnCuePoint->IsTriggeredInBlock())
					{
						*CuePointID = Events.CuePoint->CuePointID;
						*CuePointLabel = Events.CuePoint->Label;
						TriggerOnCuePoint->TriggerFrame(InOperatorStartFrame + Events.OnCuePointFrameIndex);
						if(!(*bLoop))
							SortedCuePoints.RemoveAt(Events.CuePointIndex);
					}
					else
					{
						UE_LOG(LogMetaSound, Verbose, TEXT("Skipping cue point \"%s\" at frame %d due to multiple cue points in same render block"), *Events.CuePoint->Label, Events.CuePoint->FramePosition);
					}
				}
			}
		}


		void GeneratePitchedAudio(Audio::FMultichannelBufferView& OutBuffer)
		{
			using namespace Audio;

			// Outside callers should ensure that bIsPlaying is true if calling this function.
			check(bIsPlaying);

			int32 NumFramesRequested = Audio::GetMultichannelBufferNumFrames(OutBuffer);
			int32 NumSourceFramesAvailable = GetMultichannelBufferNumFrames(SourceCircularBuffer);
			while (NumFramesRequested > 0)
			{
				int32 NumFramesProduced = 0;
				int32 OldNumSourceFramesAvailable = -1;
				const int32 NumSourceFramesNeeded = Resampler->GetNumInputFrameToProduceNumOutPutFrame(SourceState, NumFramesRequested, *bLoop);
				
				while (NumFramesProduced < 1 && OldNumSourceFramesAvailable < NumSourceFramesAvailable)
				{
					if(NumSourceFramesNeeded > NumSourceFramesAvailable)
					{
						// Generate more source audio, but still may not be enough to produce all requested frames.
						GenerateSourceAudio(SourceCircularBuffer, SourceState); //a.k.a decode audio and put it into circular buffer
					}
					OldNumSourceFramesAvailable = NumSourceFramesAvailable;
					NumSourceFramesAvailable = GetMultichannelBufferNumFrames(SourceCircularBuffer);
					checkf((OldNumSourceFramesAvailable != NumSourceFramesAvailable) || (NumSourceFramesAvailable != SourceCircularBuffer[0].GetCapacity()), TEXT("GeneratePitchedAudio: Maximum input audio buffer capacity reach!"));
					// Resample frames. 
					NumFramesProduced = Resampler->ProcessAndConsumeAudio(SourceCircularBuffer, OutBuffer, SourceState, *bLoop);
				}
				if (NumFramesProduced < 1)
				{
					UE_LOG(LogTimeStretch, Error, TEXT("Aborting currently playing metasound wave %s. Failed to produce any resampled audio frames with %d input frames and a frame ratio of %f."), *CurrentWaveAsset->GetFName().ToString(), NumSourceFramesAvailable, GetFrameRatio());
					UE_LOG(LogTimeStretch, Error, TEXT("Currently playing frame index %d. Number of source frames in buffer %d."), SourceState.GetStartFrameIndex() + Resampler->GetCurrentPaddingLength(), NumSourceFramesAvailable);
					bIsPlaying = false;
					break;
				}
				
				// Update sample counters
				int32 NewNumSourceFramesAvailable = GetMultichannelBufferNumFrames(SourceCircularBuffer);
				int32 NumSourceFramesConsumed = NumSourceFramesAvailable - NewNumSourceFramesAvailable;
				NumSourceFramesAvailable = NewNumSourceFramesAvailable;
				NumFramesRequested -= NumFramesProduced;

				SourceState.Advance(NumSourceFramesConsumed, *bLoop);

				// Shift buffer if there are more samples to create
				if (NumFramesRequested > 0)
				{
					ShiftMultichannelBufferView(NumFramesProduced, OutBuffer);
				}
			}
		}

		const FOperatorSettings OperatorSettings;

		// i/o
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		FWaveAssetReadRef WaveAsset;
		FTimeReadRef StartTime;
		FFloatReadRef StretchRatio;
		FFloatReadRef DeviationStepPercent;
		FBoolReadRef bLoop;
		FTimeReadRef LoopStartTime;
		FTimeReadRef LoopDuration;

		FTriggerWriteRef TriggerOnDone;
		FTriggerWriteRef TriggerOnNearlyDone;
		FTriggerWriteRef TriggerOnLooped;
		FTriggerWriteRef TriggerOnCuePoint;
		FInt32WriteRef CuePointID;
		FStringWriteRef CuePointLabel;
		FFloatWriteRef LoopPercent;
		FFloatWriteRef PlaybackLocation;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;

		TUniquePtr<FCustomWaveProxyReader> WaveProxyReader;
		TUniquePtr<Audio::IConvertDeinterleave> ConvertDeinterleave;
		TUniquePtr<FMultichannelWOLA> Resampler;

		FWaveAsset CurrentWaveAsset;
		TArray<FSoundWaveCuePoint> SortedCuePoints;
		Audio::FAlignedFloatBuffer InterleavedBuffer;
		Audio::FMultichannelBuffer DeinterleavedBuffer;
		LBSTimeStretch::FMultichannelCircularBufferRand SourceCircularBuffer;
		Audio::FMultichannelBufferView OutputAudioView;

		FSourceBufferState SourceState;
		float SamplingRate;
		float SampleRateFrameRatio = 1.f;
		int32 NumOutputChannels;
		int32 NumDeinterleaveChannels;
		bool bOnNearlyDoneTriggeredForWave = false;
		bool bIsPlaying = false;
		float FrameLengthSec = 0.08f;
	};

	class FTimeStretchWOLAOperatorFactory : public IOperatorFactory
	{
	public:
		FTimeStretchWOLAOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}

		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace TimeStretchWOLAVertexNames;

			const FInputVertexInterfaceData& Inputs = InParams.InputData;

			FTimeStretchWOLAOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FWaveAsset>(METASOUND_GET_PARAM_NAME(InputWaveAsset)),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputStartTime), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputStretchRatio), InParams.OperatorSettings),
				//Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputFrameLengthSec), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDeviationStepPercent), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputLoop), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputLoopStart), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputLoopDuration), InParams.OperatorSettings)
			};

			return MakeUnique<FTimeStretchWOLAOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};

	
	template<typename AudioChannelConfigurationInfoType>
	class TTimeStretchWOLANode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace TimeStretchWOLAVertexNames;

			// Workaround to override display name of OutputLoopRatio
			static const FDataVertexMetadata OutputLoopRatioMetadata
			{ 
				METASOUND_GET_PARAM_TT(OutputLoopRatio), // description 
				METASOUND_LOCTEXT("OutputLoopRatioNotPercentDisplayName", "Loop Ratio") // display name  
			};

			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
					TInputDataVertex<FWaveAsset>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputWaveAsset)),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputStartTime), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputStretchRatio), 1.f),
//					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFrameLengthSec), 0.08f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDeviationStepPercent), 0.0f),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputLoop), false),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputLoopStart), 0.0f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputLoopDuration), -1.0f)
					),
				FOutputVertexInterface(
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnPlay)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnNearlyDone)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnLooped)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnCuePoint)),
					TOutputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputCuePointID)),
					TOutputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputCuePointLabel)),
					TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME(OutputLoopRatio), OutputLoopRatioMetadata),
					TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputPlaybackLocation))
				)
			);

			// Add audio outputs dependent upon source info.
			for (const FOutputDataVertex& OutputDataVertex : AudioChannelConfigurationInfoType::GetAudioOutputs())
			{
				VertexInterface.GetOutputInterface().Add(OutputDataVertex);
			}

			return VertexInterface;
		}

		static const FNodeClassMetadata& GetNodeInfo()
		{
			auto InitNodeInfo = []() -> FNodeClassMetadata
			{
				FNodeClassMetadata Info;
				Info.ClassName = { CustomEngineNodes::Namespace, TEXT("Time Stretch WOLA"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_TimeStretchWOLANodeDescription", "Plays a wave asset with at different tempo/speed without changing its pitches. "
																								"Slower than TimeStretchSFX node but this node can be used for audio with musical elements.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("WavePlayerSoundKeyword", "Sound"),
				                  METASOUND_LOCTEXT("WavePlayerCueKeyword", "Cue")
				};

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TTimeStretchWOLANode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FTimeStretchWOLAOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TTimeStretchWOLANode(const FNodeInitData& InInitData)
			: TTimeStretchWOLANode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TTimeStretchWOLANode() = default;

		virtual FOperatorFactorySharedRef GetDefaultOperatorFactory() const override
		{
			return Factory;
		}

		virtual const FVertexInterface& GetVertexInterface() const override
		{
			return Interface;
		}

		virtual bool SetVertexInterface(const FVertexInterface& InInterface) override
		{
			return InInterface == Interface;
		}

		virtual bool IsVertexInterfaceSupported(const FVertexInterface& InInterface) const override
		{
			return InInterface == Interface;
		}

	private:
		FOperatorFactorySharedRef Factory;
		FVertexInterface Interface;
	};

	struct FMonoWOLAChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_TimeStretchWOLAMonoNodeDisplayName", "Time Stretch WOLA (Mono)"); }
		static FName GetVariantName() { return CustomEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace TimeStretchWOLAVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoTimeStretchWOLANode = TTimeStretchWOLANode<FMonoWOLAChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoTimeStretchWOLANode);

	struct FStereoWOLAChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_TimeStretchWOLAStereoNodeDisplayName", "Time Stretch WOLA (Stereo)"); }
		static FName GetVariantName() { return CustomEngineNodes::StereoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace TimeStretchWOLAVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			};
		}
	};
	using FStereoTimeStretchWOLANode = TTimeStretchWOLANode<FStereoWOLAChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FStereoTimeStretchWOLANode);

	struct FQuadWOLAChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_TimeStretchWOLAQuadNodeDisplayName", "Time Stretch WOLA (Quad)"); }
		static FName GetVariantName() { return CustomEngineNodes::QuadVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace TimeStretchWOLAVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontRight)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioSideLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioSideRight))
			};
		}
	};
	using FQuadTimeStretchWOLANode = TTimeStretchWOLANode<FQuadWOLAChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FQuadTimeStretchWOLANode);

	struct FFiveDotOneWOLAChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_TimeStretchWOLAFiveDotOneNodeDisplayName", "Time Stretch WOLA (5.1)"); }
		static FName GetVariantName() { return CustomEngineNodes::FiveDotOneVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace TimeStretchWOLAVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontRight)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontCenter)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLowFrequency)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioSideLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioSideRight))
			};
		}
	};
	using FFiveDotOneTimeStretchWOLANode = TTimeStretchWOLANode<FFiveDotOneWOLAChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FFiveDotOneTimeStretchWOLANode);

	struct FSevenDotOneWOLAChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_TimeStretchWOLASevenDotOneNodeDisplayName", "Time Stretch WOLA (7.1)"); }
		static FName GetVariantName() { return CustomEngineNodes::SevenDotOneVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace TimeStretchWOLAVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontRight)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioFrontCenter)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLowFrequency)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioSideLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioSideRight)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioBackLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioBackRight))
			};
		}
	};
	using FSevenDotOneTimeStretchWOLANode = TTimeStretchWOLANode<FSevenDotOneWOLAChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FSevenDotOneTimeStretchWOLANode);
} 

#undef LOCTEXT_NAMESPACE
