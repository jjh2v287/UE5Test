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

#define LOCTEXT_NAMESPACE "UKMetasoundContentDataNode"
namespace Metasound
{
	namespace UKContentDataOperatorNames
	{
		METASOUND_PARAM(OutputTODTime, "TOD Time", "TOD Time")
	}

	class FUKContentDataNode : public TExecutableOperator<FUKContentDataNode>
	{
		public:

			static const FNodeClassMetadata& GetNodeInfo();
			static const FVertexInterface& GetVertexInterface();
			static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

			FUKContentDataNode(const FOperatorSettings& InSettings);

			virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
			virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
			virtual FDataReferenceCollection GetInputs() const override;
			virtual FDataReferenceCollection GetOutputs() const override;
			void Reset(const IOperator::FResetParams& InParams);
			void Execute();

		private:
			FFloatWriteRef TODTimeOutput;
	};
}
#undef LOCTEXT_NAMESPACE
