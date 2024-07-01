// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

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
#include "HRTFModal.h"
#include "DSP/FloatArrayMath.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_ModalHRTFNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace ModalHRTFVertexNames
	{
		METASOUND_PARAM(InputIsEnable, "Enable HRTF", "Apply HRFT if true. Otherwise, use stereo panner with equal power law.")
		METASOUND_PARAM(InputIsClamp, "Is Clamp", "Clamp the output inside the range [-1, 1] or not. At gain = 1, the model has already been calibrated to not going out side of this range. Thus, you only need to use this if you set the gain to be higher than 1.")
		METASOUND_PARAM(InputAudio, "In Audio", "The input audio to spatialize.")
		METASOUND_PARAM(InputIsInAudioStop, "Is In Audio Stop", "True if the input audio has finished/stopped. This is used to trigger the Stop output when the output decays to zero.")
		METASOUND_PARAM(InputAzimuth, "Azimuth", "The sound source to listener azimuth in degrees. 0 degrees is in front, -90 degrees is to the left, -180 or 180 degrees are behind, 90 degrees is to the right.")
		METASOUND_PARAM(InputElevation, "Elevation", "The sound source to listener elevation in degrees. 0 degrees is at the ear level. -90 degrees is at bottom of the head. 90 degrees is at the top at the head.")
		METASOUND_PARAM(InputGain, "Gain", "The gain of the output audio. As spatialization changes the total power of the output, you can use this to scale the gain of both channels.")

		METASOUND_PARAM(OutputTriggerOnDone, "On Finished", "Triggers when all channels decays to zero and IsInAudioStop is true.")
		
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "Left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "Right channel audio output.")
	}

	class FModalHRTFOperator : public TExecutableOperator<FModalHRTFOperator>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FModalHRTFOperator(const FOperatorSettings& InSettings,
		                   const FBoolReadRef& InEnable,
		                   const FBoolReadRef& IsClamp,
		                   const FAudioBufferReadRef& InAudioInput,
		                   const FBoolReadRef& InIsInAudioStop, 
		                   const FFloatReadRef& InAzimuth,
		                   const FFloatReadRef& InElevation,
		                   const FFloatReadRef& InGain);

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;
		void Reset(const IOperator::FResetParams& InParams);
		void Execute();

	private:
		float ConvertAzimuthToPan() const;
		void ComputePanGains(float InPan, float& OutLeftGain, float& OutRightGain) const;

		void ApplyHRTF();
		void ApplyStereoPanner();
		
		void OnFinish();
		
	private:
		FBoolReadRef bEnable;
		FBoolReadRef bClamp;
		FAudioBufferReadRef AudioInput;
		FBoolReadRef bInAudioStop;
		FFloatReadRef Azimuth;
		FFloatReadRef Elevation;
		FFloatReadRef Gain;

		FTriggerWriteRef TriggerOnDone;
		FAudioBufferWriteRef AudioLeftOutput;
		FAudioBufferWriteRef AudioRightOutput;

		float SamplingRate;
		int32 NumFramesPerBlock;

		TArrayView<const float> InAudioView;
		Audio::FMultichannelBufferView OutputAudioView;
		TUniquePtr<FHRTFModal> HRTFModal;
		bool bIsPlaying;
		bool bFinish;

		float PrevPan;
		float PrevPanLeft;
		float PrevPanRight;
	};

	FModalHRTFOperator::FModalHRTFOperator(const FOperatorSettings& InSettings,
	                                       const FBoolReadRef& InEnable,
	                                       const FBoolReadRef& InIsClamp,
	                                       const FAudioBufferReadRef& InAudioInput,
	                                       const FBoolReadRef& InIsInAudioStop, 
	                                       const FFloatReadRef& InAzimuth,
	                                       const FFloatReadRef& InElevation,
	                                       const FFloatReadRef& InGain)
		: bEnable(InEnable)
		, bClamp(InIsClamp)
		, AudioInput(InAudioInput)
		, bInAudioStop(InIsInAudioStop)
		, Azimuth(InAzimuth)
		, Elevation(InElevation)
		, Gain(InGain)
		, TriggerOnDone(FTriggerWriteRef::CreateNew(InSettings))
		, AudioLeftOutput(FAudioBufferWriteRef::CreateNew(InSettings))
		, AudioRightOutput(FAudioBufferWriteRef::CreateNew(InSettings))
	{
		SamplingRate = InSettings.GetSampleRate();
		NumFramesPerBlock = InSettings.GetNumFramesPerBlock();
		
		// Hold on to a view of the output audio. Audio buffers are only writable
		// by this object and will not be reallocated.
		OutputAudioView.Empty(2);
		OutputAudioView.Emplace(AudioLeftOutput->GetData(), AudioLeftOutput->Num());
		OutputAudioView.Emplace(AudioRightOutput->GetData(), AudioRightOutput->Num());

		InAudioView = TArrayView<const float>(AudioInput->GetData(), AudioInput->Num());
		
		bIsPlaying = false;
		bFinish = false;
		
		PrevPan = ConvertAzimuthToPan();
		ComputePanGains(PrevPan, PrevPanLeft, PrevPanRight);
	}

	float FModalHRTFOperator::ConvertAzimuthToPan() const
	{
		float CurrentAzimuth = *Azimuth;
		if(CurrentAzimuth< -90.f)
			CurrentAzimuth = -180.f - CurrentAzimuth;
		else if(CurrentAzimuth > 90.f)
			CurrentAzimuth = 180.f - CurrentAzimuth;
		return FMath::Clamp(CurrentAzimuth / 90.f, -1.0f, 1.0f);
	}

	void FModalHRTFOperator::ComputePanGains(float InPan, float& OutLeftGain, float& OutRightGain) const
	{
		// Convert [-1.0, 1.0] to [0.0, 1.0]
		const float Fraction = 0.5f * (InPan + 1.0f);
		// Compute the left and right amount with one math call
		FMath::SinCos(&OutRightGain, &OutLeftGain, 0.5f * PI * Fraction);

		//Reduce gain by half to have the same power as when using HRTF
		OutLeftGain = OutLeftGain / 2.0f;
		OutRightGain = OutRightGain / 2.0f;
	}

	void FModalHRTFOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace ModalHRTFVertexNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsEnable), bEnable);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsClamp), bClamp);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAudio), AudioInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputIsInAudioStop), bInAudioStop);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAzimuth), Azimuth);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputElevation), Elevation);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputGain), Gain);
	}

	void FModalHRTFOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace ModalHRTFVertexNames;
		
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnDone), TriggerOnDone);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioLeft), AudioLeftOutput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioRight), AudioRightOutput);
	}

	FDataReferenceCollection FModalHRTFOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FModalHRTFOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}
	
	void FModalHRTFOperator::Reset(const IOperator::FResetParams& InParams)
	{
		TriggerOnDone->Reset();

		AudioLeftOutput->Zero();
		AudioRightOutput->Zero();

		HRTFModal.Reset();
			
		bIsPlaying = false;
		bFinish = false;

		PrevPan = ConvertAzimuthToPan();
		ComputePanGains(PrevPan, PrevPanLeft, PrevPanRight);
	}

	void FModalHRTFOperator::Execute()
	{
		//Always clear output to zero
		for (const TArrayView<float>& OutputBuffer : OutputAudioView)
			FMemory::Memzero(OutputBuffer.GetData(), NumFramesPerBlock * sizeof(float));

		if(bFinish)
			return;
		
		if(*bEnable)
			ApplyHRTF();		
		else
		{
			ApplyStereoPanner();
		}
	}

	void FModalHRTFOperator::ApplyHRTF()
	{
		if(!bIsPlaying)
		{
			HRTFModal = MakeUnique<FHRTFModal>(SamplingRate,  NumFramesPerBlock);
			bIsPlaying = true;
		}
		
		bFinish = true;
		if(HRTFModal.IsValid())
		{
			bFinish = HRTFModal->TransferToStereo(InAudioView,
												  *Azimuth,
												  *Elevation,
												  *Gain,
												  OutputAudioView,
												  *bInAudioStop,
												  *bClamp);
		}
		
		if(bFinish)
			OnFinish();
	}
	
	void FModalHRTFOperator::ApplyStereoPanner()
	{
		if(*bInAudioStop)
		{
			bFinish = true;
			OnFinish();
			return;
		}
		
		float CurrentPanningAmount = ConvertAzimuthToPan();

		const float* InputBufferPtr = AudioInput->GetData();
		int32 InputSampleCount = AudioInput->Num();
		float* OutputLeftBufferPtr = AudioLeftOutput->GetData();
		float* OutputRightBufferPtr = AudioRightOutput->GetData();

		TArrayView<const float> InputBufferView(AudioInput->GetData(), InputSampleCount);
		TArrayView<float> OutputLeftBufferView(AudioLeftOutput->GetData(), InputSampleCount);
		TArrayView<float> OutputRightBufferView(AudioRightOutput->GetData(), InputSampleCount);

		if (FMath::IsNearlyEqual(PrevPan, CurrentPanningAmount))
		{
			Audio::ArrayMultiplyByConstant(InputBufferView, PrevPanLeft, OutputLeftBufferView);
			Audio::ArrayMultiplyByConstant(InputBufferView, PrevPanRight, OutputRightBufferView);
		}
		else 
		{
			// The pan amount has changed so recompute it
			float CurrentLeftPan;
			float CurrentRightPan;
			ComputePanGains(CurrentPanningAmount, CurrentLeftPan, CurrentRightPan);

			// Copy the input to the output buffers
			FMemory::Memcpy(OutputLeftBufferPtr, InputBufferPtr, InputSampleCount * sizeof(float));
			FMemory::Memcpy(OutputRightBufferPtr, InputBufferPtr, InputSampleCount * sizeof(float));

			// Do a fast fade on the buffers from the prev left/right gains to current left/right gains
			Audio::ArrayFade(OutputLeftBufferView, PrevPanLeft, CurrentLeftPan);
			Audio::ArrayFade(OutputRightBufferView, PrevPanRight, CurrentRightPan);
			
			PrevPan = CurrentPanningAmount;
			PrevPanLeft = CurrentLeftPan;
			PrevPanRight = CurrentRightPan;
		}
	}

	void FModalHRTFOperator::OnFinish()
	{
		HRTFModal.Reset();			
		TriggerOnDone->TriggerFrame(NumFramesPerBlock - 1);
	}
	
	const FVertexInterface& FModalHRTFOperator::GetVertexInterface()
	{
		using namespace ModalHRTFVertexNames;
		
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsEnable), true),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsClamp), false),
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAudio)),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputIsInAudioStop), false),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAzimuth), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputElevation), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputGain), 1.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnDone)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FModalHRTFOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { ImpactSFXSynthEngineNodes::Namespace, TEXT("Modal HRTF"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("Metasound_ModalHRTFDisplayName", "Modal HRTF");
			Info.Description = METASOUND_LOCTEXT("Metasound_ModalHRTFNodeDescription", "Apply HRTF on an input audio signal by using modal approximation.");
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

	TUniquePtr<IOperator> FModalHRTFOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace ModalHRTFVertexNames;		
		FBoolReadRef InIsEnable = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsEnable), InParams.OperatorSettings);
		FBoolReadRef InIsClamp = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsClamp), InParams.OperatorSettings);
		FAudioBufferReadRef InAudio = InputData.GetOrConstructDataReadReference<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputAudio), InParams.OperatorSettings);
		FBoolReadRef InIsInAudioStop = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputIsInAudioStop), InParams.OperatorSettings);
		FFloatReadRef InAzimuth = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputAzimuth), InParams.OperatorSettings);
		FFloatReadRef InElevation = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputElevation), InParams.OperatorSettings);
		FFloatReadRef InGain = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputGain), InParams.OperatorSettings);

		return MakeUnique<FModalHRTFOperator>(InParams.OperatorSettings, InIsEnable, InIsClamp, InAudio, InIsInAudioStop, InAzimuth, InElevation, InGain);
	}

	class FModalHRTFNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FModalHRTFNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FModalHRTFOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FModalHRTFNode)
}

#undef LOCTEXT_NAMESPACE
