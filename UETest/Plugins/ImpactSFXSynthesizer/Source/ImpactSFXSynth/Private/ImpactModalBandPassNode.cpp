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

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactModalBandPassNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactModalBandPassVertexNames
	{
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params.")

		METASOUND_PARAM(InputCutoffFreq1, "Fcut 1", "The first cutoff frequency. If Fcut1 < Fcut2, this filter is band-pass. Otherwise, band-stop.")
		METASOUND_PARAM(InputCutoffFreq2, "Fcut 2", "The second cutoff frequency. If Fcut1 < Fcut2, this filter is band-pass. Otherwise, band-stop.")
		METASOUND_PARAM(InputFallOff, "Fall Off (dB)", "Should be negative. The fall off slope after the cutoff frequency per decade. "
												       "For example, if the cutoff frequency is 1000Hz and the fall off slope is -20dB. Then, at 100Hz, the gain is reduced by 20dB.")

		METASOUND_PARAM(InputPitchShift, "Pitch Shift", "Apply a pitch shift to all modals before filtering.")
		METASOUND_PARAM(OutputModal, "Output Modal", "The output modal.");
	}

	struct FImpactModalBandPassOpArgs
	{
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;

		FFloatReadRef CutoffFreq1;
		FFloatReadRef CutoffFreq2;
		FFloatReadRef FallOff;
		FFloatReadRef PitchShift;
	};
	
	class FImpactModalBandPassOperator : public TExecutableOperator<FImpactModalBandPassOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FImpactModalBandPassOperator(const FImpactModalBandPassOpArgs& InArgs)
			: OperatorSettings(InArgs.OperatorSettings)
		    , ModalParams(InArgs.ModalParams)
		    , NumModals(InArgs.NumModals)
			, CutoffFreq1(InArgs.CutoffFreq1)
			, CutoffFreq2(InArgs.CutoffFreq2)
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

			LastCutoffFreq1 = 0.f;
			LastCutoffFreq2 = 20e3f;
			LastFallOff = 0.f;
			LastPitchShift = 0.f;
			
			ReduceModalsGain(ModalPtr);
		}

		bool ReduceModalsGain(const FImpactModalObjAssetProxyPtr& ModalPtr)
		{
			const float CurrentCutoffFreq1 = FMath::Max(*CutoffFreq1, 1e-5f);
			const float CurrentCutoffFreq2 = FMath::Max(*CutoffFreq2, 1e-5f);
			const float CurrentFallOff =  *FallOff;
			const float CurrentPitchShift = GetPitchScaleClamped(*PitchShift);
			const bool IsPitchKeep = !ModalPtr->IsParamChanged() && FMath::IsNearlyEqual(LastPitchShift, CurrentPitchShift, 1e-5f);
			if(IsPitchKeep
				&& FMath::IsNearlyEqual(LastCutoffFreq1, CurrentCutoffFreq1, 1e-5f)
				&& FMath::IsNearlyEqual(LastCutoffFreq2, CurrentCutoffFreq2, 1e-5f)
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
					
					const float FallOfSlope1 = Freq < CurrentCutoffFreq1 ? FMath::Pow(CurrentCutoffFreq1 / FMath::Max(Freq, 1.f), CurrentFallOff / 20.0f) : 1.0f;
					const float FallOfSlope2 = Freq > CurrentCutoffFreq2 ? FMath::Pow(Freq / CurrentCutoffFreq2, CurrentFallOff / 20.0f) : 1.f;
					const float FallOfSlope = CurrentCutoffFreq1 < CurrentCutoffFreq2 ? FMath::Min(FallOfSlope1, FallOfSlope2) : FMath::Max(FallOfSlope1, FallOfSlope2);
					const float NewAmp = OrgModalParams[i] * FallOfSlope;
					OutputProxy->SetModalParam(i, NewAmp);
				}
			}
			else
			{
				for(int i = 0, j = 2; j < NumParams; i += NumParamPerModal, j += NumParamPerModal)
				{
					const float Freq = OrgModalParams[j] * CurrentPitchShift;
					OutputProxy->SetModalParam(j, Freq);
					
					const float FallOfSlope1 = Freq < CurrentCutoffFreq1 ? FMath::Pow(CurrentCutoffFreq1 / FMath::Max(Freq, 1.f), CurrentFallOff / 20.0f) : 1.0f;
					const float FallOfSlope2 = Freq > CurrentCutoffFreq2 ? FMath::Pow(Freq / CurrentCutoffFreq2, CurrentFallOff / 20.0f) : 1.f;
					const float FallOfSlope = CurrentCutoffFreq1 < CurrentCutoffFreq2 ? FMath::Min(FallOfSlope1, FallOfSlope2) : FMath::Max(FallOfSlope1, FallOfSlope2);
					const float NewAmp = OrgModalParams[i] * FallOfSlope;
					OutputProxy->SetModalParam(i, NewAmp);
				}
			}

			LastCutoffFreq1 = CurrentCutoffFreq1;
			LastCutoffFreq2 = CurrentCutoffFreq2;
			LastFallOff = CurrentFallOff;
			LastPitchShift = CurrentPitchShift;
			
			return true;
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalBandPassVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCutoffFreq1), CutoffFreq1);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCutoffFreq2), CutoffFreq2);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFallOff), FallOff);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalBandPassVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputModal), OutputModal);
		}

		void Reset(const IOperator::FResetParams& InParams)
		{
			CreateModalObj();
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ImpactModalBandPassVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCutoffFreq1), CutoffFreq1);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCutoffFreq2), CutoffFreq2);
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

		bool bIsBandStop;
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;

		FFloatReadRef CutoffFreq1;
		FFloatReadRef CutoffFreq2;
		FFloatReadRef FallOff;
		FFloatReadRef PitchShift;

		float LastCutoffFreq1;
		float LastCutoffFreq2;
		float LastFallOff;
		float LastPitchShift;
		
		FImpactModalObjWriteRef OutputModal;
		FImpactModalObjAssetProxyPtr OutputProxy;
	};

	TUniquePtr<IOperator> FImpactModalBandPassOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace ImpactModalBandPassVertexNames;
		
		const FInputVertexInterfaceData& Inputs = InParams.InputData;
		FImpactModalBandPassOpArgs Args =
		{
			InParams.OperatorSettings,
			
			Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputCutoffFreq1), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputCutoffFreq2), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputFallOff), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShift), InParams.OperatorSettings)
			
		};

		return MakeUnique<FImpactModalBandPassOperator>(Args);
	}
	
	const FNodeClassMetadata& FImpactModalBandPassOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata
			{
				FNodeClassName{ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact Modal Band-Pass/Stop"), "ModalBandPass"},
				1, // Major Version
				0, // Minor Version
				METASOUND_LOCTEXT("ImpactModalBandPassDisplayName", "Impact Modal Band Pass/Stop"),
				METASOUND_LOCTEXT("ImpactModalBandPassDesc", "Apply band pass/stop filter on a modal object."),
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

	const FVertexInterface& FImpactModalBandPassOperator::GetVertexInterface()
	{
		using namespace ImpactModalBandPassVertexNames;
			
		static const FVertexInterface VertexInterface(
			FInputVertexInterface(			
				TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputCutoffFreq1), 0.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputCutoffFreq2), 20000.f),
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
	class FImpactModalBandPassNode : public FNodeFacade
	{
	public:
		FImpactModalBandPassNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FImpactModalBandPassOperator>())
		{
		}
	};

	// Register node
	METASOUND_REGISTER_NODE(FImpactModalBandPassNode)
}

#undef LOCTEXT_NAMESPACE
