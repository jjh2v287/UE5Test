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
#include "MetasoundResidualData.h"
#include "MetasoundTime.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "ScratchingSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactSynthNodes"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ScratchingSynthVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "Stop synthesizing.")

		METASOUND_PARAM(InputSeed, "Seed", "Radomizer seed. If <= -1, use random seed.")
		METASOUND_PARAM(InputIsClamped, "Clamp", "Clamp output to [-1, 1] or not.")
		
		METASOUND_PARAM(InputImpactSpawnRate, "Spawn Rate", "Range [0, 100]. The number of spawning intervals per second.")
		METASOUND_PARAM(InputImpactSpawnChance, "Spawn Chance", "Range [0, 1]. The chance at which new impacts are spawned.")
		METASOUND_PARAM(InputImpactSpawnDecayRate, "Spawn Chance Decay", "New impacts spawning chance is reduced (or increased if < 0) based on an exoponential function over time.")
		METASOUND_PARAM(InputImpactSpawnDuration, "Spawning Duration", "If < 0 and spawning chance decay rate > 0, new impacts are generated untill spawning chance = 0.")
		
		METASOUND_PARAM(InputImpactStrengthMin, "Impact Strength Min", "High values for rough surfaces. Low value for smooth surfaces.")
		METASOUND_PARAM(InputImpactStrengthRange, "Impact Strength Range", "High values for rough surfaces. Low value for smooth surfaces. Impact Strength = Rand(Impact Strength Min, Impact Strength Min + Range).")
		METASOUND_PARAM(InputImpactStrengthDecay, "Impact Strength Decay Rate", "The strength of a new impact is reduced (or increased if < 0) based on an exoponential function over time.")

		METASOUND_PARAM(InputFrameStartMin, "Frame Start Min", "The lowest starting frame of an impact.")
		METASOUND_PARAM(InputFrameStartMax, "Frame Start Max", "The highest starting frame of an impact.")
		
		METASOUND_PARAM(InputExciteModalData, "Excitation Modal Data", "The modal data for the object which starts the action.")
		METASOUND_PARAM(InputExciteNumModal, "Excitation Num Modals", "The number of excitation modals to be used. If <= 0, use all.")
		METASOUND_PARAM(InputExciteAmpScale, "Excitation Amplitude Scale", "The amplitude scale of the excitation modal data.")
		METASOUND_PARAM(InputExciteDecayScale, "Excitation Decay Scale", "The decay scale of the excitation modal data.")
		METASOUND_PARAM(InputExcitePitchShift, "Excitation Pitch Shift", "The pitch shift of the excitation modal data.")

		METASOUND_PARAM(InputResonanceModalData, "Resonance Modal Data", "The modal data for the object which is hit by the excitation object.")
		METASOUND_PARAM(InputResonanceNumModal, "Resonance Num Modals", "The number of resonance modals to be used. If <= 0, use all.")
		METASOUND_PARAM(InputResonanceAmpScale, "Resonance Amplitude Scale", "The amplitude scale of the resonance modal data.")
		METASOUND_PARAM(InputResonanceDecayScale, "Resonance Decay Scale", "The decay scale of the resonance modal data.")
		METASOUND_PARAM(InputResonancePitchShift, "Resonance Pitch Shift", "The pitch shift of the resonance modal data.")
		
		METASOUND_PARAM(InputResidualData, "Residual Data", "The residual data of the resonance object.")
		METASOUND_PARAM(InputResidualAmpScale, "Residual Amplitude Scale", "The amplitude scale of the residual data.")
		METASOUND_PARAM(InputResidualSpeed, "Residual Play Speed", "The play speed of the residual data.")
		METASOUND_PARAM(InputResidualPitchShift, "Residual Pitch Shift", "The pitch shift of the residual data.")
		METASOUND_PARAM(InputResidualStartTime, "Residual Start Time", "The start time of the residual data.")
		METASOUND_PARAM(InputResidualDuration, "Residual Duration", "The total duration when synthesized. If < 0, run until final frame.")
		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the impact SFX energy decays to zero or reach the specified duration.")

		METASOUND_PARAM(OutputCurrentSeed, "Current Seed", "Currently used seed.")
		
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
	}

	struct FScratchingSynthOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		int32 Seed;
		bool bClamp;
		
		FFloatReadRef ImpactSpawnRate;
		FFloatReadRef ImpactSpawnChance;
		FFloatReadRef ImpactChanceDecayRate;
		FTimeReadRef ImpactSpawnDuration;
		
		FFloatReadRef ImpactStrengthMin;
		FFloatReadRef ImpactStrengthRange;
		FFloatReadRef ImpactStrengthDecay;
		FInt32ReadRef FrameStartMin;
		FInt32ReadRef FrameStartMax;

		FImpactModalObjReadRef ExciteModalObjReadRef;
		FInt32ReadRef ExciteNumModal;
		FFloatReadRef ExciteAmplScale;
		FFloatReadRef ExciteDecayScale;
		FFloatReadRef ExcitePitchShift;

		FImpactModalObjReadRef ResonanceModalObjReadRef;
		FInt32ReadRef ResonanceNumModal;
		FFloatReadRef ResonanceAmplScale;
		FFloatReadRef ResonanceDecayScale;
		FFloatReadRef ResonancePitchShift;

		FResidualDataReadRef ResidualDataReadRef;
		FFloatReadRef ResidualAmpScale;
		FFloatReadRef ResidualSpeedScale;
		FFloatReadRef ResidualPitchShift;
		FTimeReadRef ResidualStartTime;
		FTimeReadRef ResidualDuration;
	};
	
	class FScratchingSynthOperator : public TExecutableOperator<FScratchingSynthOperator>
	{
	public:

		FScratchingSynthOperator(const FScratchingSynthOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, Seed(InArgs.Seed)
			, bClamp(InArgs.bClamp)
			, ImpactSpawnRate(InArgs.ImpactSpawnRate)
			, ImpactSpawnChance(InArgs.ImpactSpawnChance)
			, ImpactChanceDecayRate(InArgs.ImpactChanceDecayRate)
			, ImpactSpawnDuration(InArgs.ImpactSpawnDuration)
			, ImpactStrengthMin(InArgs.ImpactStrengthMin)
			, ImpactStrengthRange(InArgs.ImpactStrengthRange)
			, ImpactStrengthDecay(InArgs.ImpactStrengthDecay)
			, FrameStartMin(InArgs.FrameStartMin)
			, FrameStartMax(InArgs.FrameStartMax)
			, ExciteModalObjReadRef(InArgs.ExciteModalObjReadRef)
			, ExciteNumModal(InArgs.ExciteNumModal)
			, ExciteAmplScale(InArgs.ExciteAmplScale)
			, ExciteDecayScale(InArgs.ExciteDecayScale)
			, ExcitePitchShift(InArgs.ExcitePitchShift)
			, ResonanceModalObjReadRef(InArgs.ResonanceModalObjReadRef)
			, ResonanceNumModal(InArgs.ResonanceNumModal)
			, ResonanceAmplScale(InArgs.ResonanceAmplScale)
			, ResonanceDecayScale(InArgs.ResonanceDecayScale)
			, ResonancePitchShift(InArgs.ResonancePitchShift)
			, ResidualDataReadRef(InArgs.ResidualDataReadRef)
			, ResidualAmpScale(InArgs.ResidualAmpScale)
			, ResidualSpeedScale(InArgs.ResidualSpeedScale)
			, ResidualPitchShift(InArgs.ResidualPitchShift)
			, ResidualStartTime(InArgs.ResidualStartTime)
			, ResidualDuration(InArgs.ResidualDuration)
			, TriggerOnDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
			, OutSeed(FInt32WriteRef::CreateNew(-1))
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
			using namespace ScratchingSynthVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputIsClamped), bClamp);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnRate), ImpactSpawnRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnChance), ImpactSpawnChance);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDecayRate), ImpactChanceDecayRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDuration), ImpactSpawnDuration);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), ImpactStrengthMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), ImpactStrengthRange);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthDecay), ImpactStrengthDecay);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFrameStartMin), FrameStartMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFrameStartMax), FrameStartMax);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteModalData), ExciteModalObjReadRef);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteNumModal), ExciteNumModal);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteAmpScale), ExciteAmplScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteDecayScale), ExciteDecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExcitePitchShift), ExcitePitchShift);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceModalData), ResonanceModalObjReadRef);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceNumModal), ResonanceNumModal);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceAmpScale), ResonanceAmplScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceDecayScale), ResonanceDecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonancePitchShift), ResonancePitchShift);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualData), ResidualDataReadRef);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualAmpScale), ResidualAmpScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSpeed), ResidualSpeedScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualStartTime), ResidualStartTime);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualDuration), ResidualDuration);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace ScratchingSynthVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputCurrentSeed), OutSeed);
			
			check(OutputAudioBuffers.Num() == OutputAudioBufferVertexNames.Num());

			for (int32 i = 0; i < OutputAudioBuffers.Num(); i++)
			{
				InOutVertexData.BindReadVertex(OutputAudioBufferVertexNames[i], OutputAudioBuffers[i]);
			}
		}
#else

		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace ScratchingSynthVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputIsClamped), bClamp);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnRate), ImpactSpawnRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnChance), ImpactSpawnChance);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDecayRate), ImpactChanceDecayRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDuration), ImpactSpawnDuration);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), ImpactStrengthMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), ImpactStrengthRange);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthDecay), ImpactStrengthDecay);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFrameStartMin), FrameStartMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFrameStartMax), FrameStartMax);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteModalData), ExciteModalObjReadRef);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteNumModal), ExciteNumModal);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteAmpScale), ExciteAmplScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExciteDecayScale), ExciteDecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputExcitePitchShift), ExcitePitchShift);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceModalData), ResonanceModalObjReadRef);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceNumModal), ResonanceNumModal);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceAmpScale), ResonanceAmplScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonanceDecayScale), ResonanceDecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResonancePitchShift), ResonancePitchShift);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualData), ResidualDataReadRef);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualAmpScale), ResidualAmpScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSpeed), ResidualSpeedScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualStartTime), ResidualStartTime);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualDuration), ResidualDuration);

			
			FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputCurrentSeed), OutSeed);
			
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

			ScratchingSynth.Reset();
			
			bIsPlaying = false;
		}
#endif
		
	private:
		static constexpr float MaxClamp = 1.e3f;
		static constexpr float MinClamp = 1.e-3f;
		static constexpr float DampingRatio = 1.f;
		
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
					StartPlaying();
					++PlayTrigIndex;
				}

				if (CurrAudioFrame == NextStopFrame)
				{
					StopPlaying(CurrAudioFrame);
					++StopTrigIndex;
				}
			}
		}

		void StartPlaying()
		{
			ScratchingSynth.Reset();
			
			float StartTime = ResidualStartTime->GetSeconds();
			float Duration =  ResidualDuration->GetSeconds();
			float PitchScale = GetPitchScaleClamped(*ResidualPitchShift);
			const FResidualDataAssetProxyPtr& ResidualDataProxy = (*ResidualDataReadRef).GetProxy();			
			ScratchingSynth = MakeUnique<FScratchingSynth>(SamplingRate, OperatorSettings.GetNumFramesPerBlock(), OutputAudioView.Num(),
														   Seed, ResidualDataProxy, *ResidualSpeedScale, *ResidualAmpScale,
														   PitchScale, StartTime, Duration);
			
			*OutSeed = ScratchingSynth->GetSeed();
			bIsPlaying = ScratchingSynth.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;

			bool bIsStop = true;
			if(ScratchingSynth.IsValid())
			{
				const int32 NumFramesToGenerate = EndFrame - StartFrame;
				if (NumFramesToGenerate <= 0)
				{
					UE_LOG(LogImpactSFXSynth, Warning, TEXT("ScratchingSynthNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
					return;
				}
			
				FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);

				const FScratchImpactSpawnParams SpawnParams = FScratchImpactSpawnParams(GetClampNonNegative(*ImpactSpawnRate),
																						GetClampNonNegative(*ImpactSpawnChance),
																						*ImpactChanceDecayRate,
																						ImpactSpawnDuration->GetSeconds(),
																						GetClampNonNegative(*ImpactStrengthMin),
																						GetClampNonNegative(*ImpactStrengthRange),
																						GetClampNonNegative(*ImpactStrengthDecay),
																						GetClampFrameStart(*FrameStartMin),
																						GetClampFrameStart(*FrameStartMax),
																						GetPitchScaleClamped(*ResidualPitchShift));
				
				const FImpactModalObjAssetProxyPtr& ExciteModalProxy = ExciteModalObjReadRef->GetProxy();
				FModalMods ExciteModalMods;
				ExciteModalMods.NumModals = *ExciteNumModal;
				ExciteModalMods.AmplitudeScale = *ExciteAmplScale;
				ExciteModalMods.DecayScale = *ExciteDecayScale;
				ExciteModalMods.FreqScale = GetPitchScaleClamped(*ExcitePitchShift);
				
				const FImpactModalObjAssetProxyPtr& ResonanceModalProxy = ResonanceModalObjReadRef->GetProxy();
				FModalMods ResonanceModalMods;
				ResonanceModalMods.NumModals = *ResonanceNumModal;
				ResonanceModalMods.AmplitudeScale = *ResonanceAmplScale;
				ResonanceModalMods.DecayScale = *ResonanceDecayScale;
				ResonanceModalMods.FreqScale = GetPitchScaleClamped(*ResonancePitchShift);
				
				bIsStop = ScratchingSynth->Synthesize(SpawnParams,ExciteModalProxy, ExciteModalMods,
													  ResonanceModalProxy, ResonanceModalMods,
													  BufferToGenerate, bClamp);
			}
			
			if(bIsStop)
				StopPlaying(EndFrame, false);
		}
		
		void StopPlaying(const int32 EndFrame, const bool bForceStop = true)
		{
			bIsPlaying = false;
			if(bForceStop && ScratchingSynth.IsValid())
				ScratchingSynth.Reset();
			TriggerOnDone->TriggerFrame(EndFrame);
		}

		float GetClampNonNegative(float InValue)
		{
			return FMath::Max(InValue, 0.f);
		}

		int32 GetClampFrameStart(int32 InValue)
		{
			return FMath::Clamp(InValue, 0, 100);
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		int32 Seed;
		bool bClamp;
		
		FFloatReadRef ImpactSpawnRate;
		FFloatReadRef ImpactSpawnChance;
		FFloatReadRef ImpactChanceDecayRate;
		FTimeReadRef ImpactSpawnDuration;
		
		FFloatReadRef ImpactStrengthMin;
		FFloatReadRef ImpactStrengthRange;
		FFloatReadRef ImpactStrengthDecay;
		FInt32ReadRef FrameStartMin;
		FInt32ReadRef FrameStartMax;
		
		FImpactModalObjReadRef ExciteModalObjReadRef;
		FInt32ReadRef ExciteNumModal;
		FFloatReadRef ExciteAmplScale;
		FFloatReadRef ExciteDecayScale;
		FFloatReadRef ExcitePitchShift;

		FImpactModalObjReadRef ResonanceModalObjReadRef;
		FInt32ReadRef ResonanceNumModal;
		FFloatReadRef ResonanceAmplScale;
		FFloatReadRef ResonanceDecayScale;
		FFloatReadRef ResonancePitchShift;

		FResidualDataReadRef ResidualDataReadRef;
		FFloatReadRef ResidualAmpScale;
		FFloatReadRef ResidualSpeedScale;
		FFloatReadRef ResidualPitchShift;
		FTimeReadRef ResidualStartTime;
		FTimeReadRef ResidualDuration;
		
		FTriggerWriteRef TriggerOnDone;
		FInt32WriteRef OutSeed;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;

		TUniquePtr<FScratchingSynth> ScratchingSynth;
		Audio::FMultichannelBufferView OutputAudioView;
		
		float SamplingRate;
		int32 NumOutputChannels;
		bool bIsPlaying = false;
	};

	class FScratchingSynthOperatorFactory : public IOperatorFactory
	{
	public:
		FScratchingSynthOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace ScratchingSynthVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FScratchingSynthOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputSeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<bool>(METASOUND_GET_PARAM_NAME(InputIsClamped), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactSpawnRate), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactSpawnChance), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactSpawnDecayRate), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputImpactSpawnDuration), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthDecay), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputFrameStartMin), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputFrameStartMax), InParams.OperatorSettings),

				Inputs.GetOrConstructDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputExciteModalData)),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputExciteNumModal), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputExciteAmpScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputExciteDecayScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputExcitePitchShift), InParams.OperatorSettings),

				Inputs.GetOrConstructDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputResonanceModalData)),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputResonanceNumModal), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResonanceAmpScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResonanceDecayScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResonancePitchShift), InParams.OperatorSettings),

				Inputs.GetOrConstructDataReadReference<FResidualData>(METASOUND_GET_PARAM_NAME(InputResidualData)),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualAmpScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualSpeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputResidualStartTime), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputResidualDuration), InParams.OperatorSettings)
			};

			return MakeUnique<FScratchingSynthOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};
	
	template<typename AudioChannelConfigurationInfoType>
	class TScratchingSynthNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace ScratchingSynthVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
					
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSeed), -1),
					TInputConstructorVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsClamped), true),
					
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnRate), 50.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnChance), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnDecayRate), 0.0f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnDuration), -1.f),

					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthMin), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthRange), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthDecay), 0.0f),

					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFrameStartMin), 0),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFrameStartMax), 2),
		
					TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputExciteModalData)),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputExciteNumModal), -1),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputExciteAmpScale), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputExciteDecayScale), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputExcitePitchShift), 0.0f),

					TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResonanceModalData)),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResonanceNumModal), -1),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResonanceAmpScale), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResonanceDecayScale), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResonancePitchShift), 0.0f),

					TInputDataVertex<FResidualData>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualData)),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualAmpScale), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualSpeed), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualPitchShift), 0.0f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualStartTime), 0.0f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualDuration), -1.0f)
					),
				FOutputVertexInterface(
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnPlay)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone)),
					TOutputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputCurrentSeed))					
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
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Scratching Synth"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_ScratchingSynthNodeDescription", "Simulating Scratching SFX.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("ScratchingSyntKeyword", "Synthesis") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TScratchingSynthNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FScratchingSynthOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TScratchingSynthNode(const FNodeInitData& InInitData)
			: TScratchingSynthNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TScratchingSynthNode() = default;

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

	struct FScratchingSynthMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ScratchingSynthMonoNodeDisplayName", "Scratching Synth (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ScratchingSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoScratchingSynthNode = TScratchingSynthNode<FScratchingSynthMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoScratchingSynthNode);
	
	struct FScratchingSynthStereoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_ScratchingSynthStereoNodeDisplayName", "Scratching Synth (Stereo)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::StereoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace ScratchingSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			};
		}
	};
	using FStereoScratchingSynthNode = TScratchingSynthNode<FScratchingSynthStereoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FStereoScratchingSynthNode);

	
}

#undef LOCTEXT_NAMESPACE
