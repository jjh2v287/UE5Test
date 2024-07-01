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

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactModalDecayClampNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactModalDecayClampVertexNames
	{
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params.")
		METASOUND_PARAM(InputDecayScale, "Decay Scale", "Apply a decay scale to all modals before clamping.")
		METASOUND_PARAM(InputDecayMin, "Decay Min", "The minimum decay. Larger than or equal to 0.")
		METASOUND_PARAM(InputDecayMax, "Decay Max", "The maximum decay. Must be larger than Decay Min.")
		
		METASOUND_PARAM(OutputModal, "Output Modal", "The output modal.");
	}

	struct FImpactModalDecayClampOpArgs
	{
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;
		FFloatReadRef DecayScale;
		FFloatReadRef DecayMin;
		FFloatReadRef DecayMax;
	};
	
	class FImpactModalDecayClampOperator : public TExecutableOperator<FImpactModalDecayClampOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FImpactModalDecayClampOperator(const FImpactModalDecayClampOpArgs& InArgs)
			: OperatorSettings(InArgs.OperatorSettings)
		    , ModalParams(InArgs.ModalParams)
		    , NumModals(InArgs.NumModals)
			, DecayScale(InArgs.DecayScale)
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
			
			OutputModal = FImpactModalObjWriteRef::CreateNew(MakeShared<FImpactModalObjAssetProxy>(ModalPtr, NumModals));
			OutputProxy = OutputModal->GetProxy();

			LastDecayScale = 0.f;
			LastDecayMin = 0.f;
			LastDecayMax = 0.f;
			
			DecayClamp(ModalPtr);
		}

		bool DecayClamp(const FImpactModalObjAssetProxyPtr& ModalPtr)
		{
			const float CurrentDecayScale = FMath::Max(*DecayScale, 0.f);
			const float CurrentDecayMin = FMath::Max(*DecayMin, 0.f);
			const float CurrentDecayMax =  FMath::Max(*DecayMax, CurrentDecayMin + 1e-5f);;

			if(!ModalPtr->IsParamChanged()
				&& FMath::IsNearlyEqual(LastDecayScale, CurrentDecayScale, 1e-5f)
				&& FMath::IsNearlyEqual(LastDecayMin, CurrentDecayMin, 1e-5f)
				&& FMath::IsNearlyEqual(LastDecayMax, CurrentDecayMax, 1e-5f))
					return false;

			const TArrayView<const float> OrgModalParams = ModalPtr->GetParams();
			const int32 NumParams = FMath::Min(OutputProxy->GetNumModals(), ModalPtr->GetNumModals()) * FModalSynth::NumParamsPerModal;

			if(ModalPtr->IsParamChanged())
				OutputProxy->CopyAllParams(OrgModalParams);
			
			for(int i = FModalSynth::DecayBin; i < NumParams; i += FModalSynth::NumParamsPerModal)
			{
				const float Decay = OrgModalParams[i];
				OutputProxy->SetModalParam(i, FMath::Clamp(Decay * CurrentDecayScale, CurrentDecayMin, CurrentDecayMax));
			}

			LastDecayScale = CurrentDecayScale;
			LastDecayMin = CurrentDecayMin;
			LastDecayMax = CurrentDecayMax;
			
			return true;
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalDecayClampVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayMin), DecayMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayMax), DecayMax);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalDecayClampVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputModal), OutputModal);
		}

		void Reset(const IOperator::FResetParams& InParams)
		{
			CreateModalObj();
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ImpactModalDecayClampVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayMin), DecayMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayMax), DecayMax);
			
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
			const FImpactModalObjAssetProxyPtr& ModalPtr = ModalParams->GetProxy();
			if(!ModalPtr.IsValid())
				return;
			
			OutputProxy->SetIsParamChanged(DecayClamp(ModalPtr));	
		}

	private:
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;

		FFloatReadRef DecayScale;
		FFloatReadRef DecayMin;
		FFloatReadRef DecayMax;

		float LastDecayScale;
		float LastDecayMin;
		float LastDecayMax;
		
		FImpactModalObjWriteRef OutputModal;
		FImpactModalObjAssetProxyPtr OutputProxy;
	};

	TUniquePtr<IOperator> FImpactModalDecayClampOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace ImpactModalDecayClampVertexNames;
		
		const FInputVertexInterfaceData& Inputs = InParams.InputData;
		FImpactModalDecayClampOpArgs Args =
		{
			InParams.OperatorSettings,

			Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScale), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayMin), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayMax), InParams.OperatorSettings)
			
		};

		return MakeUnique<FImpactModalDecayClampOperator>(Args);
	}
	
	const FNodeClassMetadata& FImpactModalDecayClampOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata
			{
				FNodeClassName{ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact Modal Decay Clamp"), "ModalDecayClamp"},
				1, // Major Version
				0, // Minor Version
				METASOUND_LOCTEXT("ImpactModalDecayClampDisplayName", "Impact Modal Decay Clamp"),
				METASOUND_LOCTEXT("ImpactModalDecayClampDesc", "Clamp the decay rate of all modals."),
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

	const FVertexInterface& FImpactModalDecayClampOperator::GetVertexInterface()
	{
		using namespace ImpactModalDecayClampVertexNames;
			
		static const FVertexInterface VertexInterface(
			FInputVertexInterface(
				TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScale), 1.f),
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
	class FImpactModalDecayClampNode : public FNodeFacade
	{
	public:
		FImpactModalDecayClampNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FImpactModalDecayClampOperator>())
		{
		}
	};

	// Register node
	METASOUND_REGISTER_NODE(FImpactModalDecayClampNode)
}

#undef LOCTEXT_NAMESPACE
