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
#include "..\Public\VehicleEngineSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_VehicleEngineForceGenNodes"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace VehicleEngineSynthVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		METASOUND_PARAM(InputTriggerStop, "Stop", "stop synthesizing.")		

		METASOUND_PARAM(InputSeed, "Seed", "Radomizer seed. If <= -1, use random seed.")
		METASOUND_PARAM(InputNumPulsePerRevolution, "Num Pulse Per Revolution", "This parameter depends on the type of engine. For example, a four-stroke and 8 cylinders engine has 0.5*8 = 4 pulses per revolution. A two-stroke and 2 cylinders engine has 1*2 = 2 pulses per revolution.")
		
		METASOUND_PARAM(InputRPM, "RPM", "Current revolution per minutes.")
		METASOUND_PARAM(InputIdleRPM, "Idle RPM", "For cars, idle RPM is around 600 to 1000.")
		METASOUND_PARAM(InputRPMNoiseFactor, "RPM Var Noise Factor", "The changing speed of RPM affects the noise of engine. This parameter is used to control this effect.")
		METASOUND_PARAM(InputThrottle, "Throttle Input", "The throttle input value from the player controller. (This input is usually mapped to W/S on keyboards or Analog Up/Down on controllers.)")
		
		METASOUND_PARAM(InputRandPeriod, "Random Period", "The period to randomize the gain and frequency of each modal. This is scaled down by RPM Var Noise Factor.")
		
		METASOUND_PARAM(InputFreqFluctuation, "F0 Fluctuation", "The fluctuation range of the base frequency (F0) in Hz.")
		METASOUND_PARAM(InputFreqNoiseScale, "Harmonic Fluctuation Scale", "> 0. This scales the fluctuation range of harnomic frequencies.")

		METASOUND_PARAM(InputAmpRandMin, "Min Gain Rand Percent", "The minimum percentage to randomize the gain of each modal.")
		METASOUND_PARAM(InputAmpRandMax, "Max Gain Rand Percent", "The maximum percentage to randomize the gain of each modal.")

		METASOUND_PARAM(InputNumModalDeceleration, "Num Modal Deceleration", "Number of modal parameters to use when deccelerating or changing gears.")
		
		METASOUND_PARAM(InputModalParams, "Harmonic Modal", "Modal parameters to be used as harmonic components.")
		METASOUND_PARAM(InputNumModalParams, "Num Harmonic Modal", "Number of modal parameters to use.")
		METASOUND_PARAM(InputHarmonicGain, "Harmonic Gain", "The gain of all harmonic components.")
		METASOUND_PARAM(InputHarmonicPitchShift, "Harmonic Pitch Shift", "The frequency scale applied to all harmonic components.")
		
		METASOUND_PARAM(OutputTriggerOnPlay, "On Play", "Triggers when Play is triggered.")
		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when the SFX energy decays to zero or reach the specified duration.")
		
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
		METASOUND_PARAM(OutputF0, "F0", "The base frequency.")
		METASOUND_PARAM(OutputIsDeceleration, "IsDeceleration", "True if RPM is decelerating.")
		METASOUND_PARAM(OutputRPMCurve, "RPM Curve", "The changing curve of RPM scaled with RPM Var Noise Factor.")
	}

	struct FVehicleEngineSynthOpArgs
	{
		FOperatorSettings Settings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;

		int32 Seed;
		int32 NumPulse;
		
		FFloatReadRef RPM;
		FFloatReadRef IdleRPM;
		FFloatReadRef RPMNoiseFactor;
		FFloatReadRef ThrottleInput;
		FFloatReadRef RandPeriod;
		
		FFloatReadRef F0Fluctuation;
		FFloatReadRef HarmonicFluctuation;

		FFloatReadRef AmpRandMin;
		FFloatReadRef AmpRandMax;

		FInt32ReadRef NumModalDeceleration;
		
		FImpactModalObjReadRef ModalParams;
		FInt32ReadRef NumHarmonicModal;
		float HarmonicGain;
        float HarmonicPitchShift;
	};
	
	class FVehicleEngineSynthOperator : public TExecutableOperator<FVehicleEngineSynthOperator>
	{
	public:

		FVehicleEngineSynthOperator(const FVehicleEngineSynthOpArgs& InArgs)
			: OperatorSettings(InArgs.Settings)
			, PlayTrigger(InArgs.PlayTrigger)
			, StopTrigger(InArgs.StopTrigger)
			, Seed(InArgs.Seed)
			, NumPulse(InArgs.NumPulse)
			, RPM(InArgs.RPM)
			, IdleRPM(InArgs.IdleRPM)
			, RPMNoiseFactor(InArgs.RPMNoiseFactor)
			, ThrottleInput(InArgs.ThrottleInput)
			, RandPeriod(InArgs.RandPeriod)
			, F0Fluctuation(InArgs.F0Fluctuation)
			, HarmonicFluctuation(InArgs.HarmonicFluctuation)
			, AmpRandMin(InArgs.AmpRandMin)
			, AmpRandMax(InArgs.AmpRandMax)
			, NumModalDeceleration(InArgs.NumModalDeceleration)
			, ModalParams(InArgs.ModalParams)
			, NumHarmonicModal(InArgs.NumHarmonicModal)
			, HarmonicGain(InArgs.HarmonicGain)
			, HarmonicPitchShift(InArgs.HarmonicPitchShift)
			, TriggerOnDone(FTriggerWriteRef::CreateNew(InArgs.Settings))
			, OutMonoWriteBuffer(FAudioBufferWriteRef::CreateNew(InArgs.Settings))
			, OutF0(FFloatWriteRef::CreateNew(0.f))
			, IsDeceleration(FBoolWriteRef::CreateNew(false))
			, RPMCurve(FFloatWriteRef::CreateNew(0.f))
		{
			SamplingRate = OperatorSettings.GetSampleRate();

			// Hold on to a view of the output audio
			OutputAudioView.Emplace(OutMonoWriteBuffer->GetData(), OutMonoWriteBuffer->Num());
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace VehicleEngineSynthVertexNames;
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputNumPulsePerRevolution), NumPulse);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPM), RPM);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIdleRPM), IdleRPM);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPMNoiseFactor), RPMNoiseFactor);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputThrottle), ThrottleInput);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRandPeriod), RandPeriod);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFreqFluctuation), F0Fluctuation);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFreqNoiseScale), HarmonicFluctuation);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmpRandMin), AmpRandMin);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmpRandMax), AmpRandMax);

			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModalDeceleration), NumModalDeceleration);
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModalParams), NumHarmonicModal);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputHarmonicGain), HarmonicGain);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(InputHarmonicPitchShift), HarmonicPitchShift);			
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace VehicleEngineSynthVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioMono), OutMonoWriteBuffer);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputF0), OutF0);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputIsDeceleration), IsDeceleration);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputRPMCurve), RPMCurve);
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace VehicleEngineSynthVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerStop), StopTrigger);
			
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputSeed), Seed);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputNumPulsePerRevolution), NumPulse);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPM), RPM);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIdleRPM), IdleRPM);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRPMNoiseFactor), RPMNoiseFactor);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputThrottle), ThrottleInput);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRandPeriod), RandPeriod);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFreqFluctuation), F0Fluctuation);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFreqNoiseScale), HarmonicFluctuation);
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmpRandMin), AmpRandMin);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmpRandMax), AmpRandMax);

			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModalDeceleration), NumModalDeceleration);
			
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputModalParams), ModalParams);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModalParams), NumHarmonicModal);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputHarmonicGain), HarmonicGain);
			Inputs.SetValue(METASOUND_GET_PARAM_NAME(InputHarmonicPitchShift), HarmonicPitchShift);
			
			FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnPlay), PlayTrigger);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioMono), OutMonoWriteBuffer);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputF0), OutF0);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputIsDeceleration), IsDeceleration);
			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputRPMCurve), RPMCurve);
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
			METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::VehicleEngineSynthVertexNames::Execute);
			
			TriggerOnDone->AdvanceBlock();
			
			FMemory::Memzero(OutMonoWriteBuffer->GetData(), OperatorSettings.GetNumFramesPerBlock() * sizeof(float));
			
			ExecuteSubBlocks();
		}

#if ENGINE_MINOR_VERSION > 2
		void Reset(const IOperator::FResetParams& InParams)
		{
			TriggerOnDone->Reset();

			OutMonoWriteBuffer->Zero();

			VehicleEngineSynth.Reset();
			
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
			VehicleEngineSynth.Reset();
			
			VehicleEngineSynth = MakeUnique<FVehicleEngineSynth>(SamplingRate, NumPulse,
																		ModalParams->GetProxy(), *NumHarmonicModal,
																		HarmonicGain, GetPitchScaleClamped(HarmonicPitchShift),
																		Seed);
			bIsPlaying = VehicleEngineSynth.IsValid();
		}

		void RenderFrameRange(int32 StartFrame, int32 EndFrame)
		{
			if(!bIsPlaying)
				return;
			
			const int32 NumFramesToGenerate = EndFrame - StartFrame;
			if (NumFramesToGenerate <= 0)
			{
				UE_LOG(LogImpactSFXSynth, Warning, TEXT("VehicleEngineSynthNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
				return;
			}
			
			if(VehicleEngineSynth.IsValid())
			{
				FMultichannelBufferView BufferToGenerate = SliceMultichannelBufferView(OutputAudioView, StartFrame, NumFramesToGenerate);
            	const FVehicleEngineParams Params = FVehicleEngineParams(*RPM, *IdleRPM, *RPMNoiseFactor, *ThrottleInput,
            															*RandPeriod, *F0Fluctuation, *HarmonicFluctuation,
            															*AmpRandMin, *AmpRandMax, *NumModalDeceleration);
            												
				const FImpactModalObjAssetProxyPtr& ImpactModalProxy = ModalParams->GetProxy();
				VehicleEngineSynth->Generate(BufferToGenerate, Params, ImpactModalProxy);
				*OutF0 = VehicleEngineSynth->GetCurrentBaseFreq();
				*IsDeceleration = VehicleEngineSynth->IsInDeceleration();
				*RPMCurve = VehicleEngineSynth->GetRPMCurve();
			}
			else
			{
				VehicleEngineSynth.Reset();
				bIsPlaying = false;
				TriggerOnDone->TriggerFrame(EndFrame);
			}
		}
		
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FTriggerReadRef StopTrigger;
		
		int32 Seed;
		int32 NumPulse;
		FFloatReadRef RPM;
		FFloatReadRef IdleRPM;
		FFloatReadRef RPMNoiseFactor;
		FFloatReadRef ThrottleInput;
		FFloatReadRef RandPeriod;
		
		FFloatReadRef F0Fluctuation;
		FFloatReadRef HarmonicFluctuation;

		FFloatReadRef AmpRandMin;
		FFloatReadRef AmpRandMax;
		
		FInt32ReadRef NumModalDeceleration;

		FImpactModalObjReadRef ModalParams;
		FInt32ReadRef NumHarmonicModal;
		float HarmonicGain;
		float HarmonicPitchShift;
		
		FTriggerWriteRef TriggerOnDone;
		FAudioBufferWriteRef OutMonoWriteBuffer;
		FFloatWriteRef OutF0;
		FBoolWriteRef IsDeceleration;
		FFloatWriteRef RPMCurve;
		
		TUniquePtr<FVehicleEngineSynth> VehicleEngineSynth;
		Audio::FMultichannelBufferView OutputAudioView;
				
		float SamplingRate;
		bool bIsPlaying = false;
	};

	class FVehicleEngineSynthOperatorFactory : public IOperatorFactory
	{
	public:
		FVehicleEngineSynthOperatorFactory(const TArray<FOutputDataVertex>& InOutputAudioVertices)
		: OutputAudioVertices(InOutputAudioVertices)
		{
		}
		
		virtual TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults) override
		{
			using namespace Audio;
			using namespace VehicleEngineSynthVertexNames;
		
			const FInputVertexInterfaceData& Inputs = InParams.InputData;
			
			FVehicleEngineSynthOpArgs Args =
			{
				InParams.OperatorSettings,
				
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings),
				Inputs.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerStop), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputSeed), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<int32>(METASOUND_GET_PARAM_NAME(InputNumPulsePerRevolution), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputRPM), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputIdleRPM), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputRPMNoiseFactor), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputThrottle), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputRandPeriod), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputFreqFluctuation), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputFreqNoiseScale), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmpRandMin), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmpRandMax), InParams.OperatorSettings),

				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModalDeceleration), InParams.OperatorSettings),
				
				Inputs.GetOrCreateDefaultDataReadReference<FImpactModalObj>(METASOUND_GET_PARAM_NAME(InputModalParams), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModalParams), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<float>(METASOUND_GET_PARAM_NAME(InputHarmonicGain), InParams.OperatorSettings),
				Inputs.GetOrCreateDefaultValue<float>(METASOUND_GET_PARAM_NAME(InputHarmonicPitchShift), InParams.OperatorSettings)
			};

			return MakeUnique<FVehicleEngineSynthOperator>(Args);
		}
	private:
		TArray<FOutputDataVertex> OutputAudioVertices;
	};

	
	template<typename AudioChannelConfigurationInfoType>
	class TVehicleEngineSynthNode : public FNode
	{

	public:
		static FVertexInterface DeclareVertexInterface()
		{
			using namespace VehicleEngineSynthVertexNames;
			
			FVertexInterface VertexInterface(
				FInputVertexInterface(
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
					TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerStop)),
			
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSeed), -1),
					TInputConstructorVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumPulsePerRevolution), 4),
					
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRPM), 600.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIdleRPM), 600.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRPMNoiseFactor), 0.01f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputThrottle), 1.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRandPeriod), 0.03f),
					
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFreqFluctuation), 5.f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFreqNoiseScale), 1.f),
					
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmpRandMin), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmpRandMax), 1.f),

					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModalDeceleration), 4),
					
					TInputDataVertex<FImpactModalObj>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputModalParams)),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModalParams), -1),					
					TInputConstructorVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputHarmonicGain), 1.f),
					TInputConstructorVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputHarmonicPitchShift), 0.f)
					),
				FOutputVertexInterface(
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnPlay)),
					TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone)),
					TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono)),
					TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputF0)),
					TOutputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputIsDeceleration)),
					TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputRPMCurve))
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
				Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Vehicle Engine Synth"), AudioChannelConfigurationInfoType::GetVariantName() };
				Info.MajorVersion = 1;
				Info.MinorVersion = 0;
				Info.DisplayName = AudioChannelConfigurationInfoType::GetNodeDisplayName();
				Info.Description = METASOUND_LOCTEXT("Metasound_VehicleEngineSynthNodeDescription", "Vehicle Engine Synth.");
				Info.Author = TEXT("Le Binh Son");
				Info.PromptIfMissing = PluginNodeMissingPrompt;
				Info.DefaultInterface = DeclareVertexInterface();
				Info.Keywords = { METASOUND_LOCTEXT("VehicleEngineSyntKeyword", "Synth") };

				return Info;
			};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		TVehicleEngineSynthNode(const FVertexName& InName, const FGuid& InInstanceID)
			:	FNode(InName, InInstanceID, GetNodeInfo())
			,	Factory(MakeOperatorFactoryRef<FVehicleEngineSynthOperatorFactory>(AudioChannelConfigurationInfoType::GetAudioOutputs()))
			,	Interface(DeclareVertexInterface())
		{
		}

		TVehicleEngineSynthNode(const FNodeInitData& InInitData)
			: TVehicleEngineSynthNode(InInitData.InstanceName, InInitData.InstanceID)
		{
		}

		virtual ~TVehicleEngineSynthNode() = default;

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

	struct FVehicleEngineSynthMonoAudioChannelConfigurationInfo
	{
		static FText GetNodeDisplayName() { return METASOUND_LOCTEXT("Metasound_VehicleEngineSynthMonoNodeDisplayName", "Vehicle Engine Synth (Mono)"); }
		static FName GetVariantName() { return ImpactSFXSynthEngineNodes::MonoVariant; }

		static TArray<FOutputDataVertex> GetAudioOutputs()
		{
			using namespace VehicleEngineSynthVertexNames;
			return { };
		}
	};
	using FMonoVehicleEngineSynthNode = TVehicleEngineSynthNode<FVehicleEngineSynthMonoAudioChannelConfigurationInfo>;
	METASOUND_REGISTER_NODE(FMonoVehicleEngineSynthNode);

}

#undef LOCTEXT_NAMESPACE