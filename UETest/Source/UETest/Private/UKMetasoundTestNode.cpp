// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKMetasoundTestNode.h"
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

#define LOCTEXT_NAMESPACE "UKMetasoundTestNode"
namespace Metasound
{
	FUKTestOperator::FUKTestOperator(const FOperatorSettings& InSettings, const FAudioBufferReadRef& InAudio, const FFloatReadRef& InHeadRadius, const FFloatReadRef& InAzimuth, const FFloatReadRef& InElevation)
		: AudioInput(InAudio)
		, HeadRadiusInput(InHeadRadius)
		, AzimuthInput(InAzimuth)
		, ElevationInput(InElevation)
		, AudioLeftOutput(FAudioBufferWriteRef::CreateNew(InSettings))
		, AudioRightOutput(FAudioBufferWriteRef::CreateNew(InSettings))
	{
		SamplingRate = InSettings.GetSampleRate();
		NumFramesPerBlock = InSettings.GetNumFramesPerBlock();
		InAudioView = TArrayView<const float>(AudioInput->GetData(), AudioInput->Num());
		OutputAudioView.Empty(2);
		OutputAudioView.Emplace(AudioLeftOutput->GetData(), AudioLeftOutput->Num());
		OutputAudioView.Emplace(AudioRightOutput->GetData(), AudioRightOutput->Num());

		// 설정 및 샘플 레이트 정의
		const int32 ddd = 512;		
		Settings.NumWindowFrames = ddd;
		Settings.NumHopFrames = ddd / 2;
		Settings.FFTSize = ddd * 2;
		Settings.ComparisonLag = 1;
		Settings.MelSettings.NumBands = 40; // 멜 밴드 수를 유효 주파수 빈 수보다 작게 설정
		Settings.WindowType = Audio::EWindowType::Hamming;
		OnsetAnalyzer = MakeUnique<Audio::FOnsetStrengthAnalyzer>(Settings, SamplingRate);

		Audio::FFFTSettings FFTSettings;
		FFTSettings.Log2Size = Audio::CeilLog2(NumFramesPerBlock * 2);
		FFTSettings.bArrays128BitAligned = true;
		FFTSettings.bEnableHardwareAcceleration = true;
		FFT = Audio::FFFTFactory::NewFFTAlgorithm(FFTSettings);
		ComplexSpectrum.AddUninitialized(FFT->NumOutputFloats());
	}

	void FUKTestOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKTestOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputAudio), AudioInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(HeadRadius), HeadRadiusInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(Azimuth), AzimuthInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(Elevation), ElevationInput);
	}

	void FUKTestOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace UKTestOperatorNames;

		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioLeft), AudioLeftOutput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputAudioRight), AudioRightOutput);
	}

	FDataReferenceCollection FUKTestOperator::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	FDataReferenceCollection FUKTestOperator::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed.
		checkNoEntry();
		return {};
	}

	void FUKTestOperator::Reset(const IOperator::FResetParams& InParams)
	{
		AudioLeftOutput->Zero();
		AudioRightOutput->Zero();
		OnsetAnalyzer->Reset();
	}

	void FUKTestOperator::Execute()
	{
		InAudioView = TArrayView<const float>(AudioInput->GetData(), AudioInput->Num());

		FFT->ForwardRealToComplex(AudioInput->GetData(), ComplexSpectrum.GetData());
		
		// TransferToStereo(*AzimuthInput, *ElevationInput, 1.0f);
		
		// 여기에 오디오 데이터 스트림을 가져오는 코드를 추가합니다
		// 예를 들어, 파일에서 읽거나 실시간 오디오 스트림을 처리합니다.
		// InAudioView;
		// 오디오 데이터 가져오기 (여기서는 예제 데이터를 사용합니다)
    
		// 온셋 강도를 계산합니다.
		TArray<float> OutEnvelopeStrengths;
		OnsetAnalyzer->CalculateOnsetStrengths(InAudioView, OutEnvelopeStrengths);

		// 온셋 인덱스를 추출합니다.
		// TArray<int32> OutOnsetIndices;
		// Audio::OnsetExtractIndices(PeakPickerSettings, OutEnvelopeStrengths, OutOnsetIndices);
		//
		// // 온셋 인덱스를 기반으로 타임스탬프를 계산합니다.
		// for (int32 OnsetIndex : OutOnsetIndices)
		// {
		// 	float Timestamp = Audio::FOnsetStrengthAnalyzer::GetTimestampForIndex(Settings, SamplingRate, OnsetIndex);
		// }
	}

	bool FUKTestOperator::TransferToStereo(float Azimuth, float Elevation, float Gain)
	{
		if (OutputAudioView.Num() != 2)
		{
			return true;
		}

		const int32 NumOutputFrames = Audio::GetMultichannelBufferNumFrames(OutputAudioView);
		const int32 NumAudioFrame = InAudioView.Num();
		if (NumAudioFrame != NumOutputFrames || NumOutputFrames != NumFramesPerBlock)
		{
			return true;
		}

		const float TempHeadRadius = FMath::Clamp(*HeadRadiusInput, 0.01f, 1.0f);
		MaxDelaySamples = FMath::CeilToInt32(0.001f * TempHeadRadius * SamplingRate) + 1;

		Azimuth = FMath::Clamp(Azimuth, -180.f, 180.f);
		Elevation = FMath::Clamp(Elevation, -90.f, 90.f);

		ApplySimpleHRTF(InAudioView, Azimuth, Elevation, Gain, OutputAudioView);

		for (int32 Channel = 0; Channel < 2; Channel++)
		{
			Audio::ArrayClampInPlace(OutputAudioView[Channel], -1.f, 1.f);
		}

		return false;
	}

	void FUKTestOperator::ApplySimpleHRTF(const TArrayView<const float>& InAudio, float Azimuth, float Elevation, float Gain, Audio::FMultichannelBufferView& OutAudio)
	{
		float LeftGain = 1.0f;
		float RightGain = 1.0f;
		float ElevationGain = 1.0f;

		// Simple panning based on Azimuth
		if (Azimuth < 0)
		{
			RightGain = 1.0f + Azimuth / 180.0f;
		}
		else
		{
			LeftGain = 1.0f - Azimuth / 180.0f;
		}

		// Simple elevation effect
		ElevationGain = 1.0f - FMath::Abs(Elevation) / 90.0f;

		// Interaural Time Difference (ITD)
		int32 LeftDelaySamples = 0;
		int32 RightDelaySamples = 0;
		if (Azimuth < 0)
		{
			RightDelaySamples = FMath::RoundToInt32((-Azimuth / 180.0f) * MaxDelaySamples);
		}
		else
		{
			LeftDelaySamples = FMath::RoundToInt32((Azimuth / 180.0f) * MaxDelaySamples);
		}

		for (int32 Frame = 0; Frame < NumFramesPerBlock; Frame++)
		{
			// float LeftSample = (Frame >= LeftDelaySamples) ? InAudio[Frame - LeftDelaySamples] * LeftGain * ElevationGain * Gain : InAudio[Frame] /*0.0f*/;
			// float RightSample = (Frame >= RightDelaySamples) ? InAudio[Frame - RightDelaySamples] * RightGain * ElevationGain * Gain : InAudio[Frame]/*0.0f*/;
			float LeftSample = InAudio[Frame] * LeftGain * ElevationGain * Gain;
			float RightSample = InAudio[Frame] * RightGain * ElevationGain * Gain;

			OutAudio[0][Frame] = 0.05f;
			OutAudio[1][Frame] = 0.05f;
		}
	}

	const FVertexInterface& FUKTestOperator::GetVertexInterface()
	{
		using namespace UKTestOperatorNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputAudio)),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(HeadRadius), 1.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Azimuth), 1.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Elevation), 1.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
			)
		);

		return Interface;
	}

	const FNodeClassMetadata& FUKTestOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { Metasound::StandardNodes::Namespace, TEXT("UK Test"), TEXT("") };
			Info.MajorVersion = 1;
			Info.MinorVersion = 0;
			Info.DisplayName = METASOUND_LOCTEXT("UKTest_DisplayName", "UK Test");
			Info.Description = METASOUND_LOCTEXT("UKTest_Description", "");
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

	TUniquePtr<IOperator> FUKTestOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		
		const FInputVertexInterfaceData& InputData = InParams.InputData;

		using namespace UKTestOperatorNames;

		const FAudioBufferReadRef InputAudioRef = InputData.GetOrCreateDefaultDataReadReference<Metasound::FAudioBuffer>(METASOUND_GET_PARAM_NAME(InputAudio), InParams.OperatorSettings);
		const FFloatReadRef HeadRadiusRef = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(HeadRadius), InParams.OperatorSettings);
		const FFloatReadRef AzimuthRef = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(Azimuth), InParams.OperatorSettings);
		const FFloatReadRef ElevationRef = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(Elevation), InParams.OperatorSettings);

		return MakeUnique<FUKTestOperator>(InParams.OperatorSettings, InputAudioRef, HeadRadiusRef, AzimuthRef, ElevationRef);
	}

	class FUKTestNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FUKTestNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FUKTestOperator>())
		{
		}
	};

	METASOUND_REGISTER_NODE(FUKTestNode)
}
#undef LOCTEXT_NAMESPACE