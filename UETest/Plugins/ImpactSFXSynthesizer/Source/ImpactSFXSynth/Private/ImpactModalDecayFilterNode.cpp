// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactSynthEngineNodesName.h"
#include "Runtime/Launch/Resources/Version.h"
#include "DSP/Dsp.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundImpactModalObj.h"
#include "MetasoundParamHelper.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundVertex.h"
#include "ModalSynth.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactModalDecayFilterNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactModalDecayFilterVertexNames
	{
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params.")
		METASOUND_PARAM(InputDecayMin, "Decay Min", "The minimum decay (inclusive). Larger than or equal to 0.")
		METASOUND_PARAM(InputDecayMax, "Decay Max", "The maximum decay (inclusive). Must be larger than Decay Min.")
		
		METASOUND_PARAM(OutputModal, "Output Modal", "The output modal.");
	}

	struct FImpactModalDecayFilterOpArgs
	{
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;
		float DecayMin;
		float DecayMax;
	};
	
	class FImpactModalDecayFilterOperator : public TExecutableOperator<FImpactModalDecayFilterOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FImpactModalDecayFilterOperator(const FImpactModalDecayFilterOpArgs& InArgs)
			: OperatorSettings(InArgs.OperatorSettings)
		    , ModalParams(InArgs.ModalParams)
		    , NumModals(InArgs.NumModals)
			, DecayMin(InArgs.DecayMin)
			, DecayMax(InArgs.DecayMax)
			, OutputModal(FImpactModalObjWriteRef::CreateNew())
		{
			CreateModalObj();
		}

		void CreateModalObj()
		{
			const FImpactModalObjAssetProxyPtr& ModalPtr = ModalParams->GetProxy();
			if(!ModalPtr.IsValid())
				return;

			const int32 NumUsedModals = NumModals > 0 ? FMath::Min(NumModals, ModalPtr->GetNumModals()) : ModalPtr->GetNumModals();
			
			const TArrayView<const float> OrgModalParams = ModalPtr->GetParams();
			const int32 NumParams = NumUsedModals * FModalSynth::NumParamsPerModal;
			
			TArray<float> ValidParams;
			ValidParams.Empty(NumParams);
			for(int DecayIdx = FModalSynth::DecayBin; DecayIdx < NumParams; DecayIdx += FModalSynth::NumParamsPerModal)
			{
				const float Decay = OrgModalParams[DecayIdx];
				if(Decay >= DecayMin && Decay <= DecayMax)
				{
					const int32 StartIdx = DecayIdx - FModalSynth::DecayBin;
					const int32 EndIdx = StartIdx + FModalSynth::NumParamsPerModal;
					for(int i = StartIdx; i < EndIdx; i++)
					{
						ValidParams.Emplace(OrgModalParams[i]);
					}
				}
			}

			const int32 NumTrueModal = ValidParams.Num() / FModalSynth::NumParamsPerModal;
			if(NumTrueModal > 0)
				OutputModal = FImpactModalObjWriteRef::CreateNew(MakeShared<FImpactModalObjAssetProxy>(ValidParams, NumTrueModal));
			else
				OutputModal = FImpactModalObjWriteRef::CreateNew(MakeShared<FImpactModalObjAssetProxy>(OrgModalParams.Slice(0, 1), 1));
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalDecayFilterVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputDecayMin), DecayMin);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputDecayMax), DecayMax);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalDecayFilterVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputModal), OutputModal);
		}

		void Reset(const IOperator::FResetParams& InParams)
		{
			CreateModalObj();
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ImpactModalDecayFilterVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputDecayMin), DecayMin);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputDecayMax), DecayMax);
			
			FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputModal), OutputModal);
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

		}

	private:
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;

		float DecayMin;
		float DecayMax;

		FImpactModalObjWriteRef OutputModal;
	};

	TUniquePtr<IOperator> FImpactModalDecayFilterOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace ImpactModalDecayFilterVertexNames;
		
		const FInputVertexInterfaceData& Inputs = InParams.InputData;
		FImpactModalDecayFilterOpArgs Args =
		{
			InParams.OperatorSettings,

			Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<float>(METASOUND_GET_PARAM_NAME(InputDecayMin), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<float>(METASOUND_GET_PARAM_NAME(InputDecayMax), InParams.OperatorSettings)
			
		};

		return MakeUnique<FImpactModalDecayFilterOperator>(Args);
	}
	
	const FNodeClassMetadata& FImpactModalDecayFilterOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata
			{
				FNodeClassName{ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact Modal Decay Filter"), "ModalDecayFilter"},
				1, // Major Version
				0, // Minor Version
				METASOUND_LOCTEXT("ImpactModalDecayFilterDisplayName", "Impact Modal Decay Filter"),
				METASOUND_LOCTEXT("ImpactModalDecayFilterDesc", "Filter out modals based on their decay rate. If no modal has a valid decay rate, the first modal is kept to avoid errors in subsequent nodes. This node only runs on init to avoid changing the number of modals when running."),
				PluginAuthor,
				PluginNodeMissingPrompt,
				GetVertexInterface(),
				{NodeCategories::Math},
				{},
				FNodeDisplayStyle{}
			};

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& FImpactModalDecayFilterOperator::GetVertexInterface()
	{
		using namespace ImpactModalDecayFilterVertexNames;
			
		static const FVertexInterface VertexInterface(
			FInputVertexInterface(
				TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayMin), 0.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayMax), 1000.f)
				),
			FOutputVertexInterface(
				TOutputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputModal))
			)
		);
			
		return VertexInterface;
	}

	// Node Class
	class FImpactModalDecayFilterNode : public FNodeFacade
	{
	public:
		FImpactModalDecayFilterNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FImpactModalDecayFilterOperator>())
		{
		}
	};

	// Register node
	METASOUND_REGISTER_NODE(FImpactModalDecayFilterNode)
}

#undef LOCTEXT_NAMESPACE
