// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "CustomStatGroup.h"
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
#include "ImpactExternalForceSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"

DECLARE_CYCLE_STAT(TEXT("ImpactExternalForceSynth - Total Synth Time"), STAT_ImpactExternalForceSynth, STATGROUP_ImpactSFXSynth);

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactExternalForceSynthNodes"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactExternalForceSynthVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "stop synthesizing.")		

		METASOUND_PARAM(InputForce, "In Force", "The external applied force.")
		METASOUND_PARAM(InputForceR, "In Force R", "The external applied force for the right channel.")
		
		METASOUND_PARAM(InputIsForceStop, "Is Force Node Stop", "True if the node creating external force has stopped. This is used to stop this synthesizer automatically.")
		METASOUND_PARAM(InputClamp, "Clamp", "Clamp output to [-1, 1] range.")
		
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params.")
		METASOUND_PARAM(InputAmplitudeScale, "Amplitude Scale", "Scale amplitude of the synthesized SFX.")
		METASOUND_PARAM(InputDecayScale, "Decay Scale", "Scale decay rate of the synthesized SFX.")
		METASOUND_PARAM(InputPitchShift, "Pitch Shift", "Shift pitches by semitone.")
		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the SFX energy decays to zero or reach the specified duration.")
		
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
	}

	struct FImpactExternalForceSynthOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		bool bClamp;

		TArray<FAudioBufferReadRef> InForces;
		TArray<FInputDataVertex> InForceAudioVertices;
		FBoolReadRef bIsAutoStop;

		FImpactModalObjReadRef ModalParams;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;
	};
	
	class FImpactExternalForceSynthOperator : public TExecutableOperator<FImpactExternalForceSynthOperator>
	{
	public:

		FImpactExternalForceSynthOperator(const FImpactExternalForceSynthOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, bClamp(InArgs.bClamp)
			, bIsAutoStop(InArgs.bIsAutoStop)
			, ModalParams(InArgs.ModalParams)
			, NumModals(InArgs.NumModals)
			, AmplitudeScale(InArgs.AmplitudeScale)
			, DecayScale(InArgs.DecayScale)
			, PitchShift(InArgs.PitchShift)
			, TriggerOnDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
		{
			NumOutputChannels = InArgs.OutputAudioVertices.Num();
			SamplingRate = OperatorSettings.GetSampleRate();
			for (int i = 0; i < InArgs.InForceAudioVertices.Num(); i++)
			{
				InForceBufferVertexNames.Add(InArgs.InForceAudioVertices[i].VertexName);
				InForceBuffers.Emplace(InArgs.InForces[i]);
				InForceBufferViews.Emplace(InArgs.InForces[i]->GetData(), InArgs.InForces[i]->Num());
			}
			
			for (const FOutputDataVertex& OutputAudioVertex : InArgs.OutputAudioVertices)
			{
				OutputAudioBufferVertexNames.Add(OutputAudioVertex.VertexName);

				FAudioBufferWriteRef AudioBuffer = FAudioBufferWriteRef::CreateNew(InArgs.Settings);
				OutputAudioBuffers.Add(AudioBuffer);

				// Hold on to a view of the output audio. Audio buffers are only writable
				// by this object and will not be reallocated. 
				OutputAudioView.Emplace(AudioBuffer->GetData(), AudioBuffer->Num());
			}
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactExternalForceSynthVertexNames;
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputClamp), bClamp);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsForceStop), bIsAutoStop);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);

			check(InForceBuffers.Num() == InForceBufferVertexNames.Num());

			for (int32 i = 0; i < InForceBuffers.Num(); i++)
			{
				InOutVertexData.BindReadVertex(InForceBufferVertexNames[i], InForceBuffers[i]);
			}
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactExternalForceSynthVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);

			check(OutputAudioBuffers.Num() == OutputAudioBufferVertexNames.Num());

			for (int32 i = 0; i < OutputAudioBuffers.Num(); i++)
			{
				InOutVertexData.BindReadVertex(OutputAudioBufferVertexNames[i], OutputAudioBuffers[i]);
			}
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ImpactExternalForceSynthVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputClamp), bClamp);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsForceStop), bIsAutoStop);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);

			check(InForceBuffers.Num() == InForceBufferVertexNames.Num());

			for (int32 i = 0; i < InForceBuffers.Num(); i++)
			{
				Inputs.BindReadVertex(InForceBufferVertexNames[i], InForceBuffers[i]);
			}
			
			FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);

			check(OutputAudioBuffers.Num() == OutputAudioBufferVertexNames.Num());

			for (int32 i = 0; i < OutputAudioBuffers.Num(); i++)
			{
				Outputs.BindReadVertex(OutputAudioBufferVertexNames[i], OutputAudioBuffers[i]);
			}
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
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::ImpactExternalForceSynthVertexNames::Execute);

			SCOPE_CYCLE_COUNTER(STAT_ImpactExternalForceSynth);
			
			TriggerOnDone->AdvanceBlock();

			// zero output buffers
			for (const FAudioBufferWriteRef& OutputBuffer : OutputAudioBuffers)
				FMemory::Memzero(OutputBuffer->GetData(), OperatorSettings.GetNumFramesPerBlock() * sizeof(float));
			
			ExecuteSubBlocks();
		}

#if ENGINE_MINOR_VERSION > 2
		void Reset(const IOperator::FResetParams& InParams)
		{
			TriggerOnDone->Reset();
			
			for (const FAudioBufferWriteRef& BufferRef : OutputAudioBuffers)
			{
				BufferRef->Zero();
			}

			ImpactExternalForceSynth.Reset();
			
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
			ImpactExternalForceSynth.Reset();
			
			const FImpactModalObjAssetProxyPtr& ImpactModalProxy = ModalParams->GetProxy();
			if(ImpactModalProxy.IsValid())
			{
				ImpactExternalForceSynth = MakeUnique<FImpactExternalForceSynth>(ImpactModalProxy, SamplingRate, NumOutputChannels,
																		         *NumModals, *AmplitudeScale, *DecayScale,
																		GetPitchScaleClamped(*PitchShift));
			}
			
			bIsPlaying = ImpactExternalForceSynth.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;
			
			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate <= 0)
			{
				UE_LOG(LogImpactSFXSynth, Warning, TEXT("ImpactExternalForceSynthNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
				return;
			}
			
			FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);
			
			bool bIsStop = true;
			if(InForceBufferViews.Num() > 0 && ImpactExternalForceSynth.IsValid())
			{
				bIsStop = ImpactExternalForceSynth->Synthesize(BufferToGenerate, InForceBufferViews,
														ModalParams->GetProxy(),
														*AmplitudeScale, *DecayScale, GetPitchScaleClamped(*PitchShift),
														 *bIsAutoStop, bClamp);
			}
			
			if(bIsStop)
			{
				ImpactExternalForceSynth.Reset();
				bIsPlaying = false;
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		bool bClamp;
		TArray<FAudioBufferReadRef> InForceBuffers;
		TArray<TArrayView<const float>> InForceBufferViews;
		TArray<FName> InForceBufferVertexNames;
		FBoolReadRef bIsAutoStop;

		FImpactModalObjReadRef ModalParams;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;
		
		FTriggerWriteRef TriggerOnDone;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;

		TUniquePtr<FImpactExternalForceSynth> ImpactExternalForceSynth;
		Audio::FMultichannelBufferView OutputAudioView;
				
		float SamplingRate;
		int32 NumOutputChannels;
		bool bIsPlaying = false;
	};

	class FImpactExternalForceSynthOperatorFactory : public IOperatorFactory
	{
	public:
		FImpactExternalForceSynthOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace ImpactExternalForceSynthVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			TArray<FAudioBufferReadRef> InForces;
			
			
			TDataReadReference<FAudioBuffer> LeftForce = Inputs.GetOrCreateDefaultDataReadReference<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputForce), InParams.OperatorSettings);
			InForces.Emplace(LeftForce);

			TArray<FInputDataVertex> InputForceVertices;
			InputForceVertices.Emplace(Inputs.GetVertex(METASOUND_GET_PARAM_NAME(InputForce)));
			
			if(OutputAudioVertices.Num() > 1)
			{
				InForces.Emplace(Inputs.GetOrCreateDefaultDataReadReference<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputForceR), InParams.OperatorSettings));
				InputForceVertices.Emplace(Inputs.GetVertex(METASOUND_GET_PARAM_NAME(InputForceR)));
			}
			
			FImpactExternalForceSynthOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultValue<bool>(METASOUND_GET_PARAM_NAME(InputClamp), InParams.OperatorSettings),
				
				InForces,
				InputForceVertices,
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsForceStop), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShift), InParams.OperatorSettings)
			};

			return MakeUnique<FImpactExternalForceSynthOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};

	
	template<typename AudioChannelConfigurationInfoType>
	class TImpactExternalForceSynthNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace ImpactExternalForceSynthVertexNames;

			TArray<FOutputDataVertex> OutputDataVertexes =  AudioChannelConfigurationInfoType::GetAudioOutputs();

			FInputVertexInterface InInterface = FInputVertexInterface(
												TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
												TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)));

			InInterface.Add(TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputForce)));
			if(OutputDataVertexes.Num() > 1)
				InInterface.Add(TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputForceR)));
				
			InInterface.Add(TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsForceStop), false));
			InInterface.Add(TInputConstructorVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputClamp), true));
			
			InInterface.Add(TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)));
			InInterface.Add(TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1));
			InInterface.Add(TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmplitudeScale), 1.f));
			InInterface.Add(TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScale), 1.f));
			InInterface.Add(TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchShift), 0.f));
			
			FVertexInterface VertexInterface(InInterface,
				FOutputVertexInterface(
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnPlay)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone))
				)
			);
			
			// Add audio outputs dependent upon source info.
			for (const FOutputDataVertex& OutputDataVertex : OutputDataVertexes)
			{
				VertexInterface.GetOutputInterface().Add(OutputDataVertex);
			}

			return VertexInterface;
		}

		static const FNodeClassMetadata& GetNodeInfo()
		{
			auto InitNodeInfo = []() -> FNodeClassMetadata
			{
				FNodeClassMetadata Info;
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact External Force Synth"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_ImpactExternalForceSynthNodeDescription", "Impact External Force Synthesizer.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("ImpactExternalForceSyntKeyword", "Synthesis") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TImpactExternalForceSynthNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FImpactExternalForceSynthOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TImpactExternalForceSynthNode(const FNodeInitData& InInitData)
			: TImpactExternalForceSynthNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TImpactExternalForceSynthNode() = default;

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

	struct FImpactExternalForceSynthMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ImpactExternalForceSynthMonoNodeDisplayName", "Impact External Force Synth (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ImpactExternalForceSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoImpactExternalForceSynthNode = TImpactExternalForceSynthNode<FImpactExternalForceSynthMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoImpactExternalForceSynthNode);

	struct FImpactExternalForceSynthStereoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ImpactExternalForceSynthStereoNodeDisplayName", "Impact External Force Synth (Stereo)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::StereoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ImpactExternalForceSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			};
		}
	};
	using FStereoImpactExternalForceSynthNode = TImpactExternalForceSynthNode<FImpactExternalForceSynthStereoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FStereoImpactExternalForceSynthNode);

}

#undef LOCTEXT_NAMESPACE