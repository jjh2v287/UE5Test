// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKMetasoundTestNode.h"
#include "Internationalization/Text.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesNames.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/Dsp.h"
#include "MetasoundFacade.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundVertex.h"

#define LOCTEXT_NAMESPACE "UKMetasoundTestNode"
namespace Metasound
{
	FUKTestOperator::FUKTestOperator(const FOperatorSettings& InSettings, const FAudioBufferReadRef& InAudio, const FFloatReadRef& InTest)
		: AudioInput(InAudio)
		, TestInput(InTest)
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
	{
	}

	void FUKTestOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKTestOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAudio), AudioInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTest), TestInput);
	}

	void FUKTestOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKTestOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudio), AudioOutput);
	}

	FDataReferenceCollection FUKTestOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FUKTestOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	void FUKTestOperator::Reset(const IOperator::FResetParams& InParams)
	{
		AudioOutput->Zero();
	}

	void FUKTestOperator::Execute()
	{
		ProcessAudioBuffer(AudioInput->GetData(), AudioOutput->GetData(), AudioInput->Num());
	}

	void FUKTestOperator::ProcessAudioBuffer(const float * InputBuffer, float * OutputBuffer, const int32 NumSamples)
	{
		for (int SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
		{
			const float InputSample = InputBuffer[SampleIndex];
			const float LPF = *TestInput;
			
			OutputBuffer[SampleIndex] = InputSample;
		}
	}
	
	const FVertexInterface& FUKTestOperator::GetVertexInterface()
	{
		using namespace UKTestOperatorNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAudio)),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTest), 90.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudio))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FUKTestOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { Metasound::StandardNodes::Namespace, TEXT("UK Test"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("UKTest_DisplayName", "UK Test");
			Info.Description = METASOUND_LOCTEXT("UKTest_Description", "");
			Info.Author = PluginAuthor;
			Info.PromptIfMissing = PluginNodeMissingPrompt;
			Info.DefaultInterface = GetVertexInterface();
			Info.CategoryHierarchy.Emplace(Metasound::NodeCategories::Math);
			Info.Keywords = { METASOUND_LOCTEXT("DeltaDegreeKeyword", "Binaural"), METASOUND_LOCTEXT("DeltaDegreeKeyword", "")};
			return Info;
		};

		static const FNodeClassMetadata Info = InitNodeInfo();

		return Info;
	}

	TUniquePtr<IOperator> FUKTestOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace UKTestOperatorNames;

		const FAudioBufferReadRef Angle = InputData.GetOrCreateDefaultDataReadReference<Metasound::FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputAudio), InParams.OperatorSettings);
		const FFloatReadRef DeltaAngle = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputTest), InParams.OperatorSettings);

		return MakeUnique<FUKTestOperator>(InParams.OperatorSettings, Angle, DeltaAngle);
	}

	class FUKTestNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FUKTestNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FUKTestOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FUKTestNode)
}
#undef LOCTEXT_NAMESPACE