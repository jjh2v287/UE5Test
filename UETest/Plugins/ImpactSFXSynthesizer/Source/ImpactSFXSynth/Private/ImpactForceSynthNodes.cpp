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
#include "MetasoundTime.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "ImpactForceSynth.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "ImpactSFXSynth/Public/Utils.h"

DECLARE_CYCLE_STAT(TEXT("ImpactForceSynth - Total Synth Time"), STAT_ImpactForceSynth, STATGROUP_ImpactSFXSynth);

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactForceSynthNodes"

namespace Metasound
{
	enum class EImpactForceMode : int32
	{
		Cosine = 0,
		Exponent
	};

	DECLARE_METASOUND_ENUM(EImpactForceMode, EImpactForceMode::Cosine, IMPACTSFXSYNTH_API,
						   FImpactForceMode, FImpactForceModeTypeInfo, FImpactForceModeReadRef, FImpactForceModeWriteRef);
	
	DEFINE_METASOUND_ENUM_BEGIN(EImpactForceMode, Metasound::FImpactForceMode, "ImpactForceMode")
		DEFINE_METASOUND_ENUM_ENTRY(EImpactForceMode::Cosine, "CosineDescription", "Cosine", "CosineDescriptionTT", "Use cosine envelopes. Attack time and decay time of each impact are equal to its duration divided by 2."),
		 DEFINE_METASOUND_ENUM_ENTRY(EImpactForceMode::Exponent, "ExponentDescription", "Exponent", "ExponentDescriptionTT", "Use a simple exponential decay envelope. Attack time is zero. Decay time is the same as impact duration."),
	DEFINE_METASOUND_ENUM_END()
}

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactForceSynthVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "stop synthesizing.")
		METASOUND_PARAM(InputTriggerForce, "Force", "trigger impact force start.")

		METASOUND_PARAM(InputForceMode, "Force Mode", "Use Cosine mode when you need impacts with both attack and decay slopes. Exponent mode only has decay slopes.")
		METASOUND_PARAM(InputSeed, "Seed", "Radomizer seed. If <= -1, use random seed.")
		METASOUND_PARAM(InputClamp, "Clamp", "Clamp output to [-1, 1] range.")
		METASOUND_PARAM(InputIsAutoStop, "Auto Stop", "Automatically stop when there is no new impact to be spawned and all signals decay to zero.")
		METASOUND_PARAM(InputSpawnRate, "Spawn Rate", "Number of impacts per second. An impact will always be spawned at the start, regardless of the spawn rate and spawn chance.")
		METASOUND_PARAM(InputSpawnChance, "Spawn Chance", "The spawning chance at each spawning interval.")
		METASOUND_PARAM(InputSpawnChanceDecay, "Spawn Chance Decay", "The decay rate of spawning chance over time.")
		METASOUND_PARAM(InputSpawnDuration, "Spawn Duration", "The duration to continuously spawning new impacts. If < 0, infinite duration.")

		METASOUND_PARAM(InputImpactStrengthMin, "Impact Strength Min", "The minimum strength of impacts.")
		METASOUND_PARAM(InputImpactStrengthRange, "Impact Strength Range", "The random range of impact strength. Impact Strength = Rand(Impact Strength Min, Impact Strength Min + Range).")
		METASOUND_PARAM(InputImpactDuration, "Impact Duration", "The standard duration of an impact at strength = 1.0.")
		METASOUND_PARAM(InputImpactStrengthDurationFactor, "Impact Strength Duration Factor", "Only used in Cosine mode. Determine how the strength of each impact will affect their own duration. Can be negative or positive. If positive, higher impact strength will have LOWER duration.")
		
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params.")
		METASOUND_PARAM(InputAmplitudeScale, "Amplitude Scale", "Scale amplitude of the synthesized SFX.")
		METASOUND_PARAM(InputDecayScale, "Decay Scale", "Scale decay rate of the synthesized SFX.")
		METASOUND_PARAM(InputPitchShift, "Pitch Shift", "Shift pitches by semitone. ")
		METASOUND_PARAM(InputHardForceReset, "Hard Force Reset", "When Impact Duration is longer than Impact Spawn Rate, the force of the last impact might not reach zero yet. If this is true, the remaining force from the last impact is discarded. Otherwise, it will be included with the current force.")
		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the SFX energy decays to zero or reach the specified duration.")
		
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
	}

	struct FImpactForceSynthOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		FTriggerReadRef ForceTrigger;

		FImpactForceMode ForceMode;
		int32 Seed;
		bool bClamp;
		FBoolReadRef bIsAutoStop;
		FFloatReadRef ImpactSpawnRate;
		FFloatReadRef ImpactSpawnChance;
		FFloatReadRef ImpactChanceDecayRate;
		FTimeReadRef ImpactSpawnDuration;
		
		FFloatReadRef ImpactStrengthMin;
		FFloatReadRef ImpactStrengthRange;
		FFloatReadRef ImpactDuration;
		FFloatReadRef ImpactStrengthDurationFactor;
		
		FImpactModalObjReadRef ModalParams;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;
		bool bHardForceReset;
	};
	
	class FImpactForceSynthOperator : public TExecutableOperator<FImpactForceSynthOperator>
	{
	public:

		FImpactForceSynthOperator(const FImpactForceSynthOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, ForceTrigger(InArgs.ForceTrigger)
			, ForceMode(InArgs.ForceMode)
			, Seed(InArgs.Seed)
			, bClamp(InArgs.bClamp)
			, bIsAutoStop(InArgs.bIsAutoStop)
			, ImpactSpawnRate(InArgs.ImpactSpawnRate)
			, ImpactSpawnChance(InArgs.ImpactSpawnChance)
			, ImpactChanceDecayRate(InArgs.ImpactChanceDecayRate)
			, ImpactSpawnDuration(InArgs.ImpactSpawnDuration)
			, ImpactStrengthMin(InArgs.ImpactStrengthMin)
			, ImpactStrengthRange(InArgs.ImpactStrengthRange)
			, ImpactDuration(InArgs.ImpactDuration)
			, ImpactStrengthDurationFactor(InArgs.ImpactStrengthDurationFactor)
			, ModalParams(InArgs.ModalParams)
			, NumModals(InArgs.NumModals)
			, AmplitudeScale(InArgs.AmplitudeScale)
			, DecayScale(InArgs.DecayScale)
			, PitchShift(InArgs.PitchShift)
			, bHardForceReset(InArgs.bHardForceReset)
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
			using namespace ImpactForceSynthVertexNames;
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerForce), ForceTrigger);
			
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputForceMode), ForceMode);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputClamp), bClamp);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsAutoStop), bIsAutoStop);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnRate), ImpactSpawnRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnChance), ImpactSpawnChance);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnChanceDecay), ImpactChanceDecayRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnDuration), ImpactSpawnDuration);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), ImpactStrengthMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), ImpactStrengthRange);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactDuration), ImpactDuration);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthDurationFactor), ImpactStrengthDurationFactor);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputHardForceReset), bHardForceReset);
			
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactForceSynthVertexNames;
			
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
			using namespace ImpactForceSynthVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerForce), ForceTrigger);
			
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputForceMode), ForceMode);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputClamp), bClamp);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsAutoStop), bIsAutoStop);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnRate), ImpactSpawnRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnChance), ImpactSpawnChance);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnChanceDecay), ImpactChanceDecayRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputSpawnDuration), ImpactSpawnDuration);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), ImpactStrengthMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), ImpactStrengthRange);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactDuration), ImpactDuration);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthDurationFactor), ImpactStrengthDurationFactor);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputHardForceReset), bHardForceReset);
			
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
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::ImpactForceSynthVertexNames::Execute);

			SCOPE_CYCLE_COUNTER(STAT_ImpactForceSynth);
			
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

			ImpactForceSynth.Reset();
			
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
			ImpactForceSynth.Reset();
			
			const FImpactModalObjAssetProxyPtr& ImpactModalProxy = ModalParams->GetProxy();
			FImpactForceSpawnParams SpawnParams = FImpactForceSpawnParams(*ImpactSpawnRate, *ImpactSpawnChance, *ImpactChanceDecayRate,
																		  ImpactSpawnDuration->GetSeconds(), *ImpactStrengthMin, *ImpactStrengthRange,
																		  *ImpactDuration, *ImpactStrengthDurationFactor);
			if(ImpactModalProxy.IsValid())
			{
				switch (ForceMode)
				{
					case EImpactForceMode::Exponent:
						ImpactForceSynth = MakeUnique<FImpactExponentForceSynth>(ImpactModalProxy, SamplingRate, NumOutputChannels,
																		 SpawnParams, *NumModals, *AmplitudeScale, *DecayScale,
																		 GetPitchScaleClamped(*PitchShift), Seed, bHardForceReset);
						break;

					default:
						ImpactForceSynth = MakeUnique<FImpactForceSynth>(ImpactModalProxy, SamplingRate, NumOutputChannels,
																		 SpawnParams, *NumModals, *AmplitudeScale, *DecayScale,
																		 GetPitchScaleClamped(*PitchShift), Seed, bHardForceReset);
						break;
				}				
			}
			
			bIsPlaying = ImpactForceSynth.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;
			
			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate <= 0)
			{
				UE_LOG(LogImpactSFXSynth, Warning, TEXT("ImpactForceSynthNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
				return;
			}
			
			FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);
			
			bool bIsStop = true;
			FImpactForceSpawnParams SpawnParams = FImpactForceSpawnParams(*ImpactSpawnRate, *ImpactSpawnChance, *ImpactChanceDecayRate,
																		  ImpactSpawnDuration->GetSeconds(), *ImpactStrengthMin, *ImpactStrengthRange,
																		  *ImpactDuration, *ImpactStrengthDurationFactor);
			if(ImpactForceSynth.IsValid())
			{
				bIsStop = ImpactForceSynth->Synthesize(BufferToGenerate, SpawnParams,
														ModalParams->GetProxy(),
														*AmplitudeScale, *DecayScale, GetPitchScaleClamped(*PitchShift),
														 *bIsAutoStop, bClamp, ForceTrigger->IsTriggered());
			}
			
			if(bIsStop)
			{
				ImpactForceSynth.Reset();
				bIsPlaying = false;
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		FTriggerReadRef ForceTrigger;
		
		FImpactForceMode ForceMode;
		int32 Seed;
		bool bClamp;
		FBoolReadRef bIsAutoStop;
		FFloatReadRef ImpactSpawnRate;
		FFloatReadRef ImpactSpawnChance;
		FFloatReadRef ImpactChanceDecayRate;
		FTimeReadRef ImpactSpawnDuration;
		
		FFloatReadRef ImpactStrengthMin;
		FFloatReadRef ImpactStrengthRange;
		FFloatReadRef ImpactDuration;
		FFloatReadRef ImpactStrengthDurationFactor;
		
		FImpactModalObjReadRef ModalParams;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;
		bool bHardForceReset;
		
		FTriggerWriteRef TriggerOnDone;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;

		TUniquePtr<FImpactForceSynth> ImpactForceSynth;
		Audio::FMultichannelBufferView OutputAudioView;
				
		float SamplingRate;
		int32 NumOutputChannels;
		bool bIsPlaying = false;
	};

	class FImpactForceSynthOperatorFactory : public IOperatorFactory
	{
	public:
		FImpactForceSynthOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace ImpactForceSynthVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FImpactForceSynthOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerForce), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultValue<FImpactForceMode>(METASOUND_GET_PARAM_NAME(InputForceMode), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputSeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<bool>(METASOUND_GET_PARAM_NAME(InputClamp), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsAutoStop), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputSpawnRate), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputSpawnChance), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputSpawnChanceDecay), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputSpawnDuration), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactDuration), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthDurationFactor), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShift), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<bool>(METASOUND_GET_PARAM_NAME(InputHardForceReset), InParams.OperatorSettings),
			};

			return MakeUnique<FImpactForceSynthOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};

	
	template<typename AudioChannelConfigurationInfoType>
	class TImpactForceSynthNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace ImpactForceSynthVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerForce)),
					
					TInputConstructorVertex<FImpactForceMode>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputForceMode), static_cast<int32>(EImpactForceMode::Cosine)),
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSeed), -1),
					TInputConstructorVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputClamp), true),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsAutoStop), true),
					
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSpawnRate), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSpawnChance), 0.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSpawnChanceDecay), 0.0f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSpawnDuration), -1.f),

					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthMin), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthRange), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactDuration), 0.01f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthDurationFactor), 0.25f),
					
					TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmplitudeScale), 1.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScale), 1.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchShift), 0.f),
					TInputConstructorVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputHardForceReset), false)
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
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact Force Synth"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_ImpactForceSynthNodeDescription", "Impact Force Synthesizer.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("ImpactForceSyntKeyword", "Synthesis") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TImpactForceSynthNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FImpactForceSynthOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TImpactForceSynthNode(const FNodeInitData& InInitData)
			: TImpactForceSynthNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TImpactForceSynthNode() = default;

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

	struct FImpactForceSynthMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ImpactForceSynthMonoNodeDisplayName", "Impact Force Synth (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ImpactForceSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoImpactForceSynthNode = TImpactForceSynthNode<FImpactForceSynthMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoImpactForceSynthNode);

	struct FImpactForceSynthStereoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ImpactForceSynthStereoNodeDisplayName", "Impact Force Synth (Stereo)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::StereoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ImpactForceSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			};
		}
	};
	using FStereoImpactForceSynthNode = TImpactForceSynthNode<FImpactForceSynthStereoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FStereoImpactForceSynthNode);

}

#undef LOCTEXT_NAMESPACE