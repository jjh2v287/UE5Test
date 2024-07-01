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
#include "ImpactSFXSynth/Public/Utils.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactModalHighPassNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactModalHighPassVertexNames
	{
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params.")

		METASOUND_PARAM(InputCutoffFreq, "Cutoff Frequency", "The frequency at which the gain starts to fall off. (Not the -3dB point as defined in conventional filters.)")
		METASOUND_PARAM(InputFallOff, "Fall Off (dB)", "Should be negative. The fall off slope after the cutoff frequency per decade. "
												       "For example, if the cutoff frequency is 1000Hz and the fall off slope is -20dB. Then, at 100Hz, the gain is reduced by 20dB.")

		METASOUND_PARAM(InputPitchShift, "Pitch Shift", "Apply a pitch shift to all modals before filtering.")
		METASOUND_PARAM(OutputModal, "Output Modal", "The output modal.");
	}

	struct FImpactModalHighPassOpArgs
	{
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;

		FFloatReadRef CutoffFreq;
		FFloatReadRef FallOff;
		FFloatReadRef PitchShift;
	};
	
	class FImpactModalHighPassOperator : public TExecutableOperator<FImpactModalHighPassOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FImpactModalHighPassOperator(const FImpactModalHighPassOpArgs& InArgs)
			: OperatorSettings(InArgs.OperatorSettings)
		    , ModalParams(InArgs.ModalParams)
		    , NumModals(InArgs.NumModals)
			, CutoffFreq(InArgs.CutoffFreq)
			, FallOff(InArgs.FallOff)
			, PitchShift(InArgs.PitchShift)
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

			LastCutoffFreq = 0.f;
			LastFallOff = 0.f;
			LastPitchShift = 0.f;
			
			ReduceModalsGain(ModalPtr);
		}

		bool ReduceModalsGain(const FImpactModalObjAssetProxyPtr& ModalPtr)
		{
			const float CurrentCutoffFreq = *CutoffFreq;
			const float CurrentFallOff =  *FallOff;
			const float CurrentPitchShift = GetPitchScaleClamped(*PitchShift);
			const bool IsPitchKeep = !ModalPtr->IsParamChanged() && FMath::IsNearlyEqual(LastPitchShift, CurrentPitchShift, 1e-5f);
			if(IsPitchKeep
				&& FMath::IsNearlyEqual(LastCutoffFreq, CurrentCutoffFreq, 1e-5f)
				&& FMath::IsNearlyEqual(LastFallOff, CurrentFallOff, 1e-5f))
					return false;

			const TArrayView<const float> OrgModalParams = ModalPtr->GetParams();
			const TArrayView<const float> OutputModalParams = OutputProxy->GetParams();
			constexpr int32 NumParamPerModal = FModalSynth::NumParamsPerModal;
			const int32 NumParams = FMath::Min(OutputProxy->GetNumModals(), ModalPtr->GetNumModals()) * NumParamPerModal;

			if(ModalPtr->IsParamChanged())
				OutputProxy->CopyAllParams(OrgModalParams);
			
			if(IsPitchKeep)
			{
				for(int i = 0, j = 2; j < NumParams; i += NumParamPerModal, j += NumParamPerModal)
				{
					const float Freq = OutputModalParams[j];
					if(Freq < CurrentCutoffFreq)
					{
						const float FallOfSlope = FMath::Pow(CurrentCutoffFreq / FMath::Max(Freq, 1.f), CurrentFallOff / 20.0f);
						const float NewAmp = OrgModalParams[i] * FallOfSlope;
						OutputProxy->SetModalParam(i, NewAmp);
					}
					else
						OutputProxy->SetModalParam(i, OrgModalParams[i]);
				}
			}
			else
			{
				for(int i = 0, j = 2; j < NumParams; i += NumParamPerModal, j += NumParamPerModal)
				{
					const float Freq = OrgModalParams[j] * CurrentPitchShift;
					OutputProxy->SetModalParam(j, Freq);
					if(Freq < CurrentCutoffFreq)
					{
						const float FallOfSlope = FMath::Pow(CurrentCutoffFreq / FMath::Max(Freq, 1.f), CurrentFallOff / 20.0f);
						const float NewAmp = OrgModalParams[i] * FallOfSlope;
						OutputProxy->SetModalParam(i, NewAmp);
					}
					else
						OutputProxy->SetModalParam(i, OrgModalParams[i]);
				}
			}

			LastCutoffFreq = CurrentCutoffFreq;
			LastFallOff = CurrentFallOff;
			LastPitchShift = CurrentPitchShift;
			
			return true;
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalHighPassVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCutoffFreq), CutoffFreq);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFallOff), FallOff);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalHighPassVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputModal), OutputModal);
		}

		void Reset(const IOperator::FResetParams& InParams)
		{
			CreateModalObj();
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ImpactModalHighPassVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCutoffFreq), CutoffFreq);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFallOff), FallOff);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
			
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
			
			OutputProxy->SetIsParamChanged(ReduceModalsGain(ModalPtr));	
		}

	private:
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;

		FFloatReadRef CutoffFreq;
		FFloatReadRef FallOff;
		FFloatReadRef PitchShift;

		float LastCutoffFreq;
		float LastFallOff;
		float LastPitchShift;
		
		FImpactModalObjWriteRef OutputModal;
		FImpactModalObjAssetProxyPtr OutputProxy;
	};

	TUniquePtr<IOperator> FImpactModalHighPassOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace ImpactModalHighPassVertexNames;
		
		const FInputVertexInterfaceData& Inputs = InParams.InputData;
		FImpactModalHighPassOpArgs Args =
		{
			InParams.OperatorSettings,

			Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputCutoffFreq), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputFallOff), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShift), InParams.OperatorSettings)
			
		};

		return MakeUnique<FImpactModalHighPassOperator>(Args);
	}
	
	const FNodeClassMetadata& FImpactModalHighPassOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata
			{
				FNodeClassName{ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact Modal High Pass"), "ModalHighPass"},
				1, // Major Version
				0, // Minor Version
				METASOUND_LOCTEXT("ImpactModalHighPassDisplayName", "Impact Modal High Pass"),
				METASOUND_LOCTEXT("ImpactModalHighPassDesc", "Apply high pass filter on a modal object."),
				PluginAuthor,
				PluginNodeMissingPrompt,
				GetVertexInterface(),
				{NodeCategories::Filters},
				{},
				FNodeDisplayStyle{}
			};

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& FImpactModalHighPassOperator::GetVertexInterface()
	{
		using namespace ImpactModalHighPassVertexNames;
			
		static const FVertexInterface VertexInterface(
			FInputVertexInterface(
				TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputCutoffFreq), 20000.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFallOff), -40.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchShift), 0.f)
				),
			FOutputVertexInterface(
				TOutputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputModal))
			)
		);
			
		return VertexInterface;
	}

	// Node Class
	class FImpactModalHighPassNode : public FNodeFacade
	{
	public:
		FImpactModalHighPassNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FImpactModalHighPassOperator>())
		{
		}
	};

	// Register node
	METASOUND_REGISTER_NODE(FImpactModalHighPassNode)
}

#undef LOCTEXT_NAMESPACE
