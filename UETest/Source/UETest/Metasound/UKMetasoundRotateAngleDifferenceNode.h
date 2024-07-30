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

#define LOCTEXT_NAMESPACE "UKMetasoundRotateAngleDifferenceNode"
namespace Metasound
{
	namespace UKDeltaDegreeVertexNames
	{
		METASOUND_PARAM(InputAngle, "Angle", "0 ~ 360")
		METASOUND_PARAM(InputDeltaAngle, "DeltaAngle", "Delta Angle")
		METASOUND_PARAM(OutputAngle, "Out Angle", "Out Angle")
	}

	class FUKRotateAngleDifferenceOperator : public TExecutableOperator<FUKRotateAngleDifferenceOperator>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

		FUKRotateAngleDifferenceOperator(const FOperatorSettings& InSettings, const FFloatReadRef& InAngle, const FFloatReadRef& InDeltaAngle);

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;
		void Reset(const IOperator::FResetParams& InParams);
		void Execute();

	private:
		const float GetRotateTheAngleByDifference(const float ParamAngle, const float ParamDelta);

		FFloatReadRef Angle;
		FFloatReadRef DeltaAngle;
		FFloatWriteRef AngleOutput;
	};
}
#undef LOCTEXT_NAMESPACE
