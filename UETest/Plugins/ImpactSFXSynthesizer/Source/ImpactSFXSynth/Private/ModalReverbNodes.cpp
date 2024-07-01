// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#include "ImpactSFXSynthLog.h"
#include "Runtime/Launch/Resources/Version.h"
#include "DSP/Dsp.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "ImpactSynthEngineNodesName.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundTrace.h"
#include "ModalReverb.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ModalReverbNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ModalReverbVertexNames
	{
		METASOUND_PARAM(InputTriggerPlay, "Play", "Start synthesizing.")
		
		METASOUND_PARAM(InputAudio, "In Audio", "The input audio.")
		METASOUND_PARAM(InputIsInAudioStop, "Is In Audio Stop", "If true, start tracking internal buffers and trigger On Done when all of them have decayed to zero.")
		METASOUND_PARAM(InputIsUpdateParams, "Update Room Params", "If false, room radius, absorption, and open room factor are only updated on init. Otherwise, they are updated per block rate.")
		METASOUND_PARAM(InputNumModalsLow, "Num Modals Low Band", "The number of modals in the frequency range [30, 200] Hz.")
		METASOUND_PARAM(InputNumModalsMid, "Num Modals Mid Band", "The number of modals in the frequency range [205, 4000] Hz.")
		METASOUND_PARAM(InputNumModalsHigh, "Num Modals High Band", "The number of modals in the frequency range [4050, 18000] Hz.")
		METASOUND_PARAM(InputFreqScatterScale, "Freq Scatter Scale", "The frequency scatter scale in percent. This is done to avoid the comb filter effect and add some distortions. A value of 0.05 works fine in most cases. For music, you might reduce this to 0.02 to improve clarity, but you will also have to double the number of modals used.")
		METASOUND_PARAM(InputPhaseScatterScale, "Phase Scatter Scale", "The phase scatter Scale in percent.")
		METASOUND_PARAM(InputDecayMin, "Decay Min", "The minimum decay of all frequencies. The defaul value is 2, which is the base decay for an enclosed, empty room (zero absorption), with a radius of 25m. Normally, the decay rate of your scenes will be higher than this.")
		METASOUND_PARAM(InputDecaySlopeB, "Decay Slope b", "The b coefficient in the equation: Decay = a + b * Freq + c * Freq * Freq.")
		METASOUND_PARAM(InputDecaySlopeC, "Decay Slope c", "The c coefficient in the equation: Decay = a + b * Freq + c * Freq * Freq.")
		METASOUND_PARAM(InputRoomSize, "Room Size", "The room size in the range [100, 5000] Unreal unit. The amplitude and decay rate of all frequencies are scaled inversely with this value.")
		METASOUND_PARAM(InputAbsorption, "Absorption", ">= 0. An absorption of 2 is suitable for most non-empty, enclosed rooms. Increase this if the room has special materials to absorb sound. The absorption of an acoustically treated room can be around 8 to 10.")
		METASOUND_PARAM(InputOpenRoomFactor, "Open Room Factor", "Range [1, 10]. A completely enclosed room = 1. A completely empty outdoor environtment = 10.")
		METASOUND_PARAM(InputAmpScale, "Gain", "The gain of the final output audio.")

		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when all channels decays to zero and IsInAudioStop is true.")
		METASOUND_PARAM(OutputAudio, "Out Mono", "Output audio.")
	}

	class FModalReverbOperator : public TExecutableOperator<FModalReverbOperator>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

		FModalReverbOperator(const FOperatorSettings& InSettings,
							const FTriggerReadRef& InTriggerPlay,
							const FAudioBufferReadRef& InAudioInput,
							const FBoolReadRef& InIsInAudioStop,
							const FBoolReadRef& InIsUpdateParams,
							const FInt32ReadRef& InNumModalsLow,
							const FInt32ReadRef& InNumModalsMid,
							const FInt32ReadRef& InNumModalsHigh,
							const FFloatReadRef& InFreqScatterScale,
							const FFloatReadRef& InPhaseScatterScale,
							const FFloatReadRef& InDecayMin,
							const FFloatReadRef& InDecaySlopeB,
							const FFloatReadRef& InDecaySlopeC,
							const FFloatReadRef& InRoomSize,
							const FFloatReadRef& InAbsorption,
							const FFloatReadRef& InOpenRoomFactor,
							const FFloatReadRef& InAmpScale);

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;
		void Reset(const IOperator::FResetParams& InParams);
		void Execute();
		void ExecuteSubBlocks();
		void InitSynthesizers();
		void RenderFrameRange(int32 StartFrame, int32 EndFrame);
		
	private:
		const FOperatorSettings OperatorSettings;
		
		FTriggerReadRef PlayTrigger;
		FAudioBufferReadRef AudioInput;
		FBoolReadRef bInAudioStop;
		FBoolReadRef bUpdateParams;
		FInt32ReadRef NumModalsLow;
		FInt32ReadRef NumModalsMid;
		FInt32ReadRef NumModalsHigh;
		FFloatReadRef FreqScatterScale;
		FFloatReadRef PhaseScatterScale;
		FFloatReadRef DecayMin;
		
		FFloatReadRef DecaySlopeB;
		FFloatReadRef DecaySlopeC;
		FFloatReadRef RoomSize;
		FFloatReadRef Absorption;
		FFloatReadRef OpenRoomFactor;
		
		FFloatReadRef AmpScale;

		FTriggerWriteRef TriggerOnDone;
		FAudioBufferWriteRef AudioOutput;

		float SamplingRate;
		int32 NumFramesPerBlock;

		TArrayView<const float> InAudioView;
		TArrayView<float> OutputAudioView;
		TUniquePtr<FModalReverb> ModalReverb;
		bool bIsPlaying;
		bool bFinish;

		float InitRoomSize;
		float InitAbsorption;
		float InitOpenRoomFactor;
	};

	FModalReverbOperator::FModalReverbOperator(const FOperatorSettings& InSettings,
		const FTriggerReadRef& InTriggerPlay,
		const FAudioBufferReadRef& InAudioInput,
		const FBoolReadRef& InIsInAudioStop,
		const FBoolReadRef& InIsUpdateParams,
		const FInt32ReadRef& InNumModalsLow,
		const FInt32ReadRef& InNumModalsMid,
		const FInt32ReadRef& InNumModalsHigh,
		const FFloatReadRef& InFreqScatterScale,
		const FFloatReadRef& InPhaseScatterScale,
		const FFloatReadRef& InDecayMin,
		const FFloatReadRef& InDecaySlopeB,
		const FFloatReadRef& InDecaySlopeC,
		const FFloatReadRef& InRoomSize,
		const FFloatReadRef& InAbsorption,
		const FFloatReadRef& InOpenRoomFactor,
		const FFloatReadRef& InAmpScale)
		: OperatorSettings(InSettings)
		, PlayTrigger(InTriggerPlay)
		, AudioInput(InAudioInput)
		, bInAudioStop(InIsInAudioStop)
		, bUpdateParams(InIsUpdateParams)
		, NumModalsLow(InNumModalsLow)
		, NumModalsMid(InNumModalsMid)
		, NumModalsHigh(InNumModalsHigh)
		, FreqScatterScale(InFreqScatterScale)
		, PhaseScatterScale(InPhaseScatterScale)
		, DecayMin(InDecayMin)
		, DecaySlopeB(InDecaySlopeB)
		, DecaySlopeC(InDecaySlopeC)
		, RoomSize(InRoomSize)
		, Absorption(InAbsorption)
		, OpenRoomFactor(InOpenRoomFactor)
		, AmpScale(InAmpScale)
		, TriggerOnDone(FTriggerWriteRef::CreateNew(InSettings))
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
	{
		SamplingRate = InSettings.GetSampleRate();
		NumFramesPerBlock = InSettings.GetNumFramesPerBlock();
		
		InAudioView = TArrayView<const float>(AudioInput->GetData(), AudioInput->Num());
		OutputAudioView = TArrayView<float>(AudioOutput->GetData(), AudioOutput->Num());
		bIsPlaying = false;
		bFinish = false;
		
		InitRoomSize = 1.f;
		InitAbsorption = 1.f;
		InitOpenRoomFactor = 1.f;
	}

	void FModalReverbOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace ModalReverbVertexNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerPlay), PlayTrigger);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAudio), AudioInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsInAudioStop), bInAudioStop);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsUpdateParams), bUpdateParams);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModalsLow), NumModalsLow);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModalsMid), NumModalsMid);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumModalsHigh), NumModalsHigh);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputFreqScatterScale), FreqScatterScale);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPhaseScatterScale), PhaseScatterScale);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecayMin), DecayMin);

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecaySlopeB), DecaySlopeB);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDecaySlopeC), DecaySlopeC);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputRoomSize), RoomSize);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAbsorption), Absorption);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputOpenRoomFactor), OpenRoomFactor);
		
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAmpScale), AmpScale);
	}

	void FModalReverbOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace ModalReverbVertexNames;
		
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudio), AudioOutput);
	}

	FDataReferenceCollection FModalReverbOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FModalReverbOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}
	
	void FModalReverbOperator::Reset(const IOperator::FResetParams& InParams)
	{
		TriggerOnDone->Reset();
		AudioOutput->Zero();
		ModalReverb.Reset();
		
		bIsPlaying = false;
		bFinish = false;
	}

	void FModalReverbOperator::Execute()
	{
		METASOUND_TRACE_CPUPROFILER_EVENT_SCOPE(Metasound::ModalReverbVertexNames::Execute);
		
		TriggerOnDone->AdvanceBlock();

		// zero output buffers
		FMemory::Memzero(OutputAudioView.GetData(), NumFramesPerBlock * sizeof(float));
			
		ExecuteSubBlocks();
	}

	void FModalReverbOperator::ExecuteSubBlocks()
	{
		// Parse triggers and render audio
		int32 PlayTrigIndex = 0;
		int32 NextPlayFrame = 0;
		const int32 NumPlayTrigs = PlayTrigger->NumTriggeredInBlock();

		int32 NextAudioFrame = 0;
		int32 CurrAudioFrame = 0;
		const int32 LastAudioFrame = OperatorSettings.GetNumFramesPerBlock() - 1;
		const int32 NoTrigger = OperatorSettings.GetNumFramesPerBlock() << 1; //Same as multiply by 2

		while (NextPlayFrame < LastAudioFrame)
		{
			// get the next Play and Stop indices
			NextPlayFrame = PlayTrigIndex < NumPlayTrigs ? (*PlayTrigger)[PlayTrigIndex] : NoTrigger;   
			NextAudioFrame = FMath::Min(NextPlayFrame, OperatorSettings.GetNumFramesPerBlock());
				
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
		}
	}

	void FModalReverbOperator::InitSynthesizers()
	{
		ModalReverb.Reset();
		FModalReverbParams ReverbParams = FModalReverbParams(*FreqScatterScale, *PhaseScatterScale,
															 *DecayMin, *DecaySlopeB, *DecaySlopeC);

		InitRoomSize = *RoomSize;
		InitAbsorption = *Absorption;
		InitOpenRoomFactor = *OpenRoomFactor;
		
		ModalReverb = MakeUnique<FModalReverb>(SamplingRate, ReverbParams, *NumModalsLow, *NumModalsMid, *NumModalsHigh,
											   InitAbsorption, InitRoomSize, InitOpenRoomFactor, *AmpScale);
		bIsPlaying = ModalReverb.IsValid();
	}

	void FModalReverbOperator::RenderFrameRange(int32 StartFrame, int32 EndFrame)
	{
		if(!bIsPlaying)
			return;
			
		const int32 NumFramesToGenerate = EndFrame - StartFrame;
		if (NumFramesToGenerate <= 0)
		{
			UE_LOG(LogImpactSFXSynth, Warning, TEXT("ModalReverbNodes::RenderFrameRange: StartFrame = %d and EndFrame = %d are invalid!"), StartFrame, EndFrame);
			return;
		}

		if(*bUpdateParams)
		{
			InitRoomSize = *RoomSize;
			InitAbsorption = *Absorption;
			InitOpenRoomFactor = *OpenRoomFactor;
		}

		TArrayView<float>  BufferToGenerate = OutputAudioView.Slice(StartFrame, NumFramesToGenerate);
		const bool bIsStop = ModalReverb->CreateReverb(BufferToGenerate, InAudioView,
													InitAbsorption, InitRoomSize, InitOpenRoomFactor,
		 										  *AmpScale, *bInAudioStop);
			
		if(bIsStop)
		{
			ModalReverb.Reset();
			bIsPlaying = false;
			TriggerOnDone->TriggerFrame(EndFrame);
		}
	}

	const FVertexInterface& FModalReverbOperator::GetVertexInterface()
	{
		using namespace ModalReverbVertexNames;
		
		static const FVertexInterface Interface(
			FInputVertexInterface(
				
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerPlay)),
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAudio)),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsInAudioStop), false),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsUpdateParams), false),
				TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModalsLow), 32),
				TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModalsMid), 128),
				TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumModalsHigh), 96),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputFreqScatterScale), 0.05f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPhaseScatterScale), 0.02f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecayMin), 2.f),

				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecaySlopeB), 0.02f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDecaySlopeC), 0.23f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRoomSize), 1.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAbsorption), 2.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputOpenRoomFactor), 1.f),
		
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAmpScale), 1.f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudio))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FModalReverbOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Modal Reverb"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("Metasound_ModalReverbDisplayName", "Modal Reverb");
			Info.Description = METASOUND_LOCTEXT("Metasound_ModalReverbNodeDescription", "Create reverb by convolving modals with an input audio.");
			Info.Author = PluginAuthor;
			Info.PromptIfMissing = PluginNodeMissingPrompt;
			Info.DefaultInterface = GetVertexInterface();
			Info.CategoryHierarchy.Emplace(NodeCategories::Spatialization);
			Info.Keywords = { };
			return Info;
		};

		static const FNodeClassMetadata Info = InitNodeInfo();

		return Info;
	}

	TUniquePtr<IOperator> FModalReverbOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace ModalReverbVertexNames;
		
		FTriggerReadRef InTriggerPlay = InputData.GetOrConstructDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerPlay), InParams.OperatorSettings);
		FAudioBufferReadRef InAudio = InputData.GetOrConstructDataReadReference<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputAudio), InParams.OperatorSettings);
		FBoolReadRef InIsInAudioStop = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsInAudioStop), InParams.OperatorSettings);
		FBoolReadRef InUpdateParams = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsUpdateParams), InParams.OperatorSettings);

		FInt32ReadRef InNumModalsLow = InputData.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModalsLow), InParams.OperatorSettings);
		FInt32ReadRef InNumModalsMid = InputData.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModalsMid), InParams.OperatorSettings);
		FInt32ReadRef InNumModalsHigh = InputData.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumModalsHigh), InParams.OperatorSettings);
		FFloatReadRef InFreqScatterScale = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputFreqScatterScale), InParams.OperatorSettings);
		FFloatReadRef InPhaseScatterScale = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputPhaseScatterScale), InParams.OperatorSettings);
		FFloatReadRef InDecayMin = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecayMin), InParams.OperatorSettings);
		
		FFloatReadRef InDecaySlopeB = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecaySlopeB), InParams.OperatorSettings);
		FFloatReadRef InDecaySlopeC = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputDecaySlopeC), InParams.OperatorSettings);
		FFloatReadRef InRoomSize = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputRoomSize), InParams.OperatorSettings);
		FFloatReadRef InAbsorption = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAbsorption), InParams.OperatorSettings);
		FFloatReadRef InOpenRoomDecayScale = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputOpenRoomFactor), InParams.OperatorSettings);
		
		FFloatReadRef InAmpScale = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAmpScale), InParams.OperatorSettings);

		return MakeUnique<FModalReverbOperator>(InParams.OperatorSettings, InTriggerPlay, InAudio, InIsInAudioStop, InUpdateParams,
													InNumModalsLow, InNumModalsMid, InNumModalsHigh,
													InFreqScatterScale, InPhaseScatterScale, 
													InDecayMin, InDecaySlopeB, InDecaySlopeC, InRoomSize, InAbsorption, InOpenRoomDecayScale,
													InAmpScale);
	}

	class FModalReverbNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FModalReverbNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FModalReverbOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FModalReverbNode)
}

#undef LOCTEXT_NAMESPACE
