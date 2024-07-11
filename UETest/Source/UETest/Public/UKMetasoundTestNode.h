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

#define LOCTEXT_NAMESPACE "UKMetasoundTestNode"
namespace Metasound
{
	namespace UKTestOperatorNames
	{
		METASOUND_PARAM(InputAudio, "Input Audio", "0 ~ 360")
		METASOUND_PARAM(InputTest, "Input Test", "0 ~ 360")
		METASOUND_PARAM(OutputAudio, "Output Audio", "Out Angle")
	}

	class FUKTestOperator : public TExecutableOperator<FUKTestOperator>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

		FUKTestOperator(const FOperatorSettings& InSettings, const FAudioBufferReadRef& InAudio, const FFloatReadRef& InTest);

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;
		void Reset(const IOperator::FResetParams& InParams);
		void Execute();

	private:
		void ProcessAudioBuffer(const float* InputBuffer, float* OutputBuffer, const int32 NumSamples);
		int32 NumChannels{ 1 };
		
	private:
		FAudioBufferReadRef AudioInput;
		FFloatReadRef TestInput;
		FAudioBufferWriteRef AudioOutput;
	};
}
#undef LOCTEXT_NAMESPACE
