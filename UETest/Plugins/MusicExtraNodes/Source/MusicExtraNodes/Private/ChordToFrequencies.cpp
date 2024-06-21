// Copyright 2023 Davide Socol. All Rights Reserved.

#include "ChordToFrequencies.h"

#include "Internationalization/Regex.h"
#include "DSP/Dsp.h"

#include <Kismet/KismetStringLibrary.h>

#define LOCTEXT_NAMESPACE "ChordToFrequencies"

namespace Metasound
{
	// Node parameters
	namespace ChordToFrequenciesNodeNames
	{
		METASOUND_PARAM(InputChord, "Chord", "Chord string input");

		METASOUND_PARAM(OutputFrequencies, "Frequencies", "Frequencies output");
	}

	using namespace ChordToFrequenciesNodeNames;
	
	class ChordToFrequencies : public TExecutableOperator<ChordToFrequencies>
	{
		public:
			using FArrayScaleDegreeWriteRef = TDataWriteReference<TArray<float>>;
			using ScaleDegreeArrayType = TArray<float>;

			static const FNodeClassMetadata& GetNodeInfo();
			static const FVertexInterface& DeclareVertexInterface();
			static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

			ChordToFrequencies(const FOperatorSettings& InSettings, const TDataReadReference<FString>& InString);

			virtual FDataReferenceCollection GetInputs() const override;
			virtual FDataReferenceCollection GetOutputs() const override;

			void Execute();

		private:
			TDataReadReference<FString> Chord;
			FArrayScaleDegreeWriteRef Frequencies;
	};

	ChordToFrequencies::ChordToFrequencies(const FOperatorSettings& InSettings, const TDataReadReference<FString>& InString)
		: Chord(InString)
		, Frequencies(FArrayScaleDegreeWriteRef::CreateNew())
	{
	}

	const FNodeClassMetadata& ChordToFrequencies::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata;
			Metadata.ClassName = { TEXT("UE"), TEXT("ChordToFrequencies"), TEXT("Audio") };
			Metadata.MajorVersion = 1;
			Metadata.MinorVersion = 0;
			Metadata.DisplayName = METASOUND_LOCTEXT("ChordToFrequenciesDisplayName", "Chord To Frequencies");
			Metadata.Description = METASOUND_LOCTEXT("ChordToFrequenciesDesc", "Given a chord name as a string, like F#-add9, it returns the frequencies of the chord (1st 3rd 5th and any extra notes specified)");
			Metadata.Author = PluginAuthor;
			Metadata.PromptIfMissing = PluginNodeMissingPrompt;
			Metadata.DefaultInterface = DeclareVertexInterface();
			Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_VolumeNodeCategory", "Music Extras") };

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& ChordToFrequencies::DeclareVertexInterface()
	{
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputChord))
			),
			FOutputVertexInterface(
				TOutputDataVertex<ScaleDegreeArrayType>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputFrequencies))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> ChordToFrequencies::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();
		
		TDataReadReference Chord = InputCollection.GetDataReadReference<FString>(METASOUND_GET_PARAM_NAME(InputChord));
		
		return MakeUnique<ChordToFrequencies>(InParams.OperatorSettings, Chord);
	}

	FDataReferenceCollection ChordToFrequencies::GetInputs() const
	{
		FDataReferenceCollection InputDataReferences;
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputChord), Chord);

		return InputDataReferences;
	}

	FDataReferenceCollection ChordToFrequencies::GetOutputs() const
	{
		FDataReferenceCollection OutputDataReferences;
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputFrequencies), Frequencies);

		return OutputDataReferences;
	}

	void ChordToFrequencies::Execute()
	{
		FString ChordString = *Chord.Get();

		FRegexPattern ChordPattern = FRegexPattern(TEXT(
			"("
				"([a-gA-G])"
				"([#b])?"
				"(-|min|m|\\+|aug|a|o|dim|d)?"
			")"
			"("
				"(add|Mag|M|aug|a|\\+|#|o|dim|d|-|b|sus)?"
				"([0-9]{1,2})"
			")?"
			"("
				"(add|Mag|M|aug|a|\\+|#|o|dim|d|-|b|sus)?"
				"([0-9]{1,2})"
			")?"
			"("
				"(add|Mag|M|aug|a|\\+|#|o|dim|d|-|b|sus)?"
				"([0-9]{1,2})"
			")?"
			"("
				"(add|Mag|M|aug|a|\\+|#|o|dim|d|-|b|sus)?"
				"([0-9]{1,2})"
			")?"
			"("
				"(add|Mag|M|aug|a|\\+|#|o|dim|d|-|b|sus)?"
				"([0-9]{1,2})"
			")?"
		));

		FRegexMatcher ChordMatcher = FRegexMatcher(ChordPattern, ChordString);

		bool ChordMatch = ChordMatcher.FindNext();
		
		FString FullMatch = ChordMatcher.GetCaptureGroup(0);

		FString BaseChord = ChordMatcher.GetCaptureGroup(1);

		FString BaseNote = ChordMatcher.GetCaptureGroup(2).ToUpper();
		FString BaseAlteration = ChordMatcher.GetCaptureGroup(3);
		FString BaseQuality = ChordMatcher.GetCaptureGroup(4);

		TArray<int> Intervals = {0, 4, 7};

		int BaseSemitones = 0; // Semitones over A

		if (!BaseNote.IsEmpty())
		{
			BaseSemitones = (BaseNote[0] - 65) * 2;

			if (BaseNote[0] > 66) BaseSemitones--;

			if (BaseNote[0] > 69) BaseSemitones--;
		}

		if (BaseAlteration == "#")
		{
			BaseSemitones++;
		}
		else if (BaseAlteration == "b")
		{
			BaseSemitones--;
		}

		if (BaseQuality == "-" || BaseQuality == "m" || BaseQuality == "min")
		{
			Intervals[1]--;
		}

		if (BaseQuality == "+" || BaseQuality == "aug" || BaseQuality == "a")
		{
			Intervals[2]++;
		}

		if (BaseQuality == "o" || BaseQuality == "dim" || BaseQuality == "d")
		{
			Intervals[1]--;
			Intervals[2]--;
		}

		for (int i = 5; i <= 17; i += 3) {
			FString Alteration = ChordMatcher.GetCaptureGroup(i + 1);
			FString Note = ChordMatcher.GetCaptureGroup(i + 2);

			if (Note.IsEmpty()) break;

			int NoteNumber = UKismetStringLibrary::Conv_StringToInt(Note);

			int OctaveMultiplier = NoteNumber / 9;
			int AdjustedNoteNumber = NoteNumber % 9;

			float Semitones = (AdjustedNoteNumber - 1) * 2;

			if (AdjustedNoteNumber > 3) Semitones--;

			if (AdjustedNoteNumber > 7) Semitones--;

			Semitones += OctaveMultiplier * 12;

			// add|Mag|M|_ aug|a|\\+|# dim|d|-|b no|sus
			if (Alteration == "add" || Alteration == "Mag" || Alteration == "M" || Alteration.IsEmpty())
			{
				switch (NoteNumber)
				{
				case 1:
					Intervals[0] = 0;
					break;
				case 3:
					Intervals[1] = 4;
					break;
				case 5:
					Intervals[2] = 7;
					break;
				case 7:
					Intervals.Add(Alteration.IsEmpty() || Alteration == "add" ? 10 : 11); // 7 or add7 => minor 7th, Mag7 or M7 => major 7th
					break;
				default:
					Intervals.Add(Semitones);
					break;
				}
			}
			else if (Alteration == "aug" || Alteration == "a" || Alteration == "+" || Alteration == "#")
			{
				switch (NoteNumber)
				{
				case 1:
					Intervals[0] = 1;
					break;
				case 3:
					Intervals[1] = 5;
					break;
				case 5:
					Intervals[2] = 8;
					break;
				default:
					Intervals.Add(Semitones + 1);
					break;
				}
			}
			else if (Alteration == "o" || Alteration == "dim" || Alteration == "d" || Alteration == "-" || Alteration == "b")
			{
				switch (NoteNumber)
				{
				case 1:
					Intervals[0] = -1;
					break;
				case 3:
					Intervals[1] = 4;
					break;
				case 5:
					Intervals[2] = 8;
					break;
				default:
					Intervals.Add(Semitones - 1);
					break;
				}
			}
			else if (Alteration == "sus")
			{
				switch (NoteNumber)
				{
				case 2:
					Intervals[1] = 2;
					break;
				case 4:
					Intervals[1] = 5;
					break;
				default:
					break;
				}
			}
		}

		TArray<float> NewFrequencies;

		float BaseFrequency = 440 * Audio::GetFrequencyMultiplier(BaseSemitones);

		for(int i = 0; i < Intervals.Num(); i++)
		{
			NewFrequencies.Add(BaseFrequency * Audio::GetFrequencyMultiplier(Intervals[i]));
		}

		*Frequencies = NewFrequencies;
	}

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	ChordToFrequenciesNode::ChordToFrequenciesNode(const FNodeInitData& InitData)
	:	FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<ChordToFrequencies>())
	{
	}
	
	// Register node
	METASOUND_REGISTER_NODE(ChordToFrequenciesNode);
};

#undef LOCTEXT_NAMESPACE