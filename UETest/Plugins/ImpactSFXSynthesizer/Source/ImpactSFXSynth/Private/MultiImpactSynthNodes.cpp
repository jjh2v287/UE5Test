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
#include "MetasoundEnumRegistrationMacro.h"
#include "MetasoundMultiImpactData.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTime.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "MultiImpactSynth.h"

DECLARE_CYCLE_STAT(TEXT("Multi Impact SFX - Total Synth Time"), STAT_MultiImpactSynth, STATGROUP_ImpactSFXSynth);

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ImpactSynthNodes"

namespace Metasound
{
	DECLARE_METASOUND_ENUM(LBSImpactSFXSynth::EMultiImpactVariationSpawnType, LBSImpactSFXSynth::EMultiImpactVariationSpawnType::Random, IMPACTSFXSYNTH_API,
    					   FMultiImpactVariationSpawnType, FMultiImpactVariationSpawnTypeTypeInfo, FMultiImpactVariationSpawnTypeReadRef, FMultiImpactVariationSpawnTypeWriteRef);
	
    DEFINE_METASOUND_ENUM_BEGIN(LBSImpactSFXSynth::EMultiImpactVariationSpawnType, Metasound::FMultiImpactVariationSpawnType, "MultiImpactVariationSpawnType")
    	DEFINE_METASOUND_ENUM_ENTRY(LBSImpactSFXSynth::EMultiImpactVariationSpawnType::Random, "RandomDescription", "Random", "RandomDescriptionTT", "Use variations randomly."),
		DEFINE_METASOUND_ENUM_ENTRY(LBSImpactSFXSynth::EMultiImpactVariationSpawnType::Increment, "IncrementDescription", "Increment", "IncrementDescriptionTT", "Incremently use variations from 0 index."),
		DEFINE_METASOUND_ENUM_ENTRY(LBSImpactSFXSynth::EMultiImpactVariationSpawnType::Decrement, "DecrementDescription", "Decrement", "DecrementDescriptionTT", "Decremently Use variations from 0 index."),
	DEFINE_METASOUND_ENUM_ENTRY(LBSImpactSFXSynth::EMultiImpactVariationSpawnType::PingPong, "PingPongDescription", "PingPong", "PingPongDescriptionTT", "Use varitaions from first to last then revert."),
    DEFINE_METASOUND_ENUM_END()
}

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace MultiImpactSynthVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "Stop synthesizing.")

		METASOUND_PARAM(InputMultiImpactData, "Multi Impact Data", "Impact variations applied to spawned impacts.")
		
		METASOUND_PARAM(InputSeed, "Seed", "Radomizer seed. If <= -1, use random seed.")
		METASOUND_PARAM(InputMaxSpawnedImpacts, "Maximum Number of Impacts", "The maximum number of impacts allowed to be spawned simultaneously.")
		METASOUND_PARAM(InputIsStopSpawningWhenMax, "Stop Spawning on Max", "If true, when we've reached the allowed maximum number of impacts, no new impacts will be spawned until an old impact has finished. If false, the weakest impact in the current pool will be removed to spawn a new impact.")
		METASOUND_PARAM(InputVariationSpawnType, "Variation Spawn Type", "Determine how variations are chosen when spawning.")
		METASOUND_PARAM(InputImpactSpawnDelay, "Spawn Start Time Range", "If <= 0, all impacts spawned together in a frame will be played at the same time. If > 0, the start time of each impact is delayed randomly.")
		METASOUND_PARAM(InputImpactSpawnBurst, "Spawn Burst", "The number of impacts spawned at time 0.")
		METASOUND_PARAM(InputImpactSpawnRate, "Spawn Rate", "The number of spawning intervals per second. ")
		METASOUND_PARAM(InputImpactSpawnCount, "Spawn Count", "The number of impacts spawned at each spawning interval.")
		METASOUND_PARAM(InputImpactSpawnChance, "Spawn Chance", "Range [0, 1]. The chance at which new impacts are spawned.")
		METASOUND_PARAM(InputImpactSpawnDecayRate, "Spawn Chance Decay", "New impacts spawning chance is reduced (or increased if < 0) based on an exoponential function over time.")
		METASOUND_PARAM(InputImpactSpawnDuration, "Spawning Duration", "If < 0 and spawning chance decay rate > 0, new impacts are generated untill spawning chance = 0.")
		
		METASOUND_PARAM(InputImpactStrengthMin, "Impact Strength Min", "Range [0.1, 10].")
		METASOUND_PARAM(InputImpactStrengthRange, "Impact Strength Range", "Impact Strength = Rand(Impact Strength Min, Impact Strength Min + Range).")
		METASOUND_PARAM(InputImpactStrengthDecay, "Impact Strength Decay Rate", "The strength of a new impact is reduced (or increased if < 0) based on an exoponential function over time.")

		METASOUND_PARAM(InputDecayScale, "Modal Decay Scale", "Global decay mutiplier applied to all spawned modal synthesizers.")
		METASOUND_PARAM(InputResidualSpeedScale, "Residual Speed Scale", "Global speed mutiplier applied to all spawned residual synthesizers.")

		METASOUND_PARAM(InputModalPitchShift, "Modal Pitch Shift", "Global pitch shift applied to all spawned modal synthesizers.")
		METASOUND_PARAM(InputResidualPitchShift, "Residual Pitch Shift", "Global pitch shift applied to all spawned residual synthesizers.")
		
		METASOUND_PARAM(InputIsClamped, "Clamp", "Clamp output to [-1, 1] or not.")

		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the impact SFX energy decays to zero or reach the specified duration.")

		METASOUND_PARAM(OutputCurrentSeed, "Current Seed", "Currently used seed.")
		
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "The left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "The right channel audio output.")
	}

	struct FMultiImpactSynthOpArgs
	{
		FOperatorSettings Settings;
		TArray<FOutputDataVertex> OutputAudioVertices;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;

		FMultiImpactDataReadRef MultiImpactData;
		int32 Seed;
		int32 MaxNumImpacts;
		bool bIsStopSpawningOnMax;
		FMultiImpactVariationSpawnTypeReadRef VariationSpawnType;
		
		FTimeReadRef ImpactSpawnDelay;
		FInt32ReadRef ImpactSpawnBurst;
		FFloatReadRef ImpactSpawnRate;
		FInt32ReadRef ImpactSpawnCount;
		FFloatReadRef ImpactSpawnChance;
		FFloatReadRef ImpactChanceDecayRate;
		FTimeReadRef ImpactSpawnDuration;
		
		FFloatReadRef ImpactStrengthMin;
		FFloatReadRef ImpactStrengthRange;
		FFloatReadRef ImpactStrengthDecay;

		FFloatReadRef ModalDecayScale;
		FFloatReadRef ResidualSpeedScale;
		
		FFloatReadRef ModalPitchShift;
		FFloatReadRef ResidualPitchShift;
		
		bool bClamp;
	};
	
	class FMultiImpactSynthOperator : public TExecutableOperator<FMultiImpactSynthOperator>
	{
	public:

		FMultiImpactSynthOperator(const FMultiImpactSynthOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, MultiImpactData(InArgs.MultiImpactData)
			, Seed(InArgs.Seed)
			, MaxNumImpacts(InArgs.MaxNumImpacts)
			, bIsStopSpawningOnMax(InArgs.bIsStopSpawningOnMax)
			, VariationSpawnType(InArgs.VariationSpawnType)
			, ImpactSpawnDelay(InArgs.ImpactSpawnDelay)
			, ImpactSpawnBurst(InArgs.ImpactSpawnBurst)
			, ImpactSpawnRate(InArgs.ImpactSpawnRate)
			, ImpactSpawnCount(InArgs.ImpactSpawnCount)
			, ImpactSpawnChance(InArgs.ImpactSpawnChance)
			, ImpactChanceDecayRate(InArgs.ImpactChanceDecayRate)
			, ImpactSpawnDuration(InArgs.ImpactSpawnDuration)
			, ImpactStrengthMin(InArgs.ImpactStrengthMin)
			, ImpactStrengthRange(InArgs.ImpactStrengthRange)
			, ImpactStrengthDecay(InArgs.ImpactStrengthDecay)
			, ModalDecayScale(InArgs.ModalDecayScale)
			, ResidualSpeedScale(InArgs.ResidualSpeedScale)
			, ModalPitchShift(InArgs.ModalPitchShift)
			, ResidualPitchShift(InArgs.ResidualPitchShift)
			, bClamp(InArgs.bClamp)
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
			using namespace MultiImpactSynthVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputMultiImpactData), MultiImpactData);
			
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputMaxSpawnedImpacts), MaxNumImpacts);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputIsStopSpawningWhenMax), bIsStopSpawningOnMax);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputVariationSpawnType), VariationSpawnType);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDelay), ImpactSpawnDelay);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnBurst), ImpactSpawnBurst);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnRate), ImpactSpawnRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnCount), ImpactSpawnCount);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnChance), ImpactSpawnChance);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDecayRate), ImpactChanceDecayRate);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDuration), ImpactSpawnDuration);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), ImpactStrengthMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), ImpactStrengthRange);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthDecay), ImpactStrengthDecay);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), ModalDecayScale);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSpeedScale), ResidualSpeedScale);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalPitchShift), ModalPitchShift);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputIsClamped), bClamp);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace MultiImpactSynthVertexNames;
			
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
			using namespace MultiImpactSynthVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputMultiImpactData), MultiImpactData);
			
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputMaxSpawnedImpacts), MaxNumImpacts);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputIsStopSpawningWhenMax), bIsStopSpawningOnMax);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputVariationSpawnType), VariationSpawnType);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDelay), ImpactSpawnDelay);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnBurst), ImpactSpawnBurst);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnRate), ImpactSpawnRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnCount), ImpactSpawnCount);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnChance), ImpactSpawnChance);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDecayRate), ImpactChanceDecayRate);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactSpawnDuration), ImpactSpawnDuration);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), ImpactStrengthMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), ImpactStrengthRange);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputImpactStrengthDecay), ImpactStrengthDecay);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayScale), ModalDecayScale);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualSpeedScale), ResidualSpeedScale);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalPitchShift), ModalPitchShift);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), ResidualPitchShift);
			
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputIsClamped), bClamp);

			
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

			SCOPE_CYCLE_COUNTER(STAT_MultiImpactSynth);
			
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

			MultiImpactSynth.Reset();
			
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
			MultiImpactSynth.Reset();

			const FMultiImpactDataAssetProxyPtr MultImpactProxy = MultiImpactData->GetProxy();
			if(MultImpactProxy.IsValid())
			{
				MultiImpactSynth = MakeUnique<FMultiImpactSynth>(MultImpactProxy.ToSharedRef(), GetMaxNumImpactsClamped(), SamplingRate,
											   OperatorSettings.GetNumFramesPerBlock(), OutputAudioView.Num(), bIsStopSpawningOnMax, 
											   Seed, *VariationSpawnType);
				*OutSeed = MultiImpactSynth->GetSeed();
			}
			
			bIsPlaying = MultiImpactSynth.IsValid() &&  MultiImpactSynth->CanRun();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;

			bool bIsStop = true;
			if(MultiImpactSynth.IsValid())
			{
				const int32 NumFramesToGenerate = EndFrame - StartFrame;
				if (NumFramesToGenerate <= 0)
				{
					UE_LOG(LogImpactSFXSynth, Warning, TEXT("MultiImpactSynthNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
					return;
				}
			
				FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);

				const FMultiImpactSpawnParams SpawnParams = FMultiImpactSpawnParams(*ImpactSpawnBurst,
																					*ImpactSpawnRate,
																					*ImpactSpawnCount,
																					*ImpactSpawnChance,
																					*ImpactChanceDecayRate,
																					ImpactSpawnDuration->GetSeconds(),
																					ImpactSpawnDelay->GetSeconds(),
																					*ImpactStrengthMin,
																					*ImpactStrengthRange,
																					*ImpactStrengthDecay);
				bIsStop = MultiImpactSynth->Synthesize(BufferToGenerate, SpawnParams,
													  *ModalDecayScale, *ResidualSpeedScale,
													  *ModalPitchShift, *ResidualPitchShift, bClamp);
			}
			
			if(bIsStop)
				StopPlaying(EndFrame, false);
		}
		
		void StopPlaying(const int32 EndFrame, const bool bForceStop = true)
		{
			bIsPlaying = false;
			if(bForceStop && MultiImpactSynth.IsValid())
				MultiImpactSynth.Reset();
			TriggerOnDone->TriggerFrame(EndFrame);
		}
		
		int32 GetMaxNumImpactsClamped() const
		{
			return FMath::Clamp(MaxNumImpacts, 1, 1000);
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;

		FMultiImpactDataReadRef MultiImpactData;
		int32 Seed;
		int32 MaxNumImpacts;
		bool bIsStopSpawningOnMax;
		FMultiImpactVariationSpawnTypeReadRef VariationSpawnType;

		FTimeReadRef ImpactSpawnDelay;
		FInt32ReadRef ImpactSpawnBurst;
		FFloatReadRef ImpactSpawnRate;
		FInt32ReadRef ImpactSpawnCount;
		FFloatReadRef ImpactSpawnChance;
		FFloatReadRef ImpactChanceDecayRate;
		FTimeReadRef ImpactSpawnDuration;
		
		FFloatReadRef ImpactStrengthMin;
		FFloatReadRef ImpactStrengthRange;
		FFloatReadRef ImpactStrengthDecay;

		FFloatReadRef ModalDecayScale;
		FFloatReadRef ResidualSpeedScale;

		FFloatReadRef ModalPitchShift;
		FFloatReadRef ResidualPitchShift;
		
		bool bClamp;
		
		FTriggerWriteRef TriggerOnDone;
		FInt32WriteRef OutSeed;
		TArray<FAudioBufferWriteRef> OutputAudioBuffers;
		TArray<FName> OutputAudioBufferVertexNames;

		TUniquePtr<FMultiImpactSynth> MultiImpactSynth;
		Audio::FMultichannelBufferView OutputAudioView;
		
		float SamplingRate;
		int32 NumOutputChannels;
		bool bIsPlaying = false;
	};

	class FMultiImpactSynthOperatorFactory : public IOperatorFactory
	{
	public:
		FMultiImpactSynthOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace MultiImpactSynthVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FMultiImpactSynthOpArgs Args =
			{
				InParams.OperatorSettings,
				OutputAudioVertices,
				
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<FMultiImpactData>(METASOUND_GET_PARAM_NAME(InputMultiImpactData), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputSeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputMaxSpawnedImpacts), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<bool>(METASOUND_GET_PARAM_NAME(InputIsStopSpawningWhenMax), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FMultiImpactVariationSpawnType>(METASOUND_GET_PARAM_NAME(InputVariationSpawnType), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputImpactSpawnDelay), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputImpactSpawnBurst), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactSpawnRate), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputImpactSpawnCount), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactSpawnChance), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactSpawnDecayRate), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<FTime>(METASOUND_GET_PARAM_NAME(InputImpactSpawnDuration), InParams.OperatorSettings),
				

				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthMin), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthRange), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputImpactStrengthDecay), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayScale), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualSpeedScale), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputModalPitchShift), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputResidualPitchShift), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultValue<bool>(METASOUND_GET_PARAM_NAME(InputIsClamped), InParams.OperatorSettings)
			};

			return MakeUnique<FMultiImpactSynthOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};
	
	template<typename AudioChannelConfigurationInfoType>
	class TMultiImpactSynthNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace MultiImpactSynthVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
					
					TInputDataVertex<FMultiImpactData>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputMultiImpactData)),
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSeed), -1),
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputMaxSpawnedImpacts), 25),
					TInputConstructorVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsStopSpawningWhenMax), false),
					
					TInputDataVertex<FMultiImpactVariationSpawnType>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputVariationSpawnType),
						static_cast<int32>(EMultiImpactVariationSpawnType::Random)),
					
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnDelay), 0.2f),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnBurst), 5),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnRate), 0.0f),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnCount), 0.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnChance), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnDecayRate), 1.0f),
					TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactSpawnDuration), -1.f),

					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthMin), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthRange), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputImpactStrengthDecay), 1.0f),

					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayScale), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualSpeedScale), 1.0f),

					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalPitchShift), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputResidualPitchShift), 0.0f),
					
					TInputConstructorVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsClamped), true)
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
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Multi Impact SFX Synth"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_MultiImpactSynthNodeDescription", "Simulating Multiple Impacts.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("MultiImpactSFXSyntKeyword", "Synthesis") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TMultiImpactSynthNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FMultiImpactSynthOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TMultiImpactSynthNode(const FNodeInitData& InInitData)
			: TMultiImpactSynthNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TMultiImpactSynthNode() = default;

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

	struct FMultiImpactSynthMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_MultiImpactSynthMonoNodeDisplayName", "Multi Impact SFX Synth (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace MultiImpactSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			};
		}
	};
	using FMonoMultiImpactSynthNode = TMultiImpactSynthNode<FMultiImpactSynthMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoMultiImpactSynthNode);
	
	struct FMultiImpactSynthStereoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_MultiImpactSynthStereoNodeDisplayName", "Multi Impact SFX Synth (Stereo)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::StereoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace MultiImpactSynthVertexNames;
			return {
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			};
		}
	};
	using FStereoMultiImpactSynthNode = TMultiImpactSynthNode<FMultiImpactSynthStereoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FStereoMultiImpactSynthNode);

	
}

#undef LOCTEXT_NAMESPACE
