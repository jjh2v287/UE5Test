// Copyright 2023 Davide Socol. All Rights Reserved.

#include "ShiftToOctave.h"

#include "Internationalization/Regex.h"
#include "DSP/Dsp.h"

#include <Kismet/KismetStringLibrary.h>

#define LOCTEXT_NAMESPACE "ShiftToOctave"

namespace Metasound
{
	// Node parameters
	namespace ShiftToOctaveNodeNames
	{
		METASOUND_PARAM(ShiftToOctaveInFrequency, "Frequency", "Frequency input");
		METASOUND_PARAM(ShiftToOctaveInOctave, "Octave", "Octave input");

		METASOUND_PARAM(ShiftToOctaveOutFrequency, "Shifted Frequency", "Shifted frequency output");
	}

	using namespace ShiftToOctaveNodeNames;

	class ShiftToOctave : public TExecutableOperator<ShiftToOctave>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& DeclareVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		ShiftToOctave(const FOperatorSettings& InSettings, const FFloatReadRef& InFrequency, const FInt32ReadRef& InOctave);

		// virtual void Bind(FVertexInterfaceData& InVertexData) const override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;

		void Execute();

	private:
		FFloatReadRef Frequency;
		FInt32ReadRef Octave;

		FFloatWriteRef OutFrequency;
	};

	ShiftToOctave::ShiftToOctave(const FOperatorSettings& InSettings, const FFloatReadRef& InFrequency, const FInt32ReadRef& InOctave)
		: Frequency(InFrequency)
		, Octave(InOctave)
		, OutFrequency(FFloatWriteRef::CreateNew())
	{
	}

	const FNodeClassMetadata& ShiftToOctave::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata;
			Metadata.ClassName = { TEXT("UE"), TEXT("ShiftToOctave"), TEXT("Audio") };
			Metadata.MajorVersion = 1;
			Metadata.MinorVersion = 0;
			Metadata.DisplayName = METASOUND_LOCTEXT("ShiftToOctaveDisplayName", "Shift To Octave");
			Metadata.Description = METASOUND_LOCTEXT("ShiftToOctaveDesc", "Shifts input frequency inside the specified octave (range 0-9)");
			Metadata.Author = PluginAuthor;
			Metadata.PromptIfMissing = PluginNodeMissingPrompt;
			Metadata.DefaultInterface = DeclareVertexInterface();
			Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_VolumeNodeCategory", "Music Extras") };

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& ShiftToOctave::DeclareVertexInterface()
	{
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(ShiftToOctaveInFrequency)),
				TInputDataVertex<int>(METASOUND_GET_PARAM_NAME_AND_METADATA(ShiftToOctaveInOctave))
			),
			FOutputVertexInterface(
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(ShiftToOctaveOutFrequency))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> ShiftToOctave::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		TDataReadReference Frequency = InputCollection.GetDataReadReferenceOrConstruct<float>(METASOUND_GET_PARAM_NAME(ShiftToOctaveInFrequency));
		TDataReadReference Octave = InputCollection.GetDataReadReferenceOrConstruct<int>(METASOUND_GET_PARAM_NAME(ShiftToOctaveInOctave));

		return MakeUnique<ShiftToOctave>(InParams.OperatorSettings, Frequency, Octave);
	}

	FDataReferenceCollection ShiftToOctave::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection InputDataReferences;

		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(ShiftToOctaveInFrequency), Frequency);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(ShiftToOctaveInOctave), Octave);

		return InputDataReferences;
	}

	FDataReferenceCollection ShiftToOctave::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection OutputDataReferences;

		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(ShiftToOctaveOutFrequency), OutFrequency);

		return OutputDataReferences;
	}

	void ShiftToOctave::Execute()
	{
		float NoteFrequency = *Frequency.Get();
		NoteFrequency = NoteFrequency >= 16.35 ? NoteFrequency : 16.35;

		int Semitones = Audio::GetSemitones(NoteFrequency / 16.35);

		int NoteOctave = *Octave.Get();
		NoteOctave = NoteOctave < 0 ? 0 : (NoteOctave > 9 ? 9 : NoteOctave);

		int BaseSemitones = Semitones % 12;

		*OutFrequency = 16.35 * Audio::GetFrequencyMultiplier(BaseSemitones + *Octave.Get() * 12);
	}

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	ShiftToOctaveNode::ShiftToOctaveNode(const FNodeInitData& InitData)
		: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<ShiftToOctave>())
	{
	}

	// Register node
	METASOUND_REGISTER_NODE(ShiftToOctaveNode);
};

#undef LOCTEXT_NAMESPACE