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
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundVertex.h"
#include "ModalSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactModalRandomizerNode"

#define IMP_RAND_INTERP_THRESH (2.f)
#define IMP_RAND_INTERP_STOP (1.5f)

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactModalRandomizerVertexNames
	{
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputSeed, "Seed", "The seed of the randomizer.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params.")
		
		METASOUND_PARAM(InputTimeStep, "Time Step", "The time step to get new values from the randomizer.")
		METASOUND_PARAM(InputInterpTime, "Interp Time", "The interpolating time between the current and the new random values.")
		METASOUND_PARAM(InputRandChance, "Rand Chance", "Range [0, 1]. The chance a new value is applied on the parameters of each modal.")
		METASOUND_PARAM(InputAmpRand, "Amp Rand Range", "Percentage [0, 1]. The total randomness range (+/-) applied to the amplitude of each modal in percentage.")
		METASOUND_PARAM(InputDecayRand, "Decay Rand Range", ">= 0.f. The total randomness range (+/-) applied to the decay rate of each modal in percentage. Avoid using large values. Because decay rates will affect amplitude overtime, this parameter isn't interpolated between frames.")
		METASOUND_PARAM(InputPitchRand, "Pitch Rand Range", ">= 0.f. The total randomness range (+/-) applied to the pitch of each modal in semitone.")
		METASOUND_PARAM(OutputModal, "Output Modal", "The output modal.");
	}

	struct FImpactModalRandomizerOpArgs
	{
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 Seed;
		int32 NumModals;

		FTimeReadRef TimeStep;
		FTimeReadRef InterpTime;
		FFloatReadRef RandChance;
		FFloatReadRef AmpRand;
		FFloatReadRef DecayRand;
		FFloatReadRef PitchRand;
	};
	
	class FImpactModalRandomizerOperator : public TExecutableOperator<FImpactModalRandomizerOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FImpactModalRandomizerOperator(const FImpactModalRandomizerOpArgs& InArgs)
			: OperatorSettings(InArgs.OperatorSettings)
		    , ModalParams(InArgs.ModalParams)
			, Seed(InArgs.Seed)
		    , NumModals(InArgs.NumModals)
			, TimeStep(InArgs.TimeStep)
			, InterpTime(InArgs.InterpTime)
			, RandChance(InArgs.RandChance)
			, AmpRand(InArgs.AmpRand)
			, DecayRand(InArgs.DecayRand)
			, PitchRand(InArgs.PitchRand)
			, OutputModal(FImpactModalObjWriteRef::CreateNew())
		{
			SamplingRate = OperatorSettings.GetSampleRate();
			LastRandomTime = 0.f;
			FrameTime = OperatorSettings.GetNumFramesPerBlock() / SamplingRate;
			CreateModalObj();
		}

		void CreateModalObj()
		{
			const FImpactModalObjAssetProxyPtr& ModalPtr = ModalParams->GetProxy();
			if(!ModalPtr.IsValid())
				return;
			
			OutputModal = FImpactModalObjWriteRef::CreateNew(MakeShared<FImpactModalObjAssetProxy>(ModalPtr, NumModals));
			OutputProxy = OutputModal->GetProxy();

			const int32 NumTrueModal = OutputProxy->GetNumModals();

			PitchDeltaBuffer.SetNumUninitialized(NumTrueModal);
			FMemory::Memzero(PitchDeltaBuffer.GetData(), NumTrueModal * sizeof(float));
			
			RandomStream = Seed > -1 ? FRandomStream(Seed) : FRandomStream(FMath::Rand());
			
			TArrayView<const float> OutputModalData = OutputProxy->GetParams();

			const float AmpRandRange = *AmpRand;
			const float DecayRandRange = *DecayRand;
			const float PitchRandRange = GetPitchScaleClamped(*PitchRand) - 1.f;
			const float CurRandChance = *RandChance;

			ModalInterpPercent.SetNumUninitialized(NumTrueModal);
			DecayBuffer.SetNumUninitialized(NumTrueModal);
			AmpBuffer.SetNumUninitialized(NumTrueModal);
			for(int i = 0, j = 0; j < NumTrueModal; i += FModalSynth::NumParamsPerModal, j++)
			{
				ModalInterpPercent[j] = IMP_RAND_INTERP_THRESH; // no interp
				const float CurAmp = FMath::Abs(OutputModalData[i]);
				if(CurAmp < 1e-5f || RandomStream.FRand() > CurRandChance)
				{
					AmpBuffer[j] =  FMath::Abs(OutputModalData[i]);
					DecayBuffer[j] = OutputModalData[i + FModalSynth::DecayBin];
					continue;
				}
				
				const float NewAmp = OutputModalData[i] * FMath::Max(1e-5f, 1.0f + FRandBipolar() * AmpRandRange);
				AmpBuffer[j] = FMath::Abs(NewAmp);
				OutputProxy->SetModalParam(i, NewAmp);
				

				const int32 DecayIdx = i + FModalSynth::DecayBin;
				const float NewDecay = FMath::Clamp(OutputModalData[DecayIdx] * (1.0f + FRandBipolar() * DecayRandRange), FModalSynth::DecayMin, FModalSynth::DecayMax);
				OutputProxy->SetModalParam(DecayIdx, NewDecay);
				DecayBuffer[j] = NewDecay;
				
				const int32 FreqIdx = i + FModalSynth::FreqBin;
				const float NewPitch = OutputModalData[FreqIdx] * (1.0f + FRandBipolar() * PitchRandRange);
				OutputProxy->SetModalParam(FreqIdx, NewPitch);
			}

			OutputProxy->SetIsParamChanged(true);
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalRandomizerVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTimeStep), TimeStep);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputInterpTime), InterpTime);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRandChance), RandChance);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmpRand), AmpRand);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayRand), DecayRand);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchRand), PitchRand);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactModalRandomizerVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputModal), OutputModal);
		}

		void Reset(const IOperator::FResetParams& InParams)
		{
			CreateModalObj();
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ImpactModalRandomizerVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTimeStep), TimeStep);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputInterpTime), InterpTime);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRandChance), RandChance);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmpRand), AmpRand);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayRand), DecayRand);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchRand), PitchRand);
			
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

			const float NumTrueModals = OutputProxy->GetNumModals();
			if(NumTrueModals > ModalPtr->GetNumModals())
			{
				UE_LOG(LogImpactSFXSynth, Error, TEXT("ImpactModalRandomizer::Execute: number of modals are changed while running!"));
				return;
			}

			Randomize(ModalPtr, NumTrueModals);
		}

		void Randomize(const FImpactModalObjAssetProxyPtr& ModalPtr, const float NumTrueModals)
		{
			const float CurrentInterpTime = InterpTime->GetSeconds();
			const float InterpSpeed = FMath::Min(1.f, FrameTime / FMath::Max(CurrentInterpTime, 1e-5f));
			
			TArrayView<const float> ModalData = ModalPtr->GetParams();
			TArrayView<const float> OutModalData = OutputProxy->GetParams();
			LastRandomTime += FrameTime;
			
			if(ModalPtr->IsParamChanged())
			{
				//If input modal params changed, copy and ignore all current random values;
				OutputProxy->CopyAllParams(ModalData);
				OutputProxy->SetIsParamChanged(true);
				FMemory::Memzero(PitchDeltaBuffer.GetData(), PitchDeltaBuffer.Num() * sizeof(float));
				for(int i = 0, j = 0; j < NumTrueModals; j++, i += FModalSynth::NumParamsPerModal)
				{
					ModalInterpPercent[j] = IMP_RAND_INTERP_THRESH;
					AmpBuffer[j] = FMath::Abs(ModalData[i]);
				}
				
				//Force random on next frame to update other buffers
				LastRandomTime = TimeStep->GetSeconds() + FrameTime;
				return;
			}

			bool bIsModalUpdated = false;
			for (int j = 0; j < NumTrueModals; j++)
			{
				if(ModalInterpPercent[j] < 1.f)
				{
					ModalInterpPercent[j] += InterpSpeed;
					const float Index = j * FModalSynth::NumParamsPerModal + FModalSynth::FreqBin;
					OutputProxy->SetModalParam(Index, OutModalData[Index] + InterpSpeed * PitchDeltaBuffer[j]);
					bIsModalUpdated = true;
				}
				else if (ModalInterpPercent[j] < IMP_RAND_INTERP_STOP)
				{
					ModalInterpPercent[j] = IMP_RAND_INTERP_THRESH;
					const float Index = j * FModalSynth::NumParamsPerModal + FModalSynth::DecayBin;
					OutputProxy->SetModalParam(Index, DecayBuffer[j]);
					bIsModalUpdated = true;
				}
			}
			
			if(LastRandomTime < TimeStep->GetSeconds())
			{
				OutputProxy->SetIsParamChanged(bIsModalUpdated);
				return;
			}
			
			LastRandomTime = 0.f;
						
			const float AmpRandRange = *AmpRand;
			const float DecayRandRange = *DecayRand;
			const float PitchRandRange = GetPitchScaleClamped(*PitchRand) - 1.f;
			const float CurRandChance = *RandChance;
			const float AmpInterpSpeed = 1.0f / CurrentInterpTime;
			const bool bAmpRand = !FMath::IsNearlyZero(AmpRandRange, 1e-3f);
			for(int i = 0, j = 0; j < NumTrueModals; i += FModalSynth::NumParamsPerModal, j++)
			{
				const float OrgAmp = FMath::Abs(ModalData[i]);
				if(OrgAmp < 1e-5f || RandomStream.FRand() > CurRandChance)
					continue;

				ModalInterpPercent[j] = 0.f;
				
				const int32 DecayIdx = i + FModalSynth::DecayBin;
				const float Decay = FMath::Clamp(ModalData[DecayIdx] * (1.0f + DecayRandRange * FRandBipolar()), FModalSynth::DecayMin, FModalSynth::DecayMax);
				DecayBuffer[j] = Decay;

				const int32 FreqIdx = i + FModalSynth::FreqBin;
				PitchDeltaBuffer[j] = ModalData[FreqIdx] * (1.0f + PitchRandRange * FRandBipolar()) - OutModalData[FreqIdx];

				if(bAmpRand && OrgAmp > 1e-3f)
				{
					const float CurAmp = AmpBuffer[j];
					const float NewAmp = FMath::Max(1e-5f, OrgAmp * (1.0f + AmpRandRange * FRandBipolar()));
					AmpBuffer[j] = NewAmp;
					const float TempDecay = -FMath::Loge(NewAmp / CurAmp) * AmpInterpSpeed;
					OutputProxy->SetModalParam(DecayIdx, TempDecay + Decay);
				}
				else
					OutputProxy->SetModalParam(DecayIdx, Decay);
			}

			OutputProxy->SetIsParamChanged(true);
		}

		float FRandBipolar() const
		{
			return RandomStream.FRand() - 0.5f;
		}
		
	private:
		FOperatorSettings OperatorSettings;
		
		FImpactModalObjReadRef ModalParams;
		int32 Seed;
		int32 NumModals;

		FTimeReadRef TimeStep;
		FTimeReadRef InterpTime;
		FFloatReadRef RandChance;
		FFloatReadRef AmpRand;
		FFloatReadRef DecayRand;
		FFloatReadRef PitchRand;

		float LastRandomTime;
		float FrameTime;

		FRandomStream RandomStream;

		FAlignedFloatBuffer AmpBuffer;
		FAlignedFloatBuffer DecayBuffer;
		FAlignedFloatBuffer PitchDeltaBuffer;
		
		TArray<float> ModalInterpPercent;
		
		FImpactModalObjWriteRef OutputModal;
		FImpactModalObjAssetProxyPtr OutputProxy;

		float SamplingRate;
	};

	TUniquePtr<IOperator> FImpactModalRandomizerOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace ImpactModalRandomizerVertexNames;
		
		const FInputVertexInterfaceData& Inputs = InParams.InputData;
		FImpactModalRandomizerOpArgs Args =
		{
			InParams.OperatorSettings,

			Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputSeed), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),

			Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputTimeStep), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputInterpTime), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputRandChance), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmpRand), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayRand), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchRand), InParams.OperatorSettings)
			
		};

		return MakeUnique<FImpactModalRandomizerOperator>(Args);
	}
	
	const FNodeClassMetadata& FImpactModalRandomizerOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata
			{
				FNodeClassName{ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact Modal Randomizer Interp"), "ModalRandomizer"},
				1, // Major Version
				0, // Minor Version
				METASOUND_LOCTEXT("ImpactModalRandomizerDisplayName", "Impact Modal Randomizer Interp"),
				METASOUND_LOCTEXT("ImpactModalRandomizerDesc", "Only work with Impact (External) Force Synth nodes. Smoothly apply randomness to parameters of a modal object by interpolating."),
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

	const FVertexInterface& FImpactModalRandomizerOperator::GetVertexInterface()
	{
		using namespace ImpactModalRandomizerVertexNames;
			
		static const FVertexInterface VertexInterface(
			FInputVertexInterface(
				TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSeed), -1),
				TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),

				TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTimeStep), 0.1f),
				TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputInterpTime), 0.1f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRandChance), 0.5f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmpRand), 0.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayRand), 0.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchRand), 0.f)
				),
			FOutputVertexInterface(
				TOutputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputModal))
			)
		);
			
		return VertexInterface;
	}

	// Node Class
	class FImpactModalRandomizerNode : public FNodeFacade
	{
	public:
		FImpactModalRandomizerNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FImpactModalRandomizerOperator>())
		{
		}
	};

	// Register node
	METASOUND_REGISTER_NODE(FImpactModalRandomizerNode)
}

#undef LOCTEXT_NAMESPACE
