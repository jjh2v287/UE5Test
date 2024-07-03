// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKMetasoundRotateAngleDifferenceNode.h"
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

#define LOCTEXT_NAMESPACE "UKMetasoundRotateAngleDifferenceNode"
namespace Metasound
{
	FUKRotateAngleDifferenceOperator::FUKRotateAngleDifferenceOperator(const FOperatorSettings& InSettings, const FFloatReadRef& InAngle, const FFloatReadRef& InDeltaAngle)
		: Angle(InAngle)
		, DeltaAngle(InDeltaAngle)
		, AngleOutput(FFloatWriteRef::CreateNew(0.0f))
	{
	}

	void FUKRotateAngleDifferenceOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKDeltaDegreeVertexNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAngle), Angle);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDeltaAngle), DeltaAngle);
	}

	void FUKRotateAngleDifferenceOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKDeltaDegreeVertexNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAngle), AngleOutput);
	}

	FDataReferenceCollection FUKRotateAngleDifferenceOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FUKRotateAngleDifferenceOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	void FUKRotateAngleDifferenceOperator::Reset(const IOperator::FResetParams& InParams)
	{
	}

	void FUKRotateAngleDifferenceOperator::Execute()
	{
		const float TempPanningAngle = GetRotateTheAngleByDifference(*Angle, *DeltaAngle);
		*AngleOutput = TempPanningAngle;
	}
	
	const float FUKRotateAngleDifferenceOperator::GetRotateTheAngleByDifference(const float PramAngle, const float PramDelta)
	{
		float ReturnAngle = PramAngle - PramDelta;
		if (ReturnAngle < 0.0f)
		{
			ReturnAngle = (PramDelta - FMath::Abs(ReturnAngle)) + (360.0f - PramDelta);
		}
		return ReturnAngle;
	}

	const FVertexInterface& FUKRotateAngleDifferenceOperator::GetVertexInterface()
	{
		using namespace UKDeltaDegreeVertexNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAngle), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDeltaAngle), 90.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAngle))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FUKRotateAngleDifferenceOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { Metasound::StandardNodes::Namespace, TEXT("UK Rotate Angle Difference"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("UKRotateAngleDifference_DisplayName", "UK Rotate Angle Difference");
			Info.Description = METASOUND_LOCTEXT("UKRotateAngleDifference_Description", "");
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

	TUniquePtr<IOperator> FUKRotateAngleDifferenceOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace UKDeltaDegreeVertexNames;

		FFloatReadRef Angle = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAngle), InParams.OperatorSettings);
		FFloatReadRef DeltaAngle = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDeltaAngle), InParams.OperatorSettings);

		return MakeUnique<FUKRotateAngleDifferenceOperator>(InParams.OperatorSettings, Angle, DeltaAngle);
	}

	class FUKRotateAngleDifferenceNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FUKRotateAngleDifferenceNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FUKRotateAngleDifferenceOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FUKRotateAngleDifferenceNode)
}
#undef LOCTEXT_NAMESPACE