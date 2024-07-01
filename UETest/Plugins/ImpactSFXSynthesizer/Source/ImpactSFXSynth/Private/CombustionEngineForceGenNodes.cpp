// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "ImpactSFXSynthLog.h"
#include "ImpactSynthEngineNodesName.h"
#include "DSP/Dsp.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundImpactModalObj.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "CombustionEngineForceGen.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_CombustionEngineForceGenNodes"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace CombustionEngineForceGenVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "stop synthesizing.")		

		METASOUND_PARAM(InputSeed, "Seed", "Radomizer seed. If <= -1, use random seed.")
		METASOUND_PARAM(InputNumPulsePerRevolution, "Num Pulse Per Revolution", "This parameter depends on the type of engine. For example, a four-stroke and 8 cylinders engine has 0.5*8 = 4 pulses per revolution. A two-stroke and 2 cylinders engine has 1*2 = 2 pulses per revolution.")
		
		METASOUND_PARAM(InputRPM, "RPM", "Current revolution per minutes.")
		METASOUND_PARAM(InputRPMFluctuation, "RPM Fluctuation", "The fluctation range of RPM. True RPM = Rand(RPM - Fluctuation/2, RPM + Fluctuation/2)")

		METASOUND_PARAM(InputCombustionCyclePercent, "Combustion Cycle Percent", "The combustion duration per cycle of a cyclinder.")
		METASOUND_PARAM(InputCombustionStrengthMin, "Combustion Strength Min", "The minimum strength of combustion forces.")
		METASOUND_PARAM(InputCombustionStrengthRange, "Combustion Strength Range", "Combustion Strength = Rand(Combustion Strength Min, Combustion Strength Min + Range).")
		
		METASOUND_PARAM(InputExhaustStrengthMin, "Exhaustion Strength Min", "The minimum strength of exhaustion forces.")
		METASOUND_PARAM(InputExhaustStrengthRange, "Exhaustion Strength Range", "Exhaustion Strength = Rand(Exhaustion Strength Min, Exhaustion Strength Min + Range).")

		METASOUND_PARAM(InputNoiseStrength, "Noise Strength", "The strength of noise added to the output force.")
		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the SFX energy decays to zero or reach the specified duration.")

		METASOUND_PARAM(OutputForce, "Out Force", "Triggers when Play is triggered.")
	}

	struct FCombustionEngineForceGenOpArgs
	{
		FOperatorSettings Settings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;

		int32 Seed;
		int32 NumPulsesPerRev;
		
		FFloatReadRef RPM;
		FFloatReadRef RPMFluctuation;
		
		FFloatReadRef CombustionCyclePercent;
		FFloatReadRef CombustionStrengthMin;
		FFloatReadRef CombustionStrengthRange;
		
		FFloatReadRef ExhaustStrengthMin;
		FFloatReadRef ExhaustStrengthRange;

		FFloatReadRef NoiseStrength;
	};
	
	class FCombustionEngineForceGenOperator : public TExecutableOperator<FCombustionEngineForceGenOperator>
	{
	public:

		FCombustionEngineForceGenOperator(const FCombustionEngineForceGenOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, Seed(InArgs.Seed)
			, NumPulsesPerRev(InArgs.NumPulsesPerRev)
			, RPM(InArgs.RPM)
			, RPMFluctuation(InArgs.RPMFluctuation)
			, CombustionCyclePercent(InArgs.CombustionCyclePercent)
			, CombustionStrengthMin(InArgs.CombustionStrengthMin)
			, CombustionStrengthRange(InArgs.CombustionStrengthRange)
			, ExhaustStrengthMin(InArgs.ExhaustStrengthMin)
			, ExhaustStrengthRange(InArgs.ExhaustStrengthRange)
			, NoiseStrength(InArgs.NoiseStrength)
			, TriggerOnDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
			, ForceWriteBuffer(FAudioBufferWriteRef::CreateNew(InArgs.Settings))
		{
			SamplingRate = OperatorSettings.GetSampleRate();

			// Hold on to a view of the output audio
			OutputAudioView.Emplace(ForceWriteBuffer->GetData(), ForceWriteBuffer->Num());
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace CombustionEngineForceGenVertexNames;
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumPulsePerRevolution), NumPulsesPerRev);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPM), RPM);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPMFluctuation), RPMFluctuation);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCombustionCyclePercent), CombustionCyclePercent);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCombustionStrengthMin), CombustionStrengthMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCombustionStrengthRange), CombustionStrengthRange);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExhaustStrengthMin), ExhaustStrengthMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExhaustStrengthRange), ExhaustStrengthRange);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNoiseStrength), NoiseStrength);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace CombustionEngineForceGenVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputForce), ForceWriteBuffer);
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace CombustionEngineForceGenVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumPulsePerRevolution), NumPulsesPerRev);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPM), RPM);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPMFluctuation), RPMFluctuation);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCombustionCyclePercent), CombustionCyclePercent);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCombustionStrengthMin), CombustionStrengthMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputCombustionStrengthRange), CombustionStrengthRange);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExhaustStrengthMin), ExhaustStrengthMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExhaustStrengthRange), ExhaustStrengthRange);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNoiseStrength), NoiseStrength);
			
			FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputForce), ForceWriteBuffer);
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
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::CombustionEngineForceGenVertexNames::Execute);
			
			TriggerOnDone->AdvanceBlock();
			
			FMemory::Memzero(ForceWriteBuffer->GetData(), OperatorSettings.GetNumFramesPerBlock() * sizeof(float));
			
			ExecuteSubBlocks();
		}

#if ENGINE_MINOR_VERSION > 2
		void Reset(const IOperator::FResetParams& InParams)
		{
			TriggerOnDone->Reset();
			
			ForceWriteBuffer->Zero();

			CombustionEngineForceGen.Reset();
			
			bIsPlaying = false;
		}
#endif
		
	private:
		static constexpr float MaxClamp = 1.e3f;
		static constexpr float MinClamp = 1.e-3f;
		
		void ExecuteSubBlocks()
		{
			// Parse triggers and render audio
			int32 PlayTrigIndex = 0;
			int32 NextPlayFrame = 0;
			const int32 NumPlayTrigs = PlayTrigger->NumTriggeredInBlock();

			int32 StopTrigIndex = 0;
			int32 NextStopFrame = 0;
			const int32 NumStopTrigs = StopTrigger->NumTriggeredInBlock();

			int32 CurrAudioFrame = 0;
			int32 NextAudioFrame = 0;
			const int32 LastAudioFrame = OperatorSettings.GetNumFramesPerBlock() - 1;
			const int32 NoTrigger = OperatorSettings.GetNumFramesPerBlock() << 1; //Same as multiply by 2

			while (NextAudioFrame < LastAudioFrame)
			{
				// get the next Play and Stop indices
				// (play)
				if (PlayTrigIndex < NumPlayTrigs)
				{
					NextPlayFrame = (*PlayTrigger)[PlayTrigIndex];
				}
				else
				{
					NextPlayFrame = NoTrigger;
				}

				// (stop)
				if (StopTrigIndex < NumStopTrigs)
				{
					NextStopFrame = (*StopTrigger)[StopTrigIndex];
				}
				else
				{
					NextStopFrame = NoTrigger;
				}

				// determine the next audio frame we are going to render up to
				NextAudioFrame = FMath::Min(NextPlayFrame, NextStopFrame);

				// no more triggers, rendering to the end of the block
				if (NextAudioFrame == NoTrigger)
				{
					NextAudioFrame = OperatorSettings.GetNumFramesPerBlock();
				}
				
				if (CurrAudioFrame != NextAudioFrame)
				{
					RenderFrameRange(CurrAudioFrame, NextAudioFrame);
					CurrAudioFrame = NextAudioFrame;
				}
				
				if (CurrAudioFrame == NextPlayFrame)
				{
					InitSynthesizers();
					++PlayTrigIndex;
				}

				if (CurrAudioFrame == NextStopFrame)
				{
					bIsPlaying = false;
					TriggerOnDone->TriggerFrame(CurrAudioFrame);
					++StopTrigIndex;
				}
			}
		}

		void InitSynthesizers()
		{
			CombustionEngineForceGen.Reset();
			
			CombustionEngineForceGen = MakeUnique<FCombustionEngineForceGen>(SamplingRate, NumPulsesPerRev, Seed);
			bIsPlaying = CombustionEngineForceGen.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;
			
			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate <= 0)
			{
				UE_LOG(LogImpactSFXSynth, Warning, TEXT("CombustionEngineForceGenNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
				return;
			}
			
			if(CombustionEngineForceGen.IsValid())
			{
				FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);
            	const FCombustionEngineForceParams Params = FCombustionEngineForceParams(*RPM, *RPMFluctuation, *CombustionCyclePercent,
            										*CombustionStrengthMin, *CombustionStrengthRange, 
            										*ExhaustStrengthMin, *ExhaustStrengthRange,
            										*NoiseStrength);
				
				CombustionEngineForceGen->Generate(BufferToGenerate, Params);
			}
			else
			{
				CombustionEngineForceGen.Reset();
				bIsPlaying = false;
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		int32 Seed;
		int32 NumPulsesPerRev;
		FFloatReadRef RPM;
		FFloatReadRef RPMFluctuation;
		
		FFloatReadRef CombustionCyclePercent;
		FFloatReadRef CombustionStrengthMin;
		FFloatReadRef CombustionStrengthRange;
		
		FFloatReadRef ExhaustStrengthMin;
		FFloatReadRef ExhaustStrengthRange;

		FFloatReadRef NoiseStrength;
		
		FTriggerWriteRef TriggerOnDone;
		FAudioBufferWriteRef ForceWriteBuffer;
	
		TUniquePtr<FCombustionEngineForceGen> CombustionEngineForceGen;
		Audio::FMultichannelBufferView OutputAudioView;
				
		float SamplingRate;
		bool bIsPlaying = false;
	};

	class FCombustionEngineForceGenOperatorFactory : public IOperatorFactory
	{
	public:
		FCombustionEngineForceGenOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace CombustionEngineForceGenVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FCombustionEngineForceGenOpArgs Args =
			{
				InParams.OperatorSettings,
				
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputSeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumPulsePerRevolution), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputRPM), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputRPMFluctuation), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputCombustionCyclePercent), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputCombustionStrengthMin), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputCombustionStrengthRange), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputExhaustStrengthMin), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputExhaustStrengthRange), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputNoiseStrength), InParams.OperatorSettings)
			};

			return MakeUnique<FCombustionEngineForceGenOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};

	
	template<typename AudioChannelConfigurationInfoType>
	class TCombustionEngineForceGenNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace CombustionEngineForceGenVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
			
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSeed), -1),
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumPulsePerRevolution), 4),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRPM), 600.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRPMFluctuation), 100.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputCombustionCyclePercent), 0.2f),

					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputCombustionStrengthMin), 0.1f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputCombustionStrengthRange), 0.025f),
					
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputExhaustStrengthMin), 0.1f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputExhaustStrengthRange), 0.025f),

					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNoiseStrength), 0.1f)
					),
				FOutputVertexInterface(
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnPlay)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone)),
					TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputForce))
				)
			);

			// Not use
			// Add audio outputs dependent upon source info.
			// for (const FOutputDataVertex& OutputDataVertex : AudioChannelConfigurationInfoType::GetAudioOutputs())
			// {
			// 	VertexInterface.GetOutputInterface().Add(OutputDataVertex);
			// }

			return VertexInterface;
		}

		static const FNodeClassMetadata& GetNodeInfo()
		{
			auto InitNodeInfo = []() -> FNodeClassMetadata
			{
				FNodeClassMetadata Info;
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Combustion Engine Force Gen"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_CombustionEngineForceGenNodeDescription", "Combustion Engine Force Generator.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("CombustionForceGenKeyword", "Generator") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TCombustionEngineForceGenNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FCombustionEngineForceGenOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TCombustionEngineForceGenNode(const FNodeInitData& InInitData)
			: TCombustionEngineForceGenNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TCombustionEngineForceGenNode() = default;

		virtual FOperatorFactorySharedRef GetDefaultOperatorFactory() const override
		{
			return Factory;
		}

		virtual const FVertexInterface& GetVertexInterface() const override
		{
			return Interface;
		}

		virtual bool SetVertexInterface(const FVertexInterface& InInterface) override
		{
			return InInterface == Interface;
		}

		virtual bool IsVertexInterfaceSupported(const FVertexInterface& InInterface) const override
		{
			return InInterface == Interface;
		}

	private:
		FOperatorFactorySharedRef Factory;
		FVertexInterface Interface;
	};

	struct FCombustionEngineForceGenMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_CombustionEngineForceGenMonoNodeDisplayName", "Combustion Engine Force Gen (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace CombustionEngineForceGenVertexNames;
			return { };
		}
	};
	using FMonoCombustionEngineForceGenNode = TCombustionEngineForceGenNode<FCombustionEngineForceGenMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoCombustionEngineForceGenNode);

}

#undef LOCTEXT_NAMESPACE