// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKMetasoundContentDataNode.h"
#include "Internationalization/Text.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundStandardNodesNames.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/Dsp.h"
#include "MetasoundFacade.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundVertex.h"
#include "UKAudioEngineSubsystem.h"

#define LOCTEXT_NAMESPACE "UKMetasoundContentDataNode"
namespace Metasound
{
	FUKContentDataNode::FUKContentDataNode(const FOperatorSettings& InSettings)
		: TODTimeOutput(FFloatWriteRef::CreateNew(0.0f))
	{
		
	}

	void FUKContentDataNode::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKContentDataOperatorNames;
	}

	void FUKContentDataNode::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKContentDataOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTODTime), TODTimeOutput);
	}

	FDataReferenceCollection FUKContentDataNode::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FUKContentDataNode::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	void FUKContentDataNode::Reset(const IOperator::FResetParams& InParams)
	{
	}

	void FUKContentDataNode::Execute()
	{
		*TODTimeOutput = UUKAudioEngineSubsystem::Get()->TODTime;
	}

	const FVertexInterface& FUKContentDataNode::GetVertexInterface()
	{
		using namespace UKContentDataOperatorNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
			),
			FOutputVertexInterface(
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTODTime))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FUKContentDataNode::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { Metasound::StandardNodes::Namespace, TEXT("GetUKContentData"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("UKContentData_DisplayName", "GetUKContentData");
			Info.Description = METASOUND_LOCTEXT("UKContentData_Description", "");
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

	TUniquePtr<IOperator> FUKContentDataNode::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace UKContentDataOperatorNames;

		return MakeUnique<FUKContentDataNode>(InParams.OperatorSettings);
	}

	class FUKContentDataMetasoundNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FUKContentDataMetasoundNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FUKContentDataNode>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FUKContentDataMetasoundNode)
}
#undef LOCTEXT_NAMESPACE