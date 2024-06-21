// Copyright 2023 Davide Socol. All Rights Reserved.

#include "NoteToFrequency.h"

#include "Internationalization/Regex.h"
#include "DSP/Dsp.h"

#include <Kismet/KismetStringLibrary.h>

#define LOCTEXT_NAMESPACE "NoteToFrequency"

namespace Metasound
{
	// Node parameters
	namespace NoteToFrequencyNodeNames
	{
		METASOUND_PARAM(NoteToFrequencyInputNote, "Note", "Note string input");

		METASOUND_PARAM(NoteToFrequencyOutputFrequency, "Frequency", "Frequency output");
	}

	using namespace NoteToFrequencyNodeNames;

	class NoteToFrequency : public TExecutableOperator<NoteToFrequency>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& DeclareVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		NoteToFrequency(const FOperatorSettings& InSettings, const FStringReadRef& InNote);

		// virtual void Bind(FVertexInterfaceData& InVertexData) const override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;

		void Execute();

	private:
		FStringReadRef Note;

		FFloatWriteRef Frequency;
	};

	NoteToFrequency::NoteToFrequency(const FOperatorSettings& InSettings, const FStringReadRef& InNote)
		: Note(InNote)
		, Frequency(FFloatWriteRef::CreateNew())
	{
	}

	const FNodeClassMetadata& NoteToFrequency::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata;
			Metadata.ClassName = { TEXT("UE"), TEXT("NoteToFrequency"), TEXT("Audio") };
			Metadata.MajorVersion = 1;
			Metadata.MinorVersion = 0;
			Metadata.DisplayName = METASOUND_LOCTEXT("NoteToFrequencyDisplayName", "Note To Frequency");
			Metadata.Description = METASOUND_LOCTEXT("NoteToFrequencyDesc", "Given a note name like A#2, it returns the corresponding frequency");
			Metadata.Author = PluginAuthor;
			Metadata.PromptIfMissing = PluginNodeMissingPrompt;
			Metadata.DefaultInterface = DeclareVertexInterface();
			Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_VolumeNodeCategory", "Music Extras") };

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& NoteToFrequency::DeclareVertexInterface()
	{
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(NoteToFrequencyInputNote))
			),
			FOutputVertexInterface(
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(NoteToFrequencyOutputFrequency))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> NoteToFrequency::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		TDataReadReference Note = InputCollection.GetDataReadReferenceOrConstruct<FString>(METASOUND_GET_PARAM_NAME(NoteToFrequencyInputNote));

		return MakeUnique<NoteToFrequency>(InParams.OperatorSettings, Note);
	}

	/*
	void NoteToFrequency::Bind(FVertexInterfaceData& InVertexData) const
	{
		FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();

		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTrigger), FTriggerReadRef(Trigger));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputString), FStringReadRef(String));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(InputDivider), FStringReadRef(Divider));

		FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

		Inputs.BindWriteVertex(METASOUND_GET_PARAM_NAME(OutputStringArray), TDataWriteReference<TArray<FString>>(StringArray));
	}
	*/

	FDataReferenceCollection NoteToFrequency::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection InputDataReferences;

		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(NoteToFrequencyInputNote), Note);

		return InputDataReferences;
	}

	FDataReferenceCollection NoteToFrequency::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection OutputDataReferences;

		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(NoteToFrequencyOutputFrequency), Frequency);

		return OutputDataReferences;
	}

	void NoteToFrequency::Execute()
	{
		*Frequency = NoteToFrequencyNode::GetFrequencyFromNote(*Note.Get());
	}

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	NoteToFrequencyNode::NoteToFrequencyNode(const FNodeInitData& InitData)
		: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<NoteToFrequency>())
	{
	}

	float NoteToFrequencyNode::GetFrequencyFromNote(FString InNote)
	{
		FRegexPattern NotePattern = FRegexPattern(TEXT(
			"([a-gA-G])"	// Base note
			"([b#])?"		// Alteration
			"([0-9])?"		// Octave
		));

		FRegexMatcher NoteMatcher = FRegexMatcher(NotePattern, *InNote);

		NoteMatcher.FindNext();

		FString NoteMatch = NoteMatcher.GetCaptureGroup(0);
		
		FString BaseNote = NoteMatcher.GetCaptureGroup(1).ToUpper();
		BaseNote = BaseNote.IsEmpty() ? "A" : BaseNote;
		
		FString Alteration = NoteMatcher.GetCaptureGroup(2);
		
		FString Octave = NoteMatcher.GetCaptureGroup(3);
		Octave = Octave.IsEmpty() ? "4" : Octave;
		int OctaveMultiplier = UKismetStringLibrary::Conv_StringToInt(Octave);

		if (BaseNote <= "B") OctaveMultiplier++;

		float BaseFrequency = 440;

		int BaseSemitones = (BaseNote[0] - 65) * 2;

		if (BaseNote[0] > 66) BaseSemitones--;

		if (BaseNote[0] > 69) BaseSemitones--;

		if (Alteration == "b") BaseSemitones--;

		if (Alteration == "#") BaseSemitones++;

		BaseSemitones += (OctaveMultiplier - 4) * 12;

		return BaseFrequency * Audio::GetFrequencyMultiplier(BaseSemitones);
	}

	// Register node
	METASOUND_REGISTER_NODE(NoteToFrequencyNode);
};

#undef LOCTEXT_NAMESPACE