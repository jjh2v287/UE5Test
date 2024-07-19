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
#include "MetasoundWave.h"
#include "OnsetAnalyzer.h"
#include "PeakPicker.h"
#include "UKOnsetDetection.h"
#include "DSP/MultichannelBuffer.h"

#define LOCTEXT_NAMESPACE "UKWavePlayNode"
namespace Metasound
{
	namespace UKWavePlayOperatorNames
	{
		METASOUND_PARAM(InputAudio, "Input Audio", "0 ~ 360")
		METASOUND_PARAM(InputWaveAsset, "Wave Asset", "The wave asset to be real-time decoded.")
		METASOUND_PARAM(InputLoop, "Loop", "Whether or not to loop between the start and specified end times.")
		METASOUND_PARAM(OutputAudioMono, "Out Mono", "The mono channel audio output.")
	}

	class FUKWavePlayOperator : public TExecutableOperator<FUKWavePlayOperator>
	{
		public:

			static const FNodeClassMetadata& GetNodeInfo();
			static const FVertexInterface& GetVertexInterface();
			static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

			FUKWavePlayOperator(const FOperatorSettings& InSettings, const FAudioBufferReadRef& InAudio, const FWaveAssetReadRef& InWaveAsset, const FBoolReadRef& InLoop);

			virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
			virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
			virtual FDataReferenceCollection GetInputs() const override;
			virtual FDataReferenceCollection GetOutputs() const override;
			void Reset(const IOperator::FResetParams& InParams);
			void Execute();

		private:
			FAudioBufferReadRef AudioInput;
			FWaveAssetReadRef WaveAsset;
			FBoolReadRef LoopInput;
			FAudioBufferWriteRef AudioOutput;
			const FOperatorSettings OperatorSettings;
			TUniquePtr<UUKOnsetDetection> UKOnsetDetection;
	};
}
#undef LOCTEXT_NAMESPACE
