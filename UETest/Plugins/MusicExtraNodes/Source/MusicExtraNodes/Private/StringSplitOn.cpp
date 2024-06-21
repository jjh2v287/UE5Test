// Copyright 2023 Davide Socol. All Rights Reserved.

#include "StringSplitOn.h"

#include "Internationalization/Regex.h"
#include "DSP/Dsp.h"

#include <Kismet/KismetStringLibrary.h>

#define LOCTEXT_NAMESPACE "StringSplitOn"

namespace Metasound
{
	// Node parameters
	namespace StringSplitOnNodeNames
	{
		METASOUND_PARAM(InputTrigger, "In", "Input trigger");
		METASOUND_PARAM(InputString, "String", "String input");
		METASOUND_PARAM(InputDivider, "Divider", "Divider input");

		METASOUND_PARAM(OutputStringArray, "String Array", "String array output");
	}

	using namespace StringSplitOnNodeNames;

	class StringSplitOn : public TExecutableOperator<StringSplitOn>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& DeclareVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		StringSplitOn(const FOperatorSettings& InSettings, const FTriggerReadRef& InTrigger, const FStringReadRef& InString, const FStringReadRef& InDivider);

		// virtual void Bind(FVertexInterfaceData& InVertexData) const override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;

		void Execute();

	private:
		FTriggerReadRef Trigger;

		FStringReadRef String;
		FStringReadRef Divider;

		TDataWriteReference<TArray<FString>> StringArray;
	};

	StringSplitOn::StringSplitOn(const FOperatorSettings& InSettings, const FTriggerReadRef& InTrigger, const FStringReadRef& InString, const FStringReadRef& InDivider)
		: Trigger(InTrigger)
		, String(InString)
		, Divider(InDivider)
		, StringArray(TDataWriteReference<TArray<FString>>::CreateNew())
	{
	}

	const FNodeClassMetadata& StringSplitOn::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata;
			Metadata.ClassName = { TEXT("UE"), TEXT("StringSplitOn"), TEXT("Audio") };
			Metadata.MajorVersion = 1;
			Metadata.MinorVersion = 0;
			Metadata.DisplayName = METASOUND_LOCTEXT("StringSplitOnDisplayName", "String Split On");
			Metadata.Description = METASOUND_LOCTEXT("StringSplitOnDesc", "Returns an array of strings from the given split, using the specified divider");
			Metadata.Author = PluginAuthor;
			Metadata.PromptIfMissing = PluginNodeMissingPrompt;
			Metadata.DefaultInterface = DeclareVertexInterface();
			Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_VolumeNodeCategory", "Music Extras") };

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& StringSplitOn::DeclareVertexInterface()
	{
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTrigger)),
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputString)),
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDivider))
			),
			FOutputVertexInterface(
				TOutputDataVertex<TArray<FString>>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputStringArray))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> StringSplitOn::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		TDataReadReference Trigger = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InputTrigger), InParams.OperatorSettings);
		TDataReadReference String = InputCollection.GetDataReadReferenceOrConstruct<FString>(METASOUND_GET_PARAM_NAME(InputString));
		TDataReadReference Divider = InputCollection.GetDataReadReferenceOrConstruct<FString>(METASOUND_GET_PARAM_NAME(InputDivider));

		return MakeUnique<StringSplitOn>(InParams.OperatorSettings, Trigger, String, Divider);
	}

	/*
	void StringSplitOn::Bind(FVertexInterfaceData& InVertexData) const
	{
		FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();

		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTrigger), FTriggerReadRef(Trigger));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputString), FStringReadRef(String));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDivider), FStringReadRef(Divider));

		FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

		Inputs.BindWriteVertex(METASOUND_GET_PARAM_NAME(OutputStringArray), TDataWriteReference<TArray<FString>>(StringArray));
	}
	*/

	FDataReferenceCollection StringSplitOn::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection InputDataReferences;

		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputTrigger), Trigger);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputString), String);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputDivider), Divider);

		return InputDataReferences;
	}

	FDataReferenceCollection StringSplitOn::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection OutputDataReferences;

		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputStringArray), StringArray);

		return OutputDataReferences;
	}

	void StringSplitOn::Execute()
	{
		Trigger->ExecuteBlock(
			[&](int32 StartFrame, int32 EndFrame)
			{
			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				FString MyString = *String.Get();
				FString MyDivider = *Divider.Get();

				TArray<FString> Parsed = UKismetStringLibrary::ParseIntoArray(MyString, MyDivider);

				*StringArray = Parsed;
			}
		);
	}

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	StringSplitOnNode::StringSplitOnNode(const FNodeInitData& InitData)
		: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<StringSplitOn>())
	{
	}

	// Register node
	METASOUND_REGISTER_NODE(StringSplitOnNode);
};

#undef LOCTEXT_NAMESPACE