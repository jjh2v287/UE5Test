// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactSFXSynthLog.h"
#include "ImpactSynthEngineNodesName.h"
#include "Runtime/Launch/Resources/Version.h"
#include "DSP/Dsp.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundImpactModalObj.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundTime.h"
#include "MetasoundVertex.h"
#include "ModalSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactModalMergerNodes"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactModalMergerVertexNames
	{
		METASOUND_PARAM(InputModalParams, "Modal Params A", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModal, "Number of Modals A", "<= 0 means using all modal params.")
		METASOUND_PARAM(InputAmplitudeScale, "Amplitude Scale A", "Scale amplitude of the synthesized SFX.")
		METASOUND_PARAM(InputDecayScale, "Decay Scale A", "Scale decay rate of the synthesized SFX.")
		METASOUND_PARAM(InputPitchShift, "Pitch Shift A", "Shift pitches by semitone.")

		METASOUND_PARAM(InputModalParamsB, "Modal Params B", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModalB, "Number of Modals B", "<= 0 means using all modal params.")
		METASOUND_PARAM(InputAmplitudeScaleB, "Amplitude Scale B", "Scale amplitude of the synthesized SFX.")
		METASOUND_PARAM(InputDecayScaleB, "Decay Scale B", "Scale decay rate of the synthesized SFX.")
		METASOUND_PARAM(InputPitchShiftB, "Pitch Shift B", "Shift pitches by semitone.")
		
		METASOUND_PARAM(OutputModal, "Output Modal", "The output modal.");
	}

	struct FImpactModalMergerOpArgs
	{
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;

		FImpactModalObjReadRef ModalParamsB;
		int32 NumModalsB;
		FFloatReadRef AmplitudeScaleB;
		FFloatReadRef DecayScaleB;
		FFloatReadRef PitchShiftB;
	};
	
	class FImpactModalMergerOperator : public TExecutableOperator<FImpactModalMergerOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FImpactModalMergerOperator(const FImpactModalMergerOpArgs& InArgs)
			: OperatorSettings(InArgs.OperatorSettings)
		    , ModalParams(InArgs.ModalParams)
		    , NumModals(InArgs.NumModals)
		    , AmplitudeScale(InArgs.AmplitudeScale)
		    , DecayScale(InArgs.DecayScale)
		    , PitchShift(InArgs.PitchShift)
			, ModalParamsB(InArgs.ModalParamsB)
			, NumModalsB(InArgs.NumModalsB)
			, AmplitudeScaleB(InArgs.AmplitudeScaleB)
			, DecayScaleB(InArgs.DecayScaleB)
			, PitchShiftB(InArgs.PitchShiftB)
			, OutputModal(FImpactModalObjWriteRef::CreateNew())
		{
			CreateMergeModalObj();
		}

		void CreateMergeModalObj()
		{
			const FImpactModalObjAssetProxyPtr& ModalAPtr = ModalParams->GetProxy();
			const FImpactModalObjAssetProxyPtr& ModalBPtr = ModalParamsB->GetProxy();
			if(!ModalAPtr.IsValid() || !ModalBPtr.IsValid())
				return;

			CurrentAmpScaleA = *AmplitudeScale;
			CurrentDecayScaleA = *DecayScale;
			CurrentPitchShiftA = GetPitchScaleClamped(*PitchShift);
			
			CurrentAmpScaleB = *AmplitudeScaleB;
			CurrentDecayScaleB = *DecayScaleB;
			CurrentPitchShiftB = GetPitchScaleClamped(*PitchShiftB);
			
			const FImpactModalModInfo ModA = FImpactModalModInfo(NumModals, CurrentAmpScaleA, CurrentDecayScaleA, CurrentPitchShiftA);
			const FImpactModalModInfo ModB = FImpactModalModInfo(NumModalsB, CurrentAmpScaleB, CurrentDecayScaleB, CurrentPitchShiftB);
			
			OutputModal = FImpactModalObjWriteRef::CreateNew(MakeShared<FImpactModalObjAssetProxy>(ModalAPtr, ModA, ModalBPtr, ModB));
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalMergerVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			InOutVertexData.BindVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			InOutVertexData.BindVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParamsB), ModalParamsB);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumModalB), NumModalsB);
			InOutVertexData.BindVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScaleB), AmplitudeScaleB);
			InOutVertexData.BindVertex(METASOUND_GET_PARAM_NAME(InputDecayScaleB), DecayScaleB);
			InOutVertexData.BindVertex(METASOUND_GET_PARAM_NAME(InputPitchShiftB), PitchShiftB);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalMergerVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputModal), OutputModal);
		}

		void Reset(const IOperator::FResetParams& InParams)
		{
			CreateMergeModalObj();
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ImpactModalMergerVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			Inputs.BindVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			Inputs.BindVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParamsB), ModalParamsB);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumModalB), NumModalsB);
			Inputs.BindVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScaleB), AmplitudeScaleB);
			Inputs.BindVertex(METASOUND_GET_PARAM_NAME(InputDecayScaleB), DecayScaleB);
			Inputs.BindVertex(METASOUND_GET_PARAM_NAME(InputPitchShiftB), PitchShiftB);
			
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
			const FImpactModalObjAssetProxyPtr& ModalAPtr = ModalParams->GetProxy();
			const FImpactModalObjAssetProxyPtr& ModalBPtr = ModalParamsB->GetProxy();

			const FImpactModalObjAssetProxyPtr& MergePtr = OutputModal->GetProxy();
			
			if(!MergePtr.IsValid() || !ModalAPtr.IsValid() || !ModalBPtr.IsValid())
				return;
			
			const float NewPitchShiftA = GetPitchScaleClamped(*PitchShift);
			const bool bIsAChanged = ModalAPtr->IsParamChanged()
							   || !FMath::IsNearlyEqual(CurrentAmpScaleA, *AmplitudeScale, 1e-3f)
							   || !FMath::IsNearlyEqual(CurrentDecayScaleA, *DecayScale, 1e-3f)
							   || !FMath::IsNearlyEqual(CurrentPitchShiftA, NewPitchShiftA, 1e-3f);
				
			const float NewPitchShiftB = GetPitchScaleClamped(*PitchShiftB);
			const bool bIsBChanged = ModalBPtr->IsParamChanged()
			                   || !FMath::IsNearlyEqual(CurrentAmpScaleB, *AmplitudeScaleB, 1e-3f)
							   || !FMath::IsNearlyEqual(CurrentDecayScaleB, *DecayScaleB, 1e-3f)
							   || !FMath::IsNearlyEqual(CurrentPitchShiftB, NewPitchShiftB, 1e-3f);

			if(!bIsAChanged && !bIsBChanged)
			{
				MergePtr->SetIsParamChanged(false);
				return;
			}
				
			const int32 TotalModalsA = ModalAPtr->GetNumModals();
			const int32 TotalModalsB = ModalBPtr->GetNumModals();
			
			const int32 NumModalsAUsed = NumModals > 0 ? FMath::Min(NumModals, TotalModalsA) : TotalModalsA;
			const int32 NumModalsBUsed = NumModalsB > 0 ? FMath::Min(NumModalsB, TotalModalsB) : TotalModalsB;

			const int32 TotalModalMerge = NumModalsAUsed + NumModalsBUsed;
			if(TotalModalMerge != MergePtr->GetNumModals())
			{
				UE_LOG(LogImpactSFXSynth, Error, TEXT("ImpactModalMerger::Execute: num total modals is changed!"));
				return;
			}

			if(bIsAChanged)
			{
				CurrentAmpScaleA = *AmplitudeScale;
				CurrentDecayScaleA = *DecayScale;
				CurrentPitchShiftA = NewPitchShiftA;
			}

			if(bIsBChanged)
			{
				CurrentAmpScaleB = *AmplitudeScaleB;
				CurrentDecayScaleB = *DecayScaleB;
				CurrentPitchShiftB = NewPitchShiftB;
			}
			
			const int32 MinNumParams = FMath::Min(NumModalsAUsed, NumModalsBUsed) * FModalSynth::NumParamsPerModal;

			TArrayView<const float> ModA = ModalAPtr->GetParams();
			TArrayView<const float> ModB = ModalBPtr->GetParams();
			int i = 0; 
			int j = 0;
			int k = 0;
			if(bIsAChanged && bIsBChanged)
			{
				for(; i < MinNumParams; i++, j++, k++)
				{
					MergePtr->SetModalParam(k, ModA[i] * CurrentAmpScaleA);
					i++;
					k++;
					MergePtr->SetModalParam(k, ModA[i] * CurrentDecayScaleA);
					i++;
					k++;
					MergePtr->SetModalParam(k, ModA[i] * CurrentPitchShiftA);
					k++;
				
					MergePtr->SetModalParam(k, ModB[j] * CurrentAmpScaleB);
					j++;
					k++;
					MergePtr->SetModalParam(k, ModB[j] * CurrentDecayScaleB);
					j++;
					k++;
					MergePtr->SetModalParam(k, ModB[j] * CurrentPitchShiftB);
				}
			}
			else if(bIsAChanged)
			{
				for(; i < MinNumParams; i++)
				{
					MergePtr->SetModalParam(k, ModA[i] * CurrentAmpScaleA);
					i++;
					k++;
					MergePtr->SetModalParam(k, ModA[i] * CurrentDecayScaleA);
					i++;
					k++;
					MergePtr->SetModalParam(k, ModA[i] * CurrentPitchShiftA);
					k += 4;
				}
			}
			else
			{
				for(; j < MinNumParams; j++, k++)
				{
					k += 3;
					MergePtr->SetModalParam(k, ModB[j] * CurrentAmpScaleB);
					j++;
					k++;
					MergePtr->SetModalParam(k, ModB[j] * CurrentDecayScaleB);
					j++;
					k++;
					MergePtr->SetModalParam(k, ModB[j] * CurrentPitchShiftB);
				}
			}

			if(bIsAChanged)
			{
				const int32 NumUsedParamsA = NumModalsAUsed * FModalSynth::NumParamsPerModal;
				for(; i < NumUsedParamsA; i++, k++)
				{
					MergePtr->SetModalParam(k, ModA[i] * CurrentAmpScaleA);
					i++;
					k++;
					MergePtr->SetModalParam(k, ModA[i] * CurrentDecayScaleA);
					i++;
					k++;
					MergePtr->SetModalParam(k, ModA[i] * CurrentPitchShiftA);
				}
			}

			if(bIsBChanged)
			{
				const int32 NumUsedParamsB = NumModalsBUsed * FModalSynth::NumParamsPerModal;
				for(; j < NumUsedParamsB; j++, k++)
				{
					MergePtr->SetModalParam(k, ModB[j] * CurrentAmpScaleB);
					j++;
					k++;
					MergePtr->SetModalParam(k, ModB[j] * CurrentDecayScaleB);
					j++;
					k++;
					MergePtr->SetModalParam(k, ModB[j] * CurrentPitchShiftB);
				}
			}

			MergePtr->SetIsParamChanged(true);
		}

	private:
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;

		FImpactModalObjReadRef ModalParamsB;
		int32 NumModalsB;
		FFloatReadRef AmplitudeScaleB;
		FFloatReadRef DecayScaleB;
		FFloatReadRef PitchShiftB;
		
		FImpactModalObjWriteRef OutputModal;

		float CurrentAmpScaleA;
		float CurrentDecayScaleA;
		float CurrentPitchShiftA;
		
		float CurrentAmpScaleB;
        float CurrentDecayScaleB;
        float CurrentPitchShiftB;
	};

	TUniquePtr<IOperator> FImpactModalMergerOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace ImpactModalMergerVertexNames;
		
		const FInputVertexInterfaceData& Inputs = InParams.InputData;
		FImpactModalMergerOpArgs Args =
		{
			InParams.OperatorSettings,

			Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScale), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShift), InParams.OperatorSettings),

			Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParamsB), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumModalB), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmplitudeScaleB), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScaleB), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShiftB), InParams.OperatorSettings)
		};

		return MakeUnique<FImpactModalMergerOperator>(Args);
	}
	
	const FNodeClassMetadata& FImpactModalMergerOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata
			{
				FNodeClassName{ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact Modal Merger"), "ModalMerger"},
				1, // Major Version
				0, // Minor Version
				METASOUND_LOCTEXT("ImpactModalMergerDisplayName", "Impact Modal Merger"),
				METASOUND_LOCTEXT("ImpactModalMergerDesc", "Merge two modal objects into a new object."),
				PluginAuthor,
				PluginNodeMissingPrompt,
				GetVertexInterface(),
				{NodeCategories::Mix},
				{},
				FNodeDisplayStyle{}
			};

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& FImpactModalMergerOperator::GetVertexInterface()
	{
		using namespace ImpactModalMergerVertexNames;
			
		static const FVertexInterface VertexInterface(
			FInputVertexInterface(
				TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmplitudeScale), 1.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScale), 1.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchShift), 0.f),
					
				TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParamsB)),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModalB), -1),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmplitudeScaleB), 1.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScaleB), 1.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchShiftB), 0.f)
				),
			FOutputVertexInterface(
				TOutputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputModal))
			)
		);
			
		return VertexInterface;
	}

	// Node Class
	class FImpactModalMergerNode : public FNodeFacade
	{
	public:
		FImpactModalMergerNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FImpactModalMergerOperator>())
		{
		}
	};

	// Register node
	METASOUND_REGISTER_NODE(FImpactModalMergerNode)
}

#undef LOCTEXT_NAMESPACE
