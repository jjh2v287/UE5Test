// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKWavePlayNode.h"
#include "Internationalization/Text.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesNames.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/Dsp.h"
#include "MetasoundFacade.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundVertex.h"
#include "DSP/FloatArrayMath.h"

#define LOCTEXT_NAMESPACE "UKWavePlayNode"
namespace Metasound
{
	FUKWavePlayOperator::FUKWavePlayOperator(const FOperatorSettings& InSettings, const FAudioBufferReadRef& InAudio, const FWaveAssetReadRef& InWaveAsset, const FBoolReadRef& InLoop)
		: AudioInput(InAudio)
		, WaveAsset(InWaveAsset)
		, LoopInput(InLoop)
		, OperatorSettings(InSettings)
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
	{
		UKOnsetDetection = MakeUnique<UUKOnsetDetection>();
		UKOnsetDetection->UpdateFrameSize(InSettings.GetNumFramesPerBlock());
	}

	void FUKWavePlayOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKWavePlayOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAudio), AudioInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputWaveAsset), WaveAsset);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputLoop), LoopInput);
	}

	void FUKWavePlayOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKWavePlayOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioMono), AudioOutput);
	}

	FDataReferenceCollection FUKWavePlayOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FUKWavePlayOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	void FUKWavePlayOperator::Reset(const IOperator::FResetParams& InParams)
	{
		AudioOutput->Zero();
		UKOnsetDetection.Reset();
	}

	void FUKWavePlayOperator::Execute()
	{
		// OperatorSettings.GetNumFramesPerBlock();
		// TArrayView<const float> InAudioView = TArrayView<const float>(AudioInput->GetData(), AudioInput->Num());
		// float dddd[] = {1, 0, -1, 0, 1, -1, 0.5, 0, -1, 0.5}; // Sample input
		// Audio::FAlignedFloatBuffer ComplexSpectrum2;
		// Audio::FFFTSettings FFTSettings;
		// FFTSettings.Log2Size = Audio::CeilLog2(10 * 2);
		// FFTSettings.bArrays128BitAligned = true;
		// FFTSettings.bEnableHardwareAcceleration = true;
		// TUniquePtr<Audio::IFFTAlgorithm> FFT2 = Audio::FFFTFactory::NewFFTAlgorithm(FFTSettings);
		// ComplexSpectrum2.AddUninitialized(FFT2->NumOutputFloats());
		// FFT2->ForwardRealToComplex(AudioInput->GetData(), ComplexSpectrum2.GetData());
		
		// Onset 검출 로직
		float OnsetThreshold = 0.0f; // 설정 가능한 임계값
		const TArray<float> Signal = TArray<float>(AudioInput->GetData(), AudioInput->Num());
		TArray<float> FFTReal;
		TArray<float> FFTImaginary;
		UKOnsetDetection->ComputeFFT(Signal, FFTReal, FFTImaginary);
		float ComplexSpectralDifference = UKOnsetDetection->GetComplexSpectralDifference(FFTReal, FFTImaginary);
		if (ComplexSpectralDifference > OnsetThreshold)
		{
			UE_LOG(LogTemp, Log, TEXT("Onset detected at block %f"), ComplexSpectralDifference);
		}
	}

	const FVertexInterface& FUKWavePlayOperator::GetVertexInterface()
	{
		using namespace UKWavePlayOperatorNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAudio)),
				TInputDataVertex<FWaveAsset>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputWaveAsset)),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputLoop), true)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioMono))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FUKWavePlayOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { Metasound::StandardNodes::Namespace, TEXT("UKWavePlay"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("UKWavePlay_DisplayName", "UKWavePlay");
			Info.Description = METASOUND_LOCTEXT("UKWavePlay_Description", "");
			Info.Author = PluginAuthor;
			Info.PromptIfMissing = PluginNodeMissingPrompt;
			Info.DefaultInterface = GetVertexInterface();
			Info.CategoryHierarchy.Emplace(Metasound::NodeCategories::Math);
			Info.Keywords = { METASOUND_LOCTEXT("DeltaDegreeKeyword", "Binaural"), METASOUND_LOCTEXT("DeltaDegreeKeyword", "")};
			return Info;
		};

		static const FNodeClassMetadata Info = InitNodeInfo();

		return Info;
	}

	TUniquePtr<IOperator> FUKWavePlayOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace UKWavePlayOperatorNames;

		const FAudioBufferReadRef InputAudioRef = InputData.GetOrCreateDefaultDataReadReference<Metasound::FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputAudio), InParams.OperatorSettings);
		const FWaveAssetReadRef InputWaveAssetRef = InputData.GetOrCreateDefaultDataReadReference<Metasound::FWaveAsset>(METASOUND_GET_PARAM_NAME(InputWaveAsset), InParams.OperatorSettings);
		const FBoolReadRef HeadLoopRef = InputData.GetOrCreateDefaultDataReadReference<bool>(METASOUND_GET_PARAM_NAME(InputLoop), InParams.OperatorSettings);

		return MakeUnique<FUKWavePlayOperator>(InParams.OperatorSettings, InputAudioRef, InputWaveAssetRef, HeadLoopRef);
	}

	class FUKWavePlayNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FUKWavePlayNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FUKWavePlayOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FUKWavePlayNode)
}
#undef LOCTEXT_NAMESPACE