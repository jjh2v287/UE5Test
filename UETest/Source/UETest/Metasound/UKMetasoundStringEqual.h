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

#define LOCTEXT_NAMESPACE "UKMetasoundStringEqualNode"
namespace Metasound
{
	namespace UKStringEqualOperatorNames
	{
		METASOUND_PARAM(InTrigger, "InTrigger", "InTrigger")
		METASOUND_PARAM(InStringA, "String A", "String A")
		METASOUND_PARAM(InStringB, "String B", "String B")
		METASOUND_PARAM(OutTriggerTrue, "Trigger True", "Trigger True")
		METASOUND_PARAM(OutTriggerFalse, "Trigger False", "Trigger False")
	}

	class FUKStringEqualNode : public TExecutableOperator<FUKStringEqualNode>
	{
		public:

			static const FNodeClassMetadata& GetNodeInfo();
			static const FVertexInterface& GetVertexInterface();
			static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

			FUKStringEqualNode(const FOperatorSettings& InSettings, const FTriggerReadRef& InTrigger, const FStringReadRef& InStringA, const FStringReadRef& InStringB);

			virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
			virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
			virtual FDataReferenceCollection GetInputs() const override;
			virtual FDataReferenceCollection GetOutputs() const override;
			void Reset(const IOperator::FResetParams& InParams);
			void Execute();

		private:
			FTriggerReadRef TriggerIn;
			FStringReadRef StringAIn;
			FStringReadRef StringBIn;
		
			FTriggerWriteRef TriggerTrueOut;
			FTriggerWriteRef TriggerFalseOut;
	};
}
#undef LOCTEXT_NAMESPACE
