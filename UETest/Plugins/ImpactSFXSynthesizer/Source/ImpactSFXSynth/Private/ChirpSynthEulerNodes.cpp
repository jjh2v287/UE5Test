﻿// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

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
#include "MetasoundTime.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "ChirpSynthEuler.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "ImpactSFXSynth/Public/Utils.h"

DECLARE_CYCLE_STAT(TEXT("ChirpSFXFaslt - Total Synth Time"), STAT_ChirpSynthEuler, STATGROUP_ImpactSFXSynth);

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ChirpSynthEulerNodes"

namespace Metasound
{
	enum class EChirpSynthEulerMode : int32
	{
		Linear = 0,
		Exponent
	};
	
	DECLARE_METASOUND_ENUM(EChirpSynthEulerMode, EChirpSynthEulerMode::Linear, IMPACTSFXSYNTH_API,
						   FChirpSynthEulerMode, FChirpSynthEulerModeTypeInfo, FChirpSynthEulerModeReadRef, FChirpSynthEulerModeWriteRef);
	
	DEFINE_METASOUND_ENUM_BEGIN(EChirpSynthEulerMode, Metasound::FChirpSynthEulerMode, "ChirpSynthEulerMode")
		DEFINE_METASOUND_ENUM_ENTRY(EChirpSynthEulerMode::Linear, "LinearDescription", "Linear", "LinearDescriptionTT", "For UI SFX or simple chirp effect."),
	     DEFINE_METASOUND_ENUM_ENTRY(EChirpSynthEulerMode::Exponent, "ExponentDescription", "Exponent", "ExponentDescriptionTT", "For simulating engine power up/down with smooth frequency change."),
	DEFINE_METASOUND_ENUM_END()
}

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ChirpSynthEulerVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "stop synthesizing.")		

		METASOUND_PARAM(InputChirpMode, "Chirp Mode", " Linear, sigmoid, and exponent sweeps are supported. Linear mode can be used for sci-fi or UI effects. Exponent mode is used to simulate engine power going up or down. Sigmoid mode is similar to exponent but has an overshooting effect.")
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputDuration, "Duration", "If duration <= 0, stop when the amplitude of all signals decay to zero.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params. Higher values give more realistic results but increase computational costs (linearly).")
		METASOUND_PARAM(InputAmplitudeScale, "Amplitude Scale", "Scale amplitude of the synthesized SFX.")
		METASOUND_PARAM(InputDecayScale, "Decay Scale", "Scale decay rate of the synthesized SFX.")
		METASOUND_PARAM(InputPitchShift, "Pitch Shift", "Shift pitches by semitone.")
		METASOUND_PARAM(InputChirpRate, "Chirp Rate", "Frequency changing speed. Can be positive or negative.")
		METASOUND_PARAM(InputRandomRate, "Random Rate", "If > 0, make the synthesized signal more realistic by introducing some randomness into each modal.")
		METASOUND_PARAM(InputAmplitudeRandRange, "Amp Rand Range", "Randomness percentage applied to the amplitude of each modal. [0., 1.f]. This is only used if Random Rate > 0.")
		// METASOUND_PARAM(InputPitchRandRange, "Pitch Rand Range", "Randomn pitch shift applied to each modal. Only used in non linear mode.")
		METASOUND_PARAM(InputRampDuration, "Chirp Duration", "In Linear mode, frequencies will stop changing after this duration. Im non-linear mode, this will be combined with \"Chirp Rate\" to control frequencies changing rate. If you want your SFX for engine power going up or down lasts X seconds, then set this at X and \"Chirp Rate\" at 1.")
		METASOUND_PARAM(InputClamp, "Clamp", "Clamp output to [-1, 1] range .")
		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the SFX energy decays to zero or reach the specified duration.")
		
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
	}

	struct FChirpSynthEulerOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;

		FChirpSynthEulerMode SynthMode;
		FImpactModalObjReadRef ModalParams;
		FTimeReadRef  Duration;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;
		FFloatReadRef ChirpRate;
		FTimeReadRef RandomRate;
		FFloatReadRef AmpRandRange;
		//FFloatReadRef PitchRandRange;
		FTimeReadRef RampDuration;
		FBoolReadRef bClamp;
	};
	
	class FChirpSynthEulerOperator : public TExecutableOperator<FChirpSynthEulerOperator>
	{
	public:

		FChirpSynthEulerOperator(const FChirpSynthEulerOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, SynthMode(InArgs.SynthMode)
			, ModalParams(InArgs.ModalParams)
			, Duration(InArgs.Duration)
			, NumModals(InArgs.NumModals)
			, AmplitudeScale(InArgs.AmplitudeScale)
			, DecayScale(InArgs.DecayScale)
			, PitchShift(InArgs.PitchShift)
			, ChirpRate(InArgs.ChirpRate)
			, RandomRate(InArgs.RandomRate)
			, AmpRandRange(InArgs.AmpRandRange)
			//, PitchRandRange(InArgs.PitchRandRange)
			, RampDuration(InArgs.RampDuration)
			, bClamp(InArgs.bClamp)
			, TriggerOnDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
		{
			NumOutputChannels = InArgs.OutputAudioVertices.Num();
			SamplingRate = OperatorSettings.GetSampleRate();
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
			using namespace ChirpSynthEulerVertexNames;
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);

			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputChirpMode), SynthMode);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDuration), Duration);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputChirpRate), ChirpRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRandomRate), RandomRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeRandRange), AmpRandRange);
			//InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchRandRange), PitchRandRange);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRampDuration), RampDuration);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputClamp), bClamp);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ChirpSynthEulerVertexNames;
			
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
			using namespace ChirpSynthEulerVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);

			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputChirpMode), SynthMode);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDuration), Duration);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputChirpRate), ChirpRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRandomRate), RandomRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeRandRange), AmpRandRange);
			//InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchRandRange), PitchRandRange);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRampDuration), RampDuration);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputClamp), bClamp);

			
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
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::ChirpSynthEulerVertexNames::Execute);

			SCOPE_CYCLE_COUNTER(STAT_ChirpSynthEuler);
			
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

			ChirpSynthEuler.Reset();
			
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
			ChirpSynthEuler.Reset();
			
			const FImpactModalObjAssetProxyPtr& ImpactModalProxy = ModalParams->GetProxy();
			switch (SynthMode)
			{
				case EChirpSynthEulerMode::Linear:
					ChirpSynthEuler = MakeUnique<FChirpSynthEuler>(SamplingRate, ImpactModalProxy,
												 GetSynthParamStruct(), *NumModals, Duration->GetSeconds());
				break;

				case EChirpSynthEulerMode::Exponent:
					ChirpSynthEuler = MakeUnique<FSigmoidChirpSynthEuler>(SamplingRate, ImpactModalProxy,
												     GetSynthParamStruct(), *NumModals, Duration->GetSeconds());
				break;
				
				default:
					UE_LOG(LogImpactSFXSynth, Error, TEXT("Unknown chirp synth mode!"));
				break;
			}
			
			bIsPlaying = ChirpSynthEuler.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;
			
			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate <= 0)
			{
				UE_LOG(LogImpactSFXSynth, Warning, TEXT("ChirpSynthFastNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
				return;
			}
			
			FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);
			
			bool bIsModalStop = true;
			if(ChirpSynthEuler.IsValid())
			{
				const FImpactModalObjAssetProxyPtr& ImpactModalProxy = ModalParams->GetProxy();
				if(ImpactModalProxy.IsValid())
					bIsModalStop = ChirpSynthEuler->Synthesize(BufferToGenerate, ImpactModalProxy, GetSynthParamStruct(), *bClamp);
			}
			
			if(bIsModalStop)
			{
				ChirpSynthEuler.Reset();
				bIsPlaying = false;
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}
		
		FChirpSynthEulerParams GetSynthParamStruct() const
		{
			const float PitchChange = *PitchShift;
			const float PicthScale = PitchChange < - 72.f ? 0.f : GetPitchScaleClamped(*PitchShift);
			return FChirpSynthEulerParams(RampDuration->GetSeconds(), *ChirpRate, *AmplitudeScale,
									   *DecayScale, PicthScale,
									   RandomRate->GetSeconds(), *AmpRandRange, 0.f);
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		FChirpSynthEulerMode SynthMode;
		FImpactModalObjReadRef ModalParams;
		FTimeReadRef  Duration;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;
		FFloatReadRef ChirpRate;
		FTimeReadRef RandomRate;
		FFloatReadRef AmpRandRange;
		//FFloatReadRef PitchRandRange;
		FTimeReadRef RampDuration;
		FBoolReadRef bClamp;
		
		FTriggerWriteRef TriggerOnDone;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;

		TUniquePtr<FChirpSynthEuler> ChirpSynthEuler;
		Audio::FMultichannelBufferView OutputAudioView;
		
		float SamplingRate;
		int32 NumOutputChannels;
		bool bIsPlaying = false;
	};

	class FChirpSynthEulerOperatorFactory : public IOperatorFactory
	{
	public:
		FChirpSynthEulerOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace ChirpSynthEulerVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FChirpSynthEulerOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultValue<FChirpSynthEulerMode>(METASOUND_GET_PARAM_NAME(InputChirpMode), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputDuration), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShift), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputChirpRate), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputRandomRate), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmplitudeRandRange), InParams.OperatorSettings),
				//Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchRandRange), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputRampDuration), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputClamp), InParams.OperatorSettings)
			};

			return MakeUnique<FChirpSynthEulerOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};

	
	template<typename AudioChannelConfigurationInfoType>
	class TChirpSynthEulerNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace ChirpSynthEulerVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),

					TInputConstructorVertex<FChirpSynthEulerMode>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputChirpMode), static_cast<int32>(EChirpSynthEulerMode::Linear)),
					TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDuration), -1.f),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmplitudeScale), 1.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScale), 1.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchShift), 0.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputChirpRate), 1.f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRandomRate), 0.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmplitudeRandRange), 0.f),
					//TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchRandRange), 0.f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRampDuration), -1.f),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputClamp), true)
					),
				FOutputVertexInterface(
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnPlay)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone))
				)
			);
			
			// Add audio outputs dependent upon source info.
			for (const FOutputDataVertex& OutputDataVertex : AudioChannelConfigurationInfoType::GetAudioOutputs())
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
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Chirp Synth Fast"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_ChirpSynthEulerNodeDescription", "Synthesize Chirp SFX - Fast Mode.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("ChirpSyntKeyword", "Synthesis") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TChirpSynthEulerNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FChirpSynthEulerOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TChirpSynthEulerNode(const FNodeInitData& InInitData)
			: TChirpSynthEulerNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TChirpSynthEulerNode() = default;

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

	struct FChirpSynthEulerMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ChirpSynthEulerMonoNodeDisplayName", "Chirp Synth Fast (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ChirpSynthEulerVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoChirpSynthEulerNode = TChirpSynthEulerNode<FChirpSynthEulerMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoChirpSynthEulerNode);

}

#undef LOCTEXT_NAMESPACE