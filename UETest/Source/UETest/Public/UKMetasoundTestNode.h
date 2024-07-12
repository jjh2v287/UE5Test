// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "MetasoundExecutableOperator.h"
#include "MetasoundOperatorInterface.h"
#include "Internationalization/Text.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/Dsp.h"
#include "MetasoundVertex.h"
#include "OnsetAnalyzer.h"
#include "PeakPicker.h"
#include "DSP/MultichannelBuffer.h"

#define LOCTEXT_NAMESPACE "UKMetasoundTestNode"
namespace Metasound
{
	namespace UKTestOperatorNames
	{
		METASOUND_PARAM(InputAudio, "Input Audio", "0 ~ 360")
		METASOUND_PARAM(HeadRadius, "HeadRadius", "0 ~ 360")
		METASOUND_PARAM(Azimuth, "Azimuth", "0 ~ 360")
		METASOUND_PARAM(Elevation, "Elevation", "0 ~ 360")
		METASOUND_PARAM(OutputAudioLeft, "Output Audio Left", "Out Angle")
		METASOUND_PARAM(OutputAudioRight, "Output Audio Right", "Out Angle")
	}

	class FUKTestOperator : public TExecutableOperator<FUKTestOperator>
	{
		public:

			static const FNodeClassMetadata& GetNodeInfo();
			static const FVertexInterface& GetVertexInterface();
			static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

			FUKTestOperator(const FOperatorSettings& InSettings, const FAudioBufferReadRef& InAudio, const FFloatReadRef& InHeadRadius, const FFloatReadRef& InAzimuth, const FFloatReadRef& InElevation);

			virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
			virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
			virtual FDataReferenceCollection GetInputs() const override;
			virtual FDataReferenceCollection GetOutputs() const override;
			void Reset(const IOperator::FResetParams& InParams);
			void Execute();

		private:
		bool TransferToStereo(float Azimuth, float Elevation, float Gain);
		void ApplySimpleHRTF(const TArrayView<const float>& InAudio, float Azimuth, float Elevation, float Gain, Audio::FMultichannelBufferView& OutAudio);

		private:
			float SamplingRate;
			int32 NumFramesPerBlock;
			int32 MaxDelaySamples;

			TArrayView<const float> InAudioView;
			Audio::FMultichannelBufferView OutputAudioView;

			TUniquePtr<Audio::IFFTAlgorithm> FFT;
			Audio::FAlignedFloatBuffer ComplexSpectrum;
		
			Audio::FOnsetStrengthSettings Settings;
			Audio::FPeakPickerSettings PeakPickerSettings;
			TUniquePtr<Audio::FOnsetStrengthAnalyzer> OnsetAnalyzer;
		private:
			FAudioBufferReadRef AudioInput;
			FFloatReadRef HeadRadiusInput;
			FFloatReadRef AzimuthInput;
			FFloatReadRef ElevationInput;
			FAudioBufferWriteRef AudioLeftOutput;
			FAudioBufferWriteRef AudioRightOutput;
	};
}
#undef LOCTEXT_NAMESPACE
