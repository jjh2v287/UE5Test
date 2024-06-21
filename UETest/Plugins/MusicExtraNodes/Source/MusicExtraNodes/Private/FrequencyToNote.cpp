// Copyright 2023 Davide Socol. All Rights Reserved.

#include "FrequencyToNote.h"

#include "Internationalization/Regex.h"
#include "DSP/Dsp.h"

#include <Kismet/KismetStringLibrary.h>

#define LOCTEXT_NAMESPACE "FrequencyToNote"

namespace Metasound
{
	// Node parameters
	namespace FrequencyToNoteNodeNames
	{
		METASOUND_PARAM(FrequencyToNoteInputFrequency, "Frequency", "Frequency input");

		METASOUND_PARAM(FrequencyToNoteOutputNote, "Note", "Note string output");
	}

	using namespace FrequencyToNoteNodeNames;

	class FrequencyToNote : public TExecutableOperator<FrequencyToNote>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& DeclareVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		FrequencyToNote(const FOperatorSettings& InSettings, const FFloatReadRef& InFrequency);

		// virtual void Bind(FVertexInterfaceData& InVertexData) const override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;

		void Execute();

	private:
		FFloatReadRef Frequency;

		FStringWriteRef Note;
	};

	FrequencyToNote::FrequencyToNote(const FOperatorSettings& InSettings, const FFloatReadRef& InFrequency)
		: Frequency(InFrequency)
		, Note(FStringWriteRef::CreateNew())
	{
	}

	const FNodeClassMetadata& FrequencyToNote::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata;
			Metadata.ClassName = { TEXT("UE"), TEXT("FrequencyToNote"), TEXT("Audio") };
			Metadata.MajorVersion = 1;
			Metadata.MinorVersion = 0;
			Metadata.DisplayName = METASOUND_LOCTEXT("FrequencyToNoteDisplayName", "Frequency To Note");
			Metadata.Description = METASOUND_LOCTEXT("FrequencyToNoteDesc", "Given a frequency, it returns the corresponding note name, like C#4");
			Metadata.Author = PluginAuthor;
			Metadata.PromptIfMissing = PluginNodeMissingPrompt;
			Metadata.DefaultInterface = DeclareVertexInterface();
			Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_VolumeNodeCategory", "Music Extras") };

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& FrequencyToNote::DeclareVertexInterface()
	{
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(FrequencyToNoteInputFrequency))
			),
			FOutputVertexInterface(
				TOutputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(FrequencyToNoteOutputNote))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> FrequencyToNote::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		TDataReadReference Frequency = InputCollection.GetDataReadReferenceOrConstruct<float>(METASOUND_GET_PARAM_NAME(FrequencyToNoteInputFrequency));

		return MakeUnique<FrequencyToNote>(InParams.OperatorSettings, Frequency);
	}

	FDataReferenceCollection FrequencyToNote::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection InputDataReferences;

		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(FrequencyToNoteInputFrequency), Frequency);

		return InputDataReferences;
	}

	FDataReferenceCollection FrequencyToNote::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection OutputDataReferences;

		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(FrequencyToNoteOutputNote), Note);

		return OutputDataReferences;
	}

	void FrequencyToNote::Execute()
	{
		float NoteFrequency = *Frequency.Get();
		NoteFrequency = NoteFrequency >= 16.35 ? NoteFrequency : 16.35;

		int Semitones = Audio::GetSemitones(NoteFrequency / 16.35);

		int BaseSemitones = Semitones % 12;
		int Octave = Semitones / 12;

		TArray<FString> Notes = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

		*Note = Notes[BaseSemitones] + FString::FromInt(Octave);
	}

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	FrequencyToNoteNode::FrequencyToNoteNode(const FNodeInitData& InitData)
		: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FrequencyToNote>())
	{
	}

	// Register node
	METASOUND_REGISTER_NODE(FrequencyToNoteNode);
};

#undef LOCTEXT_NAMESPACE