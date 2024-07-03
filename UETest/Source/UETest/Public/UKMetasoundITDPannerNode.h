// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "MetasoundExecutableOperator.h"
#include "MetasoundOperatorInterface.h"
#include "Internationalization/Text.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundAudioBuffer.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/Delay.h"
#include "DSP/Dsp.h"
#include "MetasoundVertex.h"

#define LOCTEXT_NAMESPACE "UKMetasoundStandardNodes_ITDPannerNode"
namespace Metasound
{
	namespace UKITDPannerVertexNames
	{
		METASOUND_PARAM(InputAudio, "In", "The input audio to spatialize.")
		METASOUND_PARAM(InputPanAngle, "Angle", "The sound source angle in degrees. 90 degrees is in front, 0 degrees is to the right, 270 degrees is behind, 180 degrees is to the left.")
		METASOUND_PARAM(InputDistanceFactor, "Distance Factor", "The normalized distance factor (0.0 to 1.0) to use for ILD (Inter-aural level difference) calculations. 0.0 is near, 1.0 is far. The further away something is the less there is a difference in levels (gain) between the ears.")
		METASOUND_PARAM(InputHeadWidth, "Head Width", "The width of the listener head to use for ITD calculations in centimeters.")
		METASOUND_PARAM(OutputAudioLeft, "Out Left", "Left channel audio output.")
		METASOUND_PARAM(OutputAudioRight, "Out Right", "Right channel audio output.")
	}

	class FUKITDPannerOperator : public TExecutableOperator<FUKITDPannerOperator>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

		FUKITDPannerOperator(const FOperatorSettings& InSettings,
			const FAudioBufferReadRef& InAudioInput, 
			const FFloatReadRef& InPanningAngle,
			const FFloatReadRef& InDistanceFactor,
			const FFloatReadRef& InHeadWidth);

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;
		void Reset(const IOperator::FResetParams& InParams);
		void Execute();

	private:
		void UpdateParams(bool bIsInit);
		const float Get90DegreeRotation(const float Angle);

		FAudioBufferReadRef AudioInput;
		FFloatReadRef PanningAngle;
		FFloatReadRef DistanceFactor;
		FFloatReadRef HeadWidth;

		FAudioBufferWriteRef AudioLeftOutput;
		FAudioBufferWriteRef AudioRightOutput;

		float CurrAngle = 0.0f;
		float CurrX = 0.0f;
		float CurrY = 0.0f;
		float CurrDistanceFactor = 0.0f;
		float CurrHeadWidth = 0.0f;
		float CurrLeftGain = 0.0f;
		float CurrRightGain = 0.0f;
		float CurrLeftDelay = 0.0f;
		float CurrRightDelay = 0.0f;

		float PrevLeftGain = 0.0f;
		float PrevRightGain = 0.0f;

		Audio::FDelay LeftDelay;
		Audio::FDelay RightDelay;
	};
}
#undef LOCTEXT_NAMESPACE
