// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "MetasoundResidualData.h"
#include "ImpactSFXSynthLog.h"
#include "ImpactSynthEngineNodesName.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTime.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "ResidualSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ResidualSynthNodes"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ResidualSynthVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "Stop synthesizing.")
		
		METASOUND_PARAM(InputResidualData, "Residual Data", "Residual preset data to use.")

		METASOUND_PARAM(InputResidualAmplitudeScale, "Amplitude Scale", "Residual amplitude scale.")
		METASOUND_PARAM(InputResidualIsLoop, "Loop", "Looping if enable.")
		METASOUND_PARAM(InputResidualRandomLoop, "Loop Start Random", "Min = 0. Loop starting time = Start time + Rand(0, Loop Start Random).")
		METASOUND_PARAM(InputResidualRandomMag, "Magnitude Random Range", "[0, 1]. Randomize the magnitude of each frequency.")
		METASOUND_PARAM(InputResidualIsUsePreviewValue, "Use Preview Values", "Use the preview values in Residual Data Editor for all properties below this pin.")
		METASOUND_PARAM(InputResidualPitchShift, "Pitch Shift", "Shift pitches by semitone.")
		METASOUND_PARAM(InputResidualPlaySpeed, "Play Speed", "Residual play back rate.")
		METASOUND_PARAM(InputResidualStartTime, "Start time", "Start synthesize at time in seconds.")
		METASOUND_PARAM(InputResidualDuration, "Duration", "Duration in seconds. <= 0 means this will stop when it has synthesized the final frame.")
		METASOUND_PARAM(InputResidualSeed, "Seed", "If < 0, use a random seed.")		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when finished.")

		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
	}

	struct FResidualSynthOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		FResidualDataReadRef ResidualData;
		FFloatReadRef ResidualAmplitudeScale;
		FBoolReadRef bIsLoop;
		FTimeReadRef RandomLoop;
		FFloatReadRef RandomMagnitude;
		FBoolReadRef bUsePreviewValues;
		FFloatReadRef ResidualPitchShift;
		FFloatReadRef ResidualPlaySpeed;
		FTimeReadRef ResidualStartTime;
		FTimeReadRef ResidualDuration;
		FInt32ReadRef ResidualSeed;
	};
	
	class FResidualSynthOperator : public TExecutableOperator<FResidualSynthOperator>
	{
	public:

		FResidualSynthOperator(const FResidualSynthOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, ResidualData(InArgs.ResidualData)
			, ResidualAmplitudeScale(InArgs.ResidualAmplitudeScale)
			, bIsLoop(InArgs.bIsLoop)
			, RandomLoop(InArgs.RandomLoop)
			, RandomMagnitude(InArgs.RandomMagnitude)
			, bUsePreviewValues(InArgs.bUsePreviewValues)
			, ResidualPitchShift(InArgs.ResidualPitchShift)
			, ResidualPlaySpeed(InArgs.ResidualPlaySpeed)
			, ResidualStartTime(InArgs.ResidualStartTime)
			, ResidualDuration(InArgs.ResidualDuration)
			, ResidualSeed(InArgs.ResidualSeed)
			, TriggerOnDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
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
#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ResidualSynthVertexNames;
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualData), ResidualData);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualAmplitudeScale), ResidualAmplitudeScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualIsLoop), bIsLoop);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualRandomLoop), RandomLoop);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualRandomMag), RandomMagnitude);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualIsUsePreviewValue), bUsePreviewValues);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPlaySpeed), ResidualPlaySpeed);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualStartTime), ResidualStartTime);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualDuration), ResidualDuration);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSeed), ResidualSeed);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ResidualSynthVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);

			check(OutputAudioBuffers.Num() == OutputAudioBufferVertexNames.Num());

			for (int32 i = 0; i < OutputAudioBuffers.Num(); i++)
			{
				InOutVertexData.BindReadVertex(OutputAudioBufferVertexNames[i], OutputAudioBuffers[i]);
			}
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ResidualSynthVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualData), ResidualData);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualAmplitudeScale), ResidualAmplitudeScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualIsLoop), bIsLoop);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualRandomLoop), RandomLoop);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualRandomMag), RandomMagnitude);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualIsUsePreviewValue), bUsePreviewValues);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPlaySpeed), ResidualPlaySpeed);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualStartTime), ResidualStartTime);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualDuration), ResidualDuration);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSeed), ResidualSeed);

			
			FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);

			check(OutputAudioBuffers.Num() == OutputAudioBufferVertexNames.Num());

			for (int32 i = 0; i < OutputAudioBuffers.Num(); i++)
			{
				Outputs.BindReadVertex(OutputAudioBufferVertexNames[i], OutputAudioBuffers[i]);
			}
		}
#endif
		
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
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::FResidualSynthOperator::Execute);
			
			TriggerOnDone->AdvanceBlock();

			// zero output buffers
			for (const FAudioBufferWriteRef& OutputBuffer : OutputAudioBuffers)
				FMemory::Memzero(OutputBuffer->GetData(), OperatorSettings.GetNumFramesPerBlock() * sizeof(float));
			
			ExecuteSubBlocks();
		}

#if ENGINE_MINOR_VERSION > 2
		void Reset(const IOperator::FResetParams& InParams)
		{
			TriggerOnDone->Reset();
			
			for (const FAudioBufferWriteRef& BufferRef : OutputAudioBuffers)
			{
				BufferRef->Zero();
			}

			ResidualSynth.Reset();
			
			bIsPlaying = false;
		}
#endif
		
	private:
		static constexpr float MaxClamp = 1.e3f;
		static constexpr float MinClamp = 1.e-3f;
		
		void ExecuteSubBlocks()
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
				
				if (CurrAudioFrame != NextAudioFrame)
				{
					RenderFrameRange(CurrAudioFrame, NextAudioFrame);
					CurrAudioFrame = NextAudioFrame;
				}
				
				if (CurrAudioFrame == NextPlayFrame)
				{
					InitSynthesizers();
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

		void InitSynthesizers()
		{
			ResidualSynth.Reset();

			const FResidualDataAssetProxyPtr& ResidualDataProxy = (*ResidualData).GetProxy();
			if(ResidualDataProxy.IsValid())
			{
				float PlaySpeed;
				int32 Seed;
				float StartTime;
				float Duration;
				float PitchScale;
				if(*bUsePreviewValues)
				{
					PlaySpeed = ResidualDataProxy->GetPreviewPlaySpeed();
					Seed = ResidualDataProxy->GetPreviewSeed();
					StartTime = ResidualDataProxy->GetPreviewStartTime();
					Duration = ResidualDataProxy->GetPreviewDuration();
					PitchScale = GetPitchScaleClamped(ResidualDataProxy->GetPreviewPitchShift());
				}
				else
				{
					PlaySpeed = *ResidualPlaySpeed;
					StartTime = (*ResidualStartTime).GetSeconds();
					Duration =  ResidualDuration->GetSeconds();
					Seed = *ResidualSeed;
					PitchScale = GetPitchScaleClamped(*ResidualPitchShift);
				}
				
				ResidualSynth = MakeUnique<FResidualSynth>(SamplingRate, OperatorSettings.GetNumFramesPerBlock(), OutputAudioView.Num(),
														   ResidualDataProxy.ToSharedRef(), PlaySpeed, *ResidualAmplitudeScale, PitchScale,
														   Seed, StartTime, Duration, *bIsLoop, 1.0f,
														   RandomLoop->GetSeconds(), 0.f, *RandomMagnitude);
			}

			bIsPlaying = ResidualSynth.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;
			
			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate <= 0)
			{
				UE_LOG(LogImpactSFXSynth, Warning, TEXT("ResidualSynthNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
				return;
			}
			
			FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);
			
			bool bIsResidualStop = true;
			if(ResidualSynth.IsValid())
			{
				bIsResidualStop = ResidualSynth->Synthesize(BufferToGenerate, false, true);
			}
			
			if(bIsResidualStop)
			{
				bIsPlaying = false;
				ResidualSynth.Reset();
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		FResidualDataReadRef ResidualData;
		FFloatReadRef ResidualAmplitudeScale;
		FBoolReadRef bIsLoop;
		FTimeReadRef RandomLoop;
		FFloatReadRef RandomMagnitude;
		FBoolReadRef bUsePreviewValues;
		FFloatReadRef ResidualPitchShift;
		FFloatReadRef ResidualPlaySpeed;
		FTimeReadRef ResidualStartTime;
		FTimeReadRef ResidualDuration;
		FInt32ReadRef ResidualSeed;
		
		FTriggerWriteRef TriggerOnDone;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;
		
		TUniquePtr<FResidualSynth> ResidualSynth;
		Audio::FMultichannelBufferView OutputAudioView;
		
		float SamplingRate;
		int32 NumOutputChannels;
		bool bIsPlaying = false;
		float LowAbsMaxTrackingTime = 0;
	};

	class FResidualSynthOperatorFactory : public IOperatorFactory
	{
	public:
		FResidualSynthOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace ResidualSynthVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FResidualSynthOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FResidualData>(METASOUND_GET_PARAM_NAME(InputResidualData)),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualAmplitudeScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputResidualIsLoop), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputResidualRandomLoop), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualRandomMag), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputResidualIsUsePreviewValue), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualPlaySpeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputResidualStartTime), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputResidualDuration), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputResidualSeed), InParams.OperatorSettings)
			};

			return MakeUnique<FResidualSynthOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};

	
	template<typename AudioChannelConfigurationInfoType>
	class TResidualSynthNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace ResidualSynthVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
					TInputDataVertex<FResidualData>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualData)),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualAmplitudeScale), 1.f),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualIsLoop), false),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualRandomLoop), 0.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualRandomMag), 0.f),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualIsUsePreviewValue), false),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualPitchShift), 0.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualPlaySpeed), 1.f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualStartTime), 0.f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualDuration), -1.f),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualSeed), -1)
					),
				FOutputVertexInterface(
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnPlay)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone))
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
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Residual Synth"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_ResidualSynthNodeDescription", "Synthesize Residual SFX.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("ResidualSyntKeyword", "Synthesis") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TResidualSynthNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FResidualSynthOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TResidualSynthNode(const FNodeInitData& InInitData)
			: TResidualSynthNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TResidualSynthNode() = default;

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

	struct FResidualSynthMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ResidualSynthMonoNodeDisplayName", "Residual SFX Synth (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ResidualSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoResidualSynthNode = TResidualSynthNode<FResidualSynthMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoResidualSynthNode);
	
	struct FResidualSynthStereoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ResidualSynthStereoNodeDisplayName", "Residual SFX Synth (Stereo)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::StereoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ResidualSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			};
		}
	};
	using FStereoResidualSynthNode = TResidualSynthNode<FResidualSynthStereoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FStereoResidualSynthNode);

}

#undef LOCTEXT_NAMESPACE