// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ImpactSFXSynthLog.h"
#include "Runtime/Launch/Resources/Version.h"
#include "DSP/Dsp.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "ImpactSynthEngineNodesName.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundTrace.h"
#include "MultiDelayReverbMix.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_TapDelayFFNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace TapDelayFFVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		
		METASOUND_PARAM(InputAudio, "In Audio", "The input audio.")
		METASOUND_PARAM(InputIsInAudioStop, "Is In Audio Stop", "If true, start tracking internal buffers and trigger On Done when all of them have decayed to zero.")
		METASOUND_PARAM(InputFeedbackGain, "Feedback Gain", "The gain of the feedback loop for all internal channels.")
		METASOUND_PARAM(InputMinDelay, "Min Delay", "The delay range of the first internal channel.")
		METASOUND_PARAM(InputMaxDelay, "Max Delay", "The delay range of the last internal channel.")

		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers IsInAudioStop is true and the internal timer has reached zero.")
		METASOUND_PARAM(OutputAudio, "Out Mono", "Output audio.")
	}

	class FTapDelayFFOperator : public TExecutableOperator<FTapDelayFFOperator>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

		FTapDelayFFOperator(const FOperatorSettings& InSettings,
							const FTriggerReadRef& InTriggerPlay,
							const FAudioBufferReadRef& InAudio,
							const FBoolReadRef& InIsInAudioStop,
							const FFloatReadRef& InFeedbackGain,
							const FFloatReadRef& InMinDelay,
							const FFloatReadRef& InMaxDelay);

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;
		void Reset(const IOperator::FResetParams& InParams);
		void Execute();
		void ExecuteSubBlocks();
		void InitSynthesizers();
		void RenderFrameRange(int32 StartFrame, int32 EndFrame);
		
	private:
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FAudioBufferReadRef AudioInput;
		FBoolReadRef IsInAudioStop;
		FFloatReadRef FeedbackGain;
		FFloatReadRef MinDelay;
		FFloatReadRef MaxDelay;

		FTriggerWriteRef TriggerOnDone;
		FAudioBufferWriteRef AudioOutput;

		float SamplingRate;
		int32 NumFramesPerBlock;

		TArrayView<const float> InAudioView;
		TArrayView<float> OutputAudioView;
		TUniquePtr<FMultiDelayReverbMix> TapDelayFF;
		bool bIsPlaying;
		bool bFinish;
		float StopTimer;
		float TimeStep;
	};

	FTapDelayFFOperator::FTapDelayFFOperator(const FOperatorSettings& InSettings,
											const FTriggerReadRef& InTriggerPlay,
											const FAudioBufferReadRef& InAudio,
											const FBoolReadRef& InIsInAudioStop,
											const FFloatReadRef& InFeedbackGain,
											const FFloatReadRef& InMinDelay,
											const FFloatReadRef& InMaxDelay)
		: OperatorSettings(InSettings)
		, PlayTrigger(InTriggerPlay)
		, AudioInput(InAudio)
		, IsInAudioStop(InIsInAudioStop)
		, FeedbackGain(InFeedbackGain)
		, MinDelay(InMinDelay)
		, MaxDelay(InMaxDelay)
		, TriggerOnDone(FTriggerWriteRef::CreateNew(InSettings))
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
	{
		SamplingRate = InSettings.GetSampleRate();
		NumFramesPerBlock = InSettings.GetNumFramesPerBlock();
		
		InAudioView = TArrayView<const float>(AudioInput->GetData(), AudioInput->Num());
		OutputAudioView = TArrayView<float>(AudioOutput->GetData(), AudioOutput->Num());
		bIsPlaying = false;
		bFinish = false;
		StopTimer = 0.f;
		TimeStep = 1.0f / SamplingRate;
	}

	void FTapDelayFFOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace TapDelayFFVertexNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAudio), AudioInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsInAudioStop), IsInAudioStop);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFeedbackGain), FeedbackGain);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputMinDelay), MinDelay);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputMaxDelay), MaxDelay);
	}

	void FTapDelayFFOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace TapDelayFFVertexNames;
		
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudio), AudioOutput);
	}

	FDataReferenceCollection FTapDelayFFOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FTapDelayFFOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}
	
	void FTapDelayFFOperator::Reset(const IOperator::FResetParams& InParams)
	{
		TriggerOnDone->Reset();
		AudioOutput->Zero();
		TapDelayFF.Reset();
		
		bIsPlaying = false;
		bFinish = false;
	}

	void FTapDelayFFOperator::Execute()
	{
		METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::TapDelayFFVertexNames::Execute);
		
		TriggerOnDone->AdvanceBlock();

		// zero output buffers
		FMemory::Memzero(OutputAudioView.GetData(), NumFramesPerBlock * sizeof(float));
			
		ExecuteSubBlocks();
	}

	void FTapDelayFFOperator::ExecuteSubBlocks()
	{
		// Parse triggers and render audio
		int32 PlayTrigIndex = 0;
		int32 NextPlayFrame = 0;
		const int32 NumPlayTrigs = PlayTrigger->NumTriggeredInBlock();

		int32 NextAudioFrame = 0;
		int32 CurrAudioFrame = 0;
		const int32 LastAudioFrame = OperatorSettings.GetNumFramesPerBlock() - 1;
		const int32 NoTrigger = OperatorSettings.GetNumFramesPerBlock() << 1; //Same as multiply by 2

		while (NextPlayFrame < LastAudioFrame)
		{
			// get the next Play and Stop indices
			NextPlayFrame = PlayTrigIndex < NumPlayTrigs ? (*PlayTrigger)[PlayTrigIndex] : NoTrigger;   
			NextAudioFrame = FMath::Min(NextPlayFrame, OperatorSettings.GetNumFramesPerBlock());
				
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
		}
	}

	void FTapDelayFFOperator::InitSynthesizers()
	{
		TapDelayFF.Reset();
		
		TapDelayFF = MakeUnique<FMultiDelayReverbMix>(SamplingRate, *FeedbackGain, *MinDelay, *MaxDelay);
		bIsPlaying = TapDelayFF.IsValid();
	}

	void FTapDelayFFOperator::RenderFrameRange(int32 StartFrame, int32 EndFrame)
	{
		if(!bIsPlaying)
			return;
		
		const int32 NumFramesToGenerate = EndFrame - StartFrame;
		if (NumFramesToGenerate <= 0)
		{
			UE_LOG(LogImpactSFXSynth, Warning, TEXT("TapDelayFFNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
			return;
		}
		
		TArrayView<float>  BufferToGenerate = OutputAudioView.Slice(StartFrame, NumFramesToGenerate);
		const float Gain = *FeedbackGain;
		TapDelayFF->ProcessAudio(BufferToGenerate, InAudioView, Gain);
		
		if(*IsInAudioStop)
		{
			StopTimer -= TimeStep * NumFramesToGenerate;
			if(StopTimer <= 0.f)
			{
				TapDelayFF.Reset();
				bIsPlaying = false;
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}
		else
			StopTimer = FMath::LogX(Gain, 0.01f) * (*MaxDelay);
	}

	const FVertexInterface& FTapDelayFFOperator::GetVertexInterface()
	{
		using namespace TapDelayFFVertexNames;
		
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAudio)),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsInAudioStop), false),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFeedbackGain), 0.5f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputMinDelay), 0.1f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputMaxDelay), 0.2f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudio))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FTapDelayFFOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Tap Defay FF"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("Metasound_TapDelayFFDisplayName", "Tap Delay FF");
			Info.Description = METASOUND_LOCTEXT("Metasound_TapDelayFFNodeDescription", "Tap delay network with feedback. The output is mixed using Householder matrix.");
			Info.Author = PluginAuthor;
			Info.PromptIfMissing = PluginNodeMissingPrompt;
			Info.DefaultInterface = GetVertexInterface();
			Info.CategoryHierarchy.Emplace(NodeCategories::Spatialization);
			Info.Keywords = { };
			return Info;
		};

		static const FNodeClassMetadata Info = InitNodeInfo();

		return Info;
	}

	TUniquePtr<IOperator> FTapDelayFFOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace TapDelayFFVertexNames;
		
		FTriggerReadRef InTriggerPlay = InputData.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings);
		FAudioBufferReadRef InAudio = InputData.GetOrConstructDataReadReference<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputAudio), InParams.OperatorSettings);
		FBoolReadRef InIsInAudioStop = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsInAudioStop), InParams.OperatorSettings);
		FFloatReadRef InGain = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputFeedbackGain), InParams.OperatorSettings);
		FFloatReadRef InDelayMin = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputMinDelay), InParams.OperatorSettings);
		FFloatReadRef InDelayMax = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputMaxDelay), InParams.OperatorSettings);

		return MakeUnique<FTapDelayFFOperator>(InParams.OperatorSettings, InTriggerPlay, InAudio, InIsInAudioStop, InGain, InDelayMin, InDelayMax);
	}

	class FTapDelayFFNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FTapDelayFFNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FTapDelayFFOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FTapDelayFFNode)
}

#undef LOCTEXT_NAMESPACE
