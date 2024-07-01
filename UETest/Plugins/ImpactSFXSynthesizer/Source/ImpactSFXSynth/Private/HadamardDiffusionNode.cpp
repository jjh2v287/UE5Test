// Copyright 2023-2024, Le Binh Son, All Rights Reserved.
//
// #include "HadamardDiffusion.h"
// #include "Runtime/Launch/Resources/Version.h"
// #include "DSP/Dsp.h"
// #include "MetasoundAudioBuffer.h"
// #include "MetasoundExecutableOperator.h"
// #include "MetasoundNodeRegistrationMacro.h"
// #include "MetasoundDataTypeRegistrationMacro.h"
// #include "MetasoundParamHelper.h"
// #include "MetasoundPrimitives.h"
// #include "MetasoundTrigger.h"
// #include "MetasoundVertex.h"
// #include "ImpactSynthEngineNodesName.h"
// #include "MetasoundStandardNodesCategories.h"
//
// #define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_HadamardDiffusionNode"
//
// namespace LBSImpactSFXSynth
// {
// 	using namespace Metasound;
// 	
// 	namespace HadamardDiffusionVertexNames
// 	{
// 		METASOUND_PARAM(InputAudio, "In Audio", "Input audio.")
// 		METASOUND_PARAM(InputNumStages, "Num Stages", "Number of diffusion stages.")
// 		METASOUND_PARAM(InputMaxDelay, "Max Delay", "The max delay times of each stage in seconds.")
// 		METASOUND_PARAM(InputIsDoubleDelayEachStage, "Is Double Delay Each Stage", "If true, the delay of each subsequent stage is doubled compared to its previous stage.")
// 		
// 		METASOUND_PARAM(OutputChannel1, "Out 1", "1st channel audio output.")
// 		METASOUND_PARAM(OutputChannel2, "Out 2", "2nd channel audio output.")
// 		METASOUND_PARAM(OutputChannel3, "Out 3", "3rd channel audio output.")
// 		METASOUND_PARAM(OutputChannel4, "Out 4", "4th channel audio output.")
// 	}
//
// 	class FHadamardDiffusionOperator : public TExecutableOperator<FHadamardDiffusionOperator>
// 	{
// 	public:
//
// 		static const FNodeClassMetadata& GetNodeInfo();
// 		static const FVertexInterface& GetVertexInterface();
// 		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
//
// 		FHadamardDiffusionOperator(const FOperatorSettings& InSettings,
// 							const FAudioBufferReadRef& InputAudio,
// 							const FInt32ReadRef& InNumStages,
// 							const FFloatReadRef& InMaxDelay,
// 							const FBoolReadRef& IsDoubleDelayEachStage);
//
// 		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
// 		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
// 		virtual FDataReferenceCollection GetInputs() const override;
// 		virtual FDataReferenceCollection GetOutputs() const override;
// 		void Reset(const IOperator::FResetParams& InParams);
// 		void Execute();
//
// 	private:
// 		FAudioBufferReadRef AudioInput;
// 		FInt32ReadRef NumStages;
// 		FFloatReadRef MaxDelay;
// 		FBoolReadRef bDoubleDelayEachStage;
//
// 		TArrayView<const float> InAudioView;
//
// 		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
// 		TArray<FName> OutputAudioBufferVertexNames;
// 		
// 		Audio::FMultichannelBufferView OutputAudioView;
// 		TUniquePtr<FHadamardDiffusion> HadamardDiffusion;
//
// 		int32 NumFramesPerBlock;
// 	};
//
// 	FHadamardDiffusionOperator::FHadamardDiffusionOperator(const FOperatorSettings& InSettings,
// 		const FAudioBufferReadRef& InputAudio,
// 		const FInt32ReadRef& InNumStages,
// 		const FFloatReadRef& InMaxDelay,
// 		const FBoolReadRef& IsDoubleDelayEachStage)
// 		: AudioInput(InputAudio)
// 		, NumStages(InNumStages)
// 		, MaxDelay(InMaxDelay)
// 		, bDoubleDelayEachStage(IsDoubleDelayEachStage)
// 	{
// 		float SamplingRate = InSettings.GetSampleRate();
// 		NumFramesPerBlock = InSettings.GetNumFramesPerBlock();
// 		HadamardDiffusion = MakeUnique<FHadamardDiffusion>(SamplingRate, *NumStages, *MaxDelay, *bDoubleDelayEachStage);
// 		
// 		// Hold on to a view of the output audio. Audio buffers are only writable
// 		// by this object and will not be reallocated.
// 		OutputAudioView.Empty(FHadamardDiffusion::NumChannels);
// 		for(int i = 0; i < FHadamardDiffusion::NumChannels; i++)
// 		{
// 			FAudioBufferWriteRef AudioBuffer = FAudioBufferWriteRef::CreateNew(InSettings);
// 			OutputAudioBuffers.Add(AudioBuffer);
//
// 			// Hold on to a view of the output audio. Audio buffers are only writable
// 			// by this object and will not be reallocated. 
// 			OutputAudioView.Emplace(AudioBuffer->GetData(), AudioBuffer->Num());
// 		}
//
// 		InAudioView = TArrayView<const float>(AudioInput->GetData(), AudioInput->Num());
// 	}
//
// 	void FHadamardDiffusionOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
// 	{
// 		using namespace HadamardDiffusionVertexNames;
// 		
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAudio), AudioInput);
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumStages), NumStages);
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputMaxDelay), MaxDelay);
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsDoubleDelayEachStage), bDoubleDelayEachStage);
// 	}
//
// 	void FHadamardDiffusionOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
// 	{
// 		using namespace HadamardDiffusionVertexNames;
// 		
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputChannel1), OutputAudioBuffers[0]);
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputChannel2), OutputAudioBuffers[1]);
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputChannel3), OutputAudioBuffers[2]);
// 		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputChannel4), OutputAudioBuffers[3]);
// 	}
//
// 	FDataReferenceCollection FHadamardDiffusionOperator::GetInputs() const
// 	{
// 		// This should never be called. Bind(...) is called instead. This method
// 		// exists as a stop-gap until the API can be deprecated and removed.
// 		checkNoEntry();
// 		return {};
// 	}
//
// 	FDataReferenceCollection FHadamardDiffusionOperator::GetOutputs() const
// 	{
// 		// This should never be called. Bind(...) is called instead. This method
// 		// exists as a stop-gap until the API can be deprecated and removed.
// 		checkNoEntry();
// 		return {};
// 	}
// 	
// 	void FHadamardDiffusionOperator::Reset(const IOperator::FResetParams& InParams)
// 	{
// 		for (int32 i = 0; i < OutputAudioBuffers.Num(); i++)
// 		{
// 			 OutputAudioBuffers[i]->Zero();
// 		}
//
// 		HadamardDiffusion->ResetBuffers();
// 	}
//
// 	void FHadamardDiffusionOperator::Execute()
// 	{
// 		if(HadamardDiffusion.IsValid())
// 		{
// 			for (const TArrayView<float>& OutputBuffer : OutputAudioView)
// 				FMemory::Memzero(OutputBuffer.GetData(), NumFramesPerBlock * sizeof(float));
// 			
// 			HadamardDiffusion->ProcessAudio(OutputAudioView, InAudioView);
// 		}
// 	}
//
// 	const FVertexInterface& FHadamardDiffusionOperator::GetVertexInterface()
// 	{
// 		using namespace HadamardDiffusionVertexNames;
// 		
// 		static const FVertexInterface Interface(
// 			FInputVertexInterface(
// 				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAudio)),
// 				TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumStages), 3),
// 				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputMaxDelay), 0.1f),
// 				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsDoubleDelayEachStage), true)
// 			),
// 			FOutputVertexInterface(
// 				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputChannel1)),
// 				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputChannel2)),
// 				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputChannel3)),
// 				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputChannel4))
// 			)
// 		);
//
// 		return Interface;
// 	}
//
// 	
//
//
// 	const FNodeClassMetadata& FHadamardDiffusionOperator::GetNodeInfo()
// 	{
// 		auto InitNodeInfo = []() -> FNodeClassMetadata
// 		{
// 			FNodeClassMetadata Info;
// 			Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Hadamard Diffuser"), TEXT("") };
// 			Info.MajorVersion = 1;
// 			Info.MinorVersion = 0;
// 			Info.DisplayName = METASOUND_LOCTEXT("Metasound_HadamardDiffusionDisplayName", "Hadamard Diffuser");
// 			Info.Description = METASOUND_LOCTEXT("Metasound_HadamardDiffusionNodeDescription", "Apply Hadamard diffusion to an input audio signal.");
// 			Info.Author = PluginAuthor;
// 			Info.PromptIfMissing = PluginNodeMissingPrompt;
// 			Info.DefaultInterface = GetVertexInterface();
// 			Info.CategoryHierarchy.Emplace(NodeCategories::Spatialization);
// 			Info.Keywords = { };
// 			return Info;
// 		};
//
// 		static const FNodeClassMetadata Info = InitNodeInfo();
//
// 		return Info;
// 	}
//
// 	TUniquePtr<IOperator> FHadamardDiffusionOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
// 	{
// 		const FInputVertexInterfaceData& InputData = InParams.InputData;
//
// 		using namespace HadamardDiffusionVertexNames;		
// 		FAudioBufferReadRef InAudio = InputData.GetOrConstructDataReadReference<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputAudio), InParams.OperatorSettings);
// 		FInt32ReadRef InNumStages = InputData.GetOrCreateDefaultDataReadReference<int>(METASOUND_GET_PARAM_NAME(InputNumStages), InParams.OperatorSettings);
// 		FFloatReadRef InMaxDelay = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputMaxDelay), InParams.OperatorSettings);
// 		FBoolReadRef IsDouble = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsDoubleDelayEachStage), InParams.OperatorSettings);
//
// 		return MakeUnique<FHadamardDiffusionOperator>(InParams.OperatorSettings, InAudio, InNumStages, InMaxDelay, IsDouble);
// 	}
//
// 	class FHadamardDiffusionNode : public FNodeFacade
// 	{
// 	public:
// 		/**
// 		 * Constructor used by the Metasound Frontend.
// 		 */
// 		FHadamardDiffusionNode(const FNodeInitData& InitData)
// 			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FHadamardDiffusionOperator>())
// 		{
// 		}
// 	};
//
// 	METASOUND_REGISTER_NODE(FHadamardDiffusionNode)
// }
//
// #undef LOCTEXT_NAMESPACE
