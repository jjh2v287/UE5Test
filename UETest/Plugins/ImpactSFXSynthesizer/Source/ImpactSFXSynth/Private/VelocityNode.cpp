// Copyright 2023-2024, Le Binh Son, All Rights Reserved.

#pragma once

#include "ImpactSynthEngineNodesName.h"
#include "Runtime/Launch/Resources/Version.h"
#include "DSP/Dsp.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundImpactModalObj.h"
#include "MetasoundParamHelper.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundVertex.h"
#include "ModalSynth.h"
#include "ImpactSFXSynth/Public/Utils.h"

#define LOCTEXT_NAMESPACE "LBSImpactSFXSynthNodes_VelocityNode"

namespace LBSImpactSFXSynth
{
	using namespace Metasound;
	
	namespace VelocityVertexNames
	{
		METASOUND_PARAM(InitValue, "Init Value", "The initial value.")
		METASOUND_PARAM(InputValue, "In Value", "The value which is changed per block rate.")
		
		METASOUND_PARAM(OutVelocity, "Output Velocity", "The output value. Return a positive value if the input is increasing. Otherwise, negative or zero.");
	}

	struct FVelocityOpArgs
	{
		FOperatorSettings OperatorSettings;

		FFloatReadRef InitValue;
		FFloatReadRef InValue;
	};
	
	class FVelocityOperator : public TExecutableOperator<FVelocityOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);
		
		FVelocityOperator(const FVelocityOpArgs& InArgs)
			: OperatorSettings(InArgs.OperatorSettings)
			, InitValue(InArgs.InitValue)
			, InValue(InArgs.InValue)
			, OutVelocity(FFloatWriteRef::CreateNew(0.f))
		{
			const float SamplingRate = OperatorSettings.GetSampleRate();
			const int32 NumFramesPerBlock = OperatorSettings.GetNumFramesPerBlock();
			FrameTimeStep = FMath::Max(1e-5f, NumFramesPerBlock / SamplingRate);
			PreviousValue = *InitValue;
		}

#if ENGINE_MINOR_VERSION > 2
		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			using namespace VelocityVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InitValue), InitValue);
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputValue), InValue);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			using namespace VelocityVertexNames;
			
			InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutVelocity), OutVelocity);
		}

		void Reset(const IOperator::FResetParams& InParams)
		{
			*OutVelocity = 0.f;
			PreviousValue = *InitValue;
			const float SampleRate = InParams.OperatorSettings.GetSampleRate();
			const int32 NumFramePerBlock = InParams.OperatorSettings.GetNumFramesPerBlock();
			FrameTimeStep = NumFramePerBlock / SampleRate;
		}
#else
		virtual void Bind(FVertexInterfaceData& InVertexData) const override
		{
			using namespace VelocityVertexNames;

			FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();
			
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InitValue), InitValue);
			Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputValue), InValue);
			
			FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

			Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutVelocity), OutVelocity);
		}
#endif
		
		virtual FDataReferenceCollection GetInputs() const override
		{
			// This should never be called. Bind(...) is called instead. This method
			// exists as a stop-gap until the API can be deprecated and removed.
			checkNoEntry();
			return {};
		}

		virtual FDataReferenceCollection GetOutputs() const override
		{
			// This should never be called. Bind(...) is called instead. This method
			// exists as a stop-gap until the API can be deprecated and removed.
			checkNoEntry();
			return {};
		}

		void Execute()
		{
			const float CurrentValue = *InValue; 
			*OutVelocity = (CurrentValue - PreviousValue) / FrameTimeStep;
			PreviousValue = CurrentValue;
		}

	private:
		FOperatorSettings OperatorSettings;

		FFloatReadRef InitValue;
		FFloatReadRef InValue;

		float PreviousValue;
		
		FFloatWriteRef OutVelocity;

		float FrameTimeStep;
	};

	TUniquePtr<IOperator> FVelocityOperator::CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
	{
		using namespace VelocityVertexNames;
		
		const FInputVertexInterfaceData& Inputs = InParams.InputData;
		FVelocityOpArgs Args =
		{
			InParams.OperatorSettings,
			
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InitValue), InParams.OperatorSettings),
			Inputs.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputValue), InParams.OperatorSettings),
		};

		return MakeUnique<FVelocityOperator>(Args);
	}
	
	const FNodeClassMetadata& FVelocityOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata
			{
				FNodeClassName{ImpactSFXSynthEngineNodes::Namespace, TEXT("Velocity"), "Velocity"},
				1, // Major Version
				0, // Minor Version
				METASOUND_LOCTEXT("VelocityDisplayName", "Velocity"),
				METASOUND_LOCTEXT("VelocityDesc", "Find the changing speed of a value."),
				PluginAuthor,
				PluginNodeMissingPrompt,
				GetVertexInterface(),
				{NodeCategories::Math},
				{},
				FNodeDisplayStyle{}
			};

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& FVelocityOperator::GetVertexInterface()
	{
		using namespace VelocityVertexNames;
			
		static const FVertexInterface VertexInterface(
			FInputVertexInterface(
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InitValue), 0.f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputValue), 0.f)
				),
			FOutputVertexInterface(
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutVelocity))
			)
		);
			
		return VertexInterface;
	}

	// Node Class
	class FVelocityNode : public FNodeFacade
	{
	public:
		FVelocityNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FVelocityOperator>())
		{
		}
	};

	// Register node
	METASOUND_REGISTER_NODE(FVelocityNode)
}

#undef LOCTEXT_NAMESPACE
