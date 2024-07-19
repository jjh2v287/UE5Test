// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKWavePlayNode.h"
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
#include "DSP/FloatArrayMath.h"

#define LOCTEXT_NAMESPACE "UKWavePlayNode"
namespace Metasound
{
	FUKWavePlayOperator::FUKWavePlayOperator(const FOperatorSettings& InSettings, const FWaveAssetReadRef& InWaveAsset, const FBoolReadRef& InLoop)
		: WaveAsset(InWaveAsset)
		, OperatorSettings(InSettings)
		, LoopInput(InLoop)
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
	{
	}

	void FUKWavePlayOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKWavePlayOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputWaveAsset), WaveAsset);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputLoop), LoopInput);
	}

	void FUKWavePlayOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKWavePlayOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioMono), AudioOutput);
	}

	FDataReferenceCollection FUKWavePlayOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FUKWavePlayOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	void FUKWavePlayOperator::Reset(const IOperator::FResetParams& InParams)
	{
		AudioOutput->Zero();
	}

	void FUKWavePlayOperator::Execute()
	{
		OperatorSettings.GetNumFramesPerBlock();
	}

	const FVertexInterface& FUKWavePlayOperator::GetVertexInterface()
	{
		using namespace UKWavePlayOperatorNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FWaveAsset>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputWaveAsset)),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputLoop), true)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FUKWavePlayOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { Metasound::StandardNodes::Namespace, TEXT("UKWavePlay"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("UKWavePlay_DisplayName", "UKWavePlay");
			Info.Description = METASOUND_LOCTEXT("UKWavePlay_Description", "");
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

	TUniquePtr<IOperator> FUKWavePlayOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace UKWavePlayOperatorNames;

		const FWaveAssetReadRef InputWaveAssetRef = InputData.GetOrCreateDefaultDataReadReference<Metasound::FWaveAsset>(METASOUND_GET_PARAM_NAME(InputWaveAsset), InParams.OperatorSettings);
		const FBoolReadRef HeadLoopRef = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputLoop), InParams.OperatorSettings);

		return MakeUnique<FUKWavePlayOperator>(InParams.OperatorSettings, InputWaveAssetRef, HeadLoopRef);
	}

	class FUKWavePlayNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FUKWavePlayNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FUKWavePlayOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FUKWavePlayNode)
}
#undef LOCTEXT_NAMESPACE