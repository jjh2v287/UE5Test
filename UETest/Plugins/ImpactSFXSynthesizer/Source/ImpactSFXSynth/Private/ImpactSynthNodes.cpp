// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "MetasoundResidualData.h"
#include "CustomStatGroup.h"
#include "ImpactSFXSynthLog.h"
#include "ImpactSynthEngineNodesName.h"
#include "DSP/Dsp.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "MetasoundImpactModalObj.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTime.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "SynthParamPresets.h"
#include "ModalSynth.h"
#include "ResidualSynth.h"
#include "DSP/FloatArrayMath.h"
#include "ImpactSFXSynth/Public/Utils.h"

DECLARE_CYCLE_STAT(TEXT("Impact SFX - Total Synth Time"), STAT_ImpactSynth, STATGROUP_ImpactSFXSynth);

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactSynthNodes"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ImpactSynthVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "Stop synthesizing.")

		METASOUND_PARAM(InputImpactStrengthScale, "Impact Strength Scale", "Min = 0.1. Max = 10.")
		METASOUND_PARAM(InputDampingRatio, "Damping Ratio", "Range [0, 1]. Automatically scale the decay rate and residual play speed by combining this ratio with \"Impact Strength Scale\". Solid/sitff objects should have this at 1.0. While hollow/resonance objects should be at 0. Leaving this at 0.5 is also fine for most applications.")
		
		METASOUND_PARAM(InputEnableModalSynth, "Enable Modal Synthesizer", "Enable modal synthesizer or not.")
		METASOUND_PARAM(InputModalParams, "Modal Params", "Modal parameters to be used.")
		METASOUND_PARAM(InputModalStartTime, "Modal Start Time", "The starting time of the synthesized audio. If negative, amplitude is synthesized in reverse. If the negative value and decay are too large, floating point truncate errors will appear.")
		METASOUND_PARAM(InputModalDuration, "Modal Duration", "If duration <= 0, stop when the amplitude of all modals decay to zero.")
		METASOUND_PARAM(InputNumModal, "Number of Modals", "<= 0 means using all modal params. Higher values give more realistic results but increase computational costs (linearly).")
		METASOUND_PARAM(InputAmplitudeScale, "Modal Amplitude Scale", "Scale amplitude of the synthesized impact SFX.")
		METASOUND_PARAM(InputDecayScale, "Modal Decay Scale", "Scale decay rate of the synthesized impact SFX. Lower value increases resonant time (to simulate hollow/thin objects, etc.).")
		METASOUND_PARAM(InputPitchShift, "Modal Pitch Shift", "Shift pitches by semitone.")

		METASOUND_PARAM(InputEnableResidualSynth, "Enable Residual Synthesizer", "Enable residual synthesizer or not.")
		METASOUND_PARAM(InputResidualData, "Residual Data", "Residual preset data to use.")

		METASOUND_PARAM(InputResidualAmplitudeScale, "Residual Amplitude Scale", "Residual amplitude scale.")
		METASOUND_PARAM(InputResidualIsUsePreviewValue, "Use Preview Values", "Use the preview values in Residual Data Editor for all properties below this pin.")
		METASOUND_PARAM(InputResidualPitchShift, "Residual Pitch Shift", "Shift pitches by semitone.")
		METASOUND_PARAM(InputResidualPlaySpeed, "Residual Play Speed", "Residual play back rate.")
		METASOUND_PARAM(InputResidualStartTime, "Residual Start time", "Start synthesize at time in seconds.")
		METASOUND_PARAM(InputResidualDuration, "Residual Duration", "Duration in seconds. <= 0 means residual synthesizer will stop when it has synthesized the final frame.")
		METASOUND_PARAM(InputResidualSeed, "Residual Seed", "< 0 means using random seed.")
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the impact SFX energy decays to zero or reach the specified duration.")

		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
	}

	struct FImpactSynthOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;

		FFloatReadRef ImpactStrengthScale;
		FFloatReadRef DampingRatio;
		
		FBoolReadRef EnableModalSynth;
		FImpactModalObjReadRef ModalParams;
		FTimeReadRef  ModalStartTime;
		FTimeReadRef  ModalDuration;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;

		FBoolReadRef EnableResidualSynth;
		FResidualDataReadRef ResidualData;
		FFloatReadRef ResidualAmplitudeScale;
		FBoolReadRef bUsePreviewValues;
		FFloatReadRef ResidualPitchShift;
		FFloatReadRef ResidualPlaySpeed;
		FTimeReadRef ResidualStartTime;
		FTimeReadRef ResidualDuration;
		FInt32ReadRef ResidualSeed;
	};
	
	class FImpactSynthOperator : public TExecutableOperator<FImpactSynthOperator>
	{
	public:

		FImpactSynthOperator(const FImpactSynthOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, ImpactStrengthScale(InArgs.ImpactStrengthScale)
			, DampingRatio(InArgs.DampingRatio)
			, EnableModalSynth(InArgs.EnableModalSynth)
			, ModalParams(InArgs.ModalParams)
			, ModalStartTime(InArgs.ModalStartTime)
			, ModalDuration(InArgs.ModalDuration)
			, NumModals(InArgs.NumModals)
			, AmplitudeScale(InArgs.AmplitudeScale)
			, DecayScale(InArgs.DecayScale)
			, PitchShift(InArgs.PitchShift)
			, EnableResidualSynth(InArgs.EnableResidualSynth)
			, ResidualData(InArgs.ResidualData)
			, ResidualAmplitudeScale(InArgs.ResidualAmplitudeScale)
			, bUsePreviewValues(InArgs.bUsePreviewValues)
			, ResidualPitchShift(InArgs.ResidualPitchShift)
			, ResidualPlaySpeed(InArgs.ResidualPlaySpeed)
			, ResidualStartTime(InArgs.ResidualStartTime)
			, ResidualDuration(InArgs.ResidualDuration)
			, ResidualSeed(InArgs.ResidualSeed)
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
			using namespace ImpactSynthVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthScale), ImpactStrengthScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDampingRatio), DampingRatio);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputEnableModalSynth), EnableModalSynth);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalStartTime), ModalStartTime);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalDuration), ModalDuration);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputEnableResidualSynth), EnableResidualSynth);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualData), ResidualData);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualAmplitudeScale), ResidualAmplitudeScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualIsUsePreviewValue), bUsePreviewValues);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPlaySpeed), ResidualPlaySpeed);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualStartTime), ResidualStartTime);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualDuration), ResidualDuration);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSeed), ResidualSeed);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ImpactSynthVertexNames;
			
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
			using namespace ImpactSynthVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthScale), ImpactStrengthScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDampingRatio), DampingRatio);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputEnableModalSynth), EnableModalSynth);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalStartTime), ModalStartTime);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalDuration), ModalDuration);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModal), NumModals);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), AmplitudeScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), DecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPitchShift), PitchShift);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputEnableResidualSynth), EnableResidualSynth);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualData), ResidualData);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualAmplitudeScale), ResidualAmplitudeScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualIsUsePreviewValue), bUsePreviewValues);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPlaySpeed), ResidualPlaySpeed);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualStartTime), ResidualStartTime);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualDuration), ResidualDuration);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSeed), ResidualSeed);

			
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
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::FImpactSynthOperator::Execute);

			SCOPE_CYCLE_COUNTER(STAT_ImpactSynth);
			
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

			ModalSynth.Reset();
			ResidualSynth.Reset();
			
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
			ModalSynth.Reset();
			ResidualSynth.Reset();
			const FImpactModalObjAssetProxyPtr& ImpactModalProxy = ModalParams->GetProxy();
			const float StrengthScale = GetImpactStrengthScaleClamped();
			if(*EnableModalSynth && ImpactModalProxy.IsValid())
			{
				ModalSynth = MakeUnique<FModalSynth>(SamplingRate, 
													ImpactModalProxy, ModalStartTime->GetSeconds(), ModalDuration->GetSeconds(),
													*NumModals, GetAmplitudeScaleClamped(), GetDecayScaleClamped(), GetPitchScaleClamped(*PitchShift),
													StrengthScale, GetDampingRatioClamped(*DampingRatio));
				
			}

			const FResidualDataAssetProxyPtr& ResidualDataProxy = (*ResidualData).GetProxy();
			if(*EnableResidualSynth && ResidualDataProxy.IsValid())
			{
				float PlaySpeed;
				int32 Seed;
				float StartTime;
				float Duration;
				float PitchScale;
				if(*bUsePreviewValues)
				{
					PlaySpeed = ResidualDataProxy->GetPreviewPlaySpeed();
					Seed = ResidualDataProxy->GetPreviewSeed();
					StartTime = ResidualDataProxy->GetPreviewStartTime();
					Duration = ResidualDataProxy->GetPreviewDuration();
					PitchScale = GetPitchScaleClamped(ResidualDataProxy->GetPreviewPitchShift());
				}
				else
				{
					PlaySpeed = *ResidualPlaySpeed;
					StartTime = (*ResidualStartTime).GetSeconds();
					Duration =  ResidualDuration->GetSeconds();
					Seed = *ResidualSeed;
					PitchScale = GetPitchScaleClamped(*ResidualPitchShift);
				}
				
				ResidualSynth = MakeUnique<FResidualSynth>(SamplingRate, OperatorSettings.GetNumFramesPerBlock(), OutputAudioView.Num(),
														   ResidualDataProxy.ToSharedRef(), PlaySpeed, *ResidualAmplitudeScale, PitchScale,
														   Seed, StartTime, Duration,
														   false, StrengthScale);
			}

			bIsPlaying = ModalSynth.IsValid() || ResidualSynth.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;
			
			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate <= 0)
			{
				UE_LOG(LogImpactSFXSynth, Warning, TEXT("ImpactSynthNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
				return;
			}
			
			FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);
			
			const bool bIsEnableModalSynth = *EnableModalSynth && ModalSynth.IsValid();
			const bool bIsEnableResidualSynth = *EnableResidualSynth && ResidualSynth.IsValid();
			bool bIsModalStop = !bIsEnableModalSynth;
			if(bIsEnableModalSynth)
			{
				bIsModalStop = ModalSynth->Synthesize(BufferToGenerate, ModalParams->GetProxy(), !bIsEnableResidualSynth);
				
				if(bIsModalStop)
					ModalSynth.Reset();
			}

			bool bIsResidualStop = true;
			if(bIsEnableResidualSynth)
			{
				bIsResidualStop = ResidualSynth->Synthesize(BufferToGenerate, bIsEnableModalSynth, true);
				if(bIsResidualStop)
					ResidualSynth.Reset();
			}
			
			if(bIsModalStop && bIsResidualStop)
			{
				bIsPlaying = false;
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}

		float GetImpactStrengthScaleClamped() const
		{
			return FMath::Clamp(*ImpactStrengthScale, 0.1f, 10.f);
		}
		
		float GetAmplitudeScaleClamped() const
		{
			return FMath::Clamp(*AmplitudeScale, MinClamp, MaxClamp);
		}

		float GetDecayScaleClamped() const
		{
			return FMath::Clamp(*DecayScale, MinClamp, MaxClamp);
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;

		FFloatReadRef ImpactStrengthScale;
		FFloatReadRef DampingRatio;
		
		FBoolReadRef EnableModalSynth;
		FImpactModalObjReadRef ModalParams;
		FTimeReadRef ModalStartTime;
		FTimeReadRef ModalDuration;
		FInt32ReadRef NumModals;
		FFloatReadRef AmplitudeScale;
		FFloatReadRef DecayScale;
		FFloatReadRef PitchShift;

		FBoolReadRef EnableResidualSynth;
		FResidualDataReadRef ResidualData;
		FFloatReadRef ResidualAmplitudeScale;
		FBoolReadRef bUsePreviewValues;
		FFloatReadRef ResidualPitchShift;
		FFloatReadRef ResidualPlaySpeed;
		FTimeReadRef ResidualStartTime;
		FTimeReadRef ResidualDuration;
		FInt32ReadRef ResidualSeed;
		
		FTriggerWriteRef TriggerOnDone;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;

		TUniquePtr<FModalSynth> ModalSynth;
		TUniquePtr<FResidualSynth> ResidualSynth;
		Audio::FMultichannelBufferView OutputAudioView;
		
		float SamplingRate;
		int32 NumOutputChannels;
		bool bIsPlaying = false;
	};

	class FImpactSynthOperatorFactory : public IOperatorFactory
	{
	public:
		FImpactSynthOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace ImpactSynthVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FImpactSynthOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDampingRatio), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputEnableModalSynth), InParams.OperatorSettings),
				 Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputModalStartTime), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputModalDuration), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModal), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmplitudeScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPitchShift), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputEnableResidualSynth), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FResidualData>(METASOUND_GET_PARAM_NAME(InputResidualData)),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualAmplitudeScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputResidualIsUsePreviewValue), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualPlaySpeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputResidualStartTime), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputResidualDuration), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputResidualSeed), InParams.OperatorSettings)
			};

			return MakeUnique<FImpactSynthOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};
	
	template<typename AudioChannelConfigurationInfoType>
	class TImpactSynthNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace ImpactSynthVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
					
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthScale), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDampingRatio), 0.5f),
					
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputEnableModalSynth), true),
					TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalStartTime), 0.f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalDuration), -1.f),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModal), -1),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmplitudeScale), 1.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScale), 1.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPitchShift), 0.f),
					
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputEnableResidualSynth), true),
					TInputDataVertex<FResidualData>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualData)),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualAmplitudeScale), 1.f),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualIsUsePreviewValue), false),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualPitchShift), 0.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualPlaySpeed), 1.f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualStartTime), 0.f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualDuration), -1.f),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualSeed), -1)
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
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Impact SFX Synth"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_ImpactSynthNodeDescription", "Synthesize Impact SFX.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("ImpactSFXSyntKeyword", "Synthesis") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TImpactSynthNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FImpactSynthOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TImpactSynthNode(const FNodeInitData& InInitData)
			: TImpactSynthNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TImpactSynthNode() = default;

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

	struct FImpactSynthMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ImpactSynthMonoNodeDisplayName", "Impact SFX Synth (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ImpactSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoImpactSynthNode = TImpactSynthNode<FImpactSynthMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoImpactSynthNode);
	
	struct FImpactSynthStereoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ImpactSynthStereoNodeDisplayName", "Impact SFX Synth (Stereo)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::StereoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ImpactSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			};
		}
	};
	using FStereoImpactSynthNode = TImpactSynthNode<FImpactSynthStereoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FStereoImpactSynthNode);

	
}

#undef LOCTEXT_NAMESPACE