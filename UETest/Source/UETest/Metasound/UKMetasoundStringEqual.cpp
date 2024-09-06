// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKMetasoundStringEqual.h"
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

#define LOCTEXT_NAMESPACE "UKMetasoundStringEqualNode"
namespace Metasound
{
	FUKStringEqualNode::FUKStringEqualNode(const FOperatorSettings& InSettings, const FTriggerReadRef& InTrigger, const FStringReadRef& InStringA, const FStringReadRef& InStringB)
		: TriggerIn(InTrigger),
		StringAIn(InStringA),
		StringBIn(InStringB),
		TriggerTrueOut(FTriggerWriteRef::CreateNew(InSettings)),
		TriggerFalseOut(FTriggerWriteRef::CreateNew(InSettings))
	{
		
	}

	void FUKStringEqualNode::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKStringEqualOperatorNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InTrigger), TriggerIn);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InStringA), StringAIn);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InStringB), StringBIn);
	}

	void FUKStringEqualNode::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKStringEqualOperatorNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutTriggerTrue), TriggerTrueOut);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutTriggerFalse), TriggerFalseOut);
	}

	FDataReferenceCollection FUKStringEqualNode::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FUKStringEqualNode::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	void FUKStringEqualNode::Reset(const IOperator::FResetParams& InParams)
	{
	}

	void FUKStringEqualNode::Execute()
	{
		TriggerTrueOut->AdvanceBlock();
		TriggerFalseOut->AdvanceBlock();
		
		TriggerIn->ExecuteBlock(
			[&](int32 StartFrame, int32 EndFrame)
			{
			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				if (StringAIn->Compare(*StringBIn) == 0)
				{
					TriggerTrueOut->TriggerFrame(0);
					if(UUKAudioEngineSubsystem::Get()->OnAudioFinishedNative.IsBound())
					{
						// UUKAudioEngineSubsystem::Get()->OnAudioFinishedNative.Broadcast(nullptr);
					}
				}
				else
				{
					TriggerFalseOut->TriggerFrame(0);
				}
			}
		);
	}

	const FVertexInterface& FUKStringEqualNode::GetVertexInterface()
	{
		using namespace UKStringEqualOperatorNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InTrigger)),
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InStringA)),
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InStringB))
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutTriggerTrue)),
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutTriggerFalse))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FUKStringEqualNode::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { Metasound::StandardNodes::Namespace, TEXT("UKStringEqual"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("UKStringEqual_DisplayName", "UKStringEqual");
			Info.Description = METASOUND_LOCTEXT("UKStringEqual_Description", "");
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

	TUniquePtr<IOperator> FUKStringEqualNode::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace UKStringEqualOperatorNames;

		const FTriggerReadRef TriggerIn = InputData.GetOrCreateDefaultDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InTrigger), InParams.OperatorSettings);
		const FStringReadRef StringAIn = InputData.GetOrCreateDefaultDataReadReference<FString>(METASOUND_GET_PARAM_NAME(InStringA), InParams.OperatorSettings);
		const FStringReadRef StringBIn = InputData.GetOrCreateDefaultDataReadReference<FString>(METASOUND_GET_PARAM_NAME(InStringB), InParams.OperatorSettings);
		
		return MakeUnique<FUKStringEqualNode>(InParams.OperatorSettings, TriggerIn, StringAIn, StringBIn);
	}

	class FUKStringEqualMetasoundNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FUKStringEqualMetasoundNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FUKStringEqualNode>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FUKStringEqualMetasoundNode)
}
#undef LOCTEXT_NAMESPACE