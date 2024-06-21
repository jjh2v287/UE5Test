// Copyright 2023 Davide Socol. All Rights Reserved.

#include "MelodyPlayer.h"

#include "Internationalization/Regex.h"
#include "DSP/Dsp.h"

#include <NoteToFrequency.h>

#define LOCTEXT_NAMESPACE "MelodyPlayerNode"

namespace Metasound
{
	// Node parameters
	namespace MelodyPlayerNames
	{
		METASOUND_PARAM(MelodyPlayerInStart, "Start", "Start playing the melody");
		METASOUND_PARAM(MelodyPlayerInStop, "Stop", "Stop playing the melody");

		METASOUND_PARAM(MelodyPlayerInMelody, "Melody string", "Notes string (ex. note: \"C#4d1.5i0.4\" , (d)uration and (i)ntensity are optional and default to 1) separated by spaces");

		METASOUND_PARAM(MelodyPlayerInDoLoop, "Do Loop", "Does the melody loop");
		METASOUND_PARAM(MelodyPlayerInBeatTime, "Beat Time", "Beat time (1 beat note duration)");

		METASOUND_PARAM(MelodyPlayerOutAttack, "Attack", "Attack output");
		METASOUND_PARAM(MelodyPlayerOutRelease, "Release", "Release output");
		METASOUND_PARAM(MelodyPlayerOutFrequency, "Frequency", "Frequency output");
		METASOUND_PARAM(MelodyPlayerOutIntensity, "Intensity", "Intensity output");
		METASOUND_PARAM(MelodyPlayerOutEnd, "End", "Melody end output");
	}

	using namespace MelodyPlayerNames;

	class MelodyPlayer : public TExecutableOperator<MelodyPlayer>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& DeclareVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		MelodyPlayer(const FOperatorSettings& InSettings, const FTriggerReadRef& InTriggerStart, const FTriggerReadRef& InTriggerStop, const FStringReadRef& InMelody, const FBoolReadRef& InDoLoop, const FTimeReadRef& InBeatDuration);

		// virtual void Bind(FVertexInterfaceData& InVertexData) const override;
		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;

		void Execute();

	private:
		FTriggerReadRef TriggerStart;
		FTriggerReadRef TriggerStop;

		FStringReadRef Melody;

		FBoolReadRef DoLoop;
		FTimeReadRef BeatDuration;

		FTriggerWriteRef TriggerAttack;
		FTriggerWriteRef TriggerRelease;
		FTriggerWriteRef TriggerEnd;

		FFloatWriteRef Frequency;
		FFloatWriteRef Intensity;

		float SampleRate;
		float NumFramesPerBlock;

		TArray<float> FrameTimes;
		TArray<float> Frequencies;
		TArray<float> Intensities;
		int CurrentIndex = 0;
		int CurrentBlock = 0;

		bool Playing = false;
	};

	MelodyPlayer::MelodyPlayer(const FOperatorSettings& InSettings, const FTriggerReadRef& InTriggerStart, const FTriggerReadRef& InTriggerStop, const FStringReadRef& InMelody, const FBoolReadRef& InDoLoop, const FTimeReadRef& InBeatDuration)
		: TriggerStart(InTriggerStart)
		, TriggerStop(InTriggerStop)
		, Melody(InMelody)
		, DoLoop(InDoLoop)
		, BeatDuration(InBeatDuration)
		, TriggerAttack(FTriggerWriteRef::CreateNew(InSettings))
		, TriggerRelease(FTriggerWriteRef::CreateNew(InSettings))
		, TriggerEnd(FTriggerWriteRef::CreateNew(InSettings))
		, Frequency(FFloatWriteRef::CreateNew())
		, Intensity(FFloatWriteRef::CreateNew())
		, SampleRate(InSettings.GetSampleRate())
		, NumFramesPerBlock(InSettings.GetNumFramesPerBlock())
	{
	}

	const FNodeClassMetadata& MelodyPlayer::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata;
			Metadata.ClassName = { TEXT("UE"), TEXT("MelodyPlayer"), TEXT("Audio") };
			Metadata.MajorVersion = 1;
			Metadata.MinorVersion = 0;
			Metadata.DisplayName = METASOUND_LOCTEXT("MelodyPlayerDisplayName", "Melody Player");
			Metadata.Description = METASOUND_LOCTEXT("MelodyPlayerDesc", "Melody Player outputs attack,release,frequency and intensity in sequence based on the input string sequence");
			Metadata.Author = PluginAuthor;
			Metadata.PromptIfMissing = PluginNodeMissingPrompt;
			Metadata.DefaultInterface = DeclareVertexInterface();
			Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_VolumeNodeCategory", "Music Extras") };

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& MelodyPlayer::DeclareVertexInterface()
	{
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerInStart)),
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerInStop)),
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerInMelody)),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerInDoLoop)),
				TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerInBeatTime))
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerOutAttack)),
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerOutRelease)),
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerOutFrequency)),
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerOutIntensity)),
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(MelodyPlayerOutEnd))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> MelodyPlayer::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		FTriggerReadRef TriggerStart = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(MelodyPlayerInStart), InParams.OperatorSettings);
		FTriggerReadRef TriggerStop = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(MelodyPlayerInStop), InParams.OperatorSettings);

		TDataReadReference Melody = InputCollection.GetDataReadReferenceOrConstruct<FString>(METASOUND_GET_PARAM_NAME(MelodyPlayerInMelody));
		TDataReadReference DoLoop = InputCollection.GetDataReadReferenceOrConstruct<bool>(METASOUND_GET_PARAM_NAME(MelodyPlayerInDoLoop));
		TDataReadReference BeatDuration = InputCollection.GetDataReadReferenceOrConstruct<FTime>(METASOUND_GET_PARAM_NAME(MelodyPlayerInBeatTime));

		return MakeUnique<MelodyPlayer>(InParams.OperatorSettings, TriggerStart, TriggerStop, Melody, DoLoop, BeatDuration);
	}

	/* OUTPUT TRIGGERS DON'T WORK WITH THIS ¯\O/¯
	void MelodyPlayer::Bind(FVertexInterfaceData& InVertexData) const
	{
		FInputVertexInterfaceData& Inputs = InVertexData.GetInputs();

		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerInStart), FTriggerReadRef(TriggerStart));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerInStop), FTriggerReadRef(TriggerStop));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerInMelody), TDataReadReference<TArray<FString>>(Melody));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerInDoLoop), FBoolReadRef(DoLoop));
		Inputs.BindReadVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerInBeatTime), FTimeReadRef(BeatDuration));

		FOutputVertexInterfaceData& Outputs = InVertexData.GetOutputs();

		Inputs.BindWriteVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerOutAttack), FTriggerWriteRef(TriggerAttack));
		Inputs.BindWriteVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerOutRelease), FTriggerWriteRef(TriggerRelease));
		Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(MelodyPlayerOutFrequency), FFloatWriteRef(Frequency));
		Outputs.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputDuration), FTimeWriteRef(Duration));
	}
	*/

	FDataReferenceCollection MelodyPlayer::GetInputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection InputDataReferences;

		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerInStart), TriggerStart);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerInStop), TriggerStop);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerInMelody), Melody);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerInDoLoop), DoLoop);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerInBeatTime), BeatDuration);

		return InputDataReferences;
	}

	FDataReferenceCollection MelodyPlayer::GetOutputs() const
	{
		// This should never be called. Bind(...) is called instead. This method
		// exists as a stop-gap until the API can be deprecated and removed. 

		FDataReferenceCollection OutputDataReferences;

		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerOutAttack), TriggerAttack);
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerOutRelease), TriggerRelease);
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerOutFrequency), Frequency);
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerOutIntensity), Intensity);
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(MelodyPlayerOutEnd), TriggerEnd);

		return OutputDataReferences;
	}

	void MelodyPlayer::Execute()
	{
		TriggerAttack->AdvanceBlock();
		TriggerRelease->AdvanceBlock();
		TriggerEnd->AdvanceBlock();

		if (Playing)
		{
			CurrentBlock++;

			if (CurrentIndex == 0)
			{
				float NewFrequency = Frequencies[0];
				float NewIntensity = Intensities[0];

				if (NewFrequency > 0 && NewIntensity > 0)
				{
					*Frequency = NewFrequency;
					*Intensity = NewIntensity;

					TriggerAttack->TriggerFrame(1);
				}

				CurrentIndex++;

				if (CurrentIndex > FrameTimes.Num())
				{
					TriggerRelease->TriggerFrame(0);

					if (*DoLoop)
					{
						CurrentIndex = 0;
						CurrentBlock = 0;
					}
					else
					{
						Playing = false;
					}

					TriggerEnd->TriggerFrame(0);
				}
			}
			else
			{
				if (CurrentBlock >= FrameTimes[CurrentIndex - 1] * BeatDuration->GetSeconds() - 1) // -1 To activate Release 1 frame early and Attack on the right frame
				{
					if (CurrentIndex < FrameTimes.Num())
					{
						float NewFrequency = Frequencies[CurrentIndex];
						float NewIntensity = Intensities[CurrentIndex];

						if (NewFrequency > 0 && NewIntensity > 0)
						{
							*Frequency = NewFrequency;
							*Intensity = NewIntensity;

							TriggerAttack->TriggerFrame(1);
						}
					}

					TriggerRelease->TriggerFrame(0);
					
					CurrentIndex++;					

					if (CurrentIndex > FrameTimes.Num())
					{
						if (*DoLoop)
						{
							CurrentIndex = 0;
							CurrentBlock = 0;
						}
						else {
							Playing = false;
						}
						
						int BlocksPerBeat = BeatDuration->GetSeconds() * SampleRate / NumFramesPerBlock;

						BlocksPerBeat = FMath::Max(BlocksPerBeat, 1);

						int BlocksInCurrentBeat = CurrentBlock % BlocksPerBeat;

						TriggerEnd->TriggerFrame(BlocksInCurrentBeat <= 0 ? 0 : BlocksPerBeat - BlocksInCurrentBeat);
					}
				}
			}
		}

		TriggerStart->ExecuteBlock(
			[&](int32 StartFrame, int32 EndFrame)
			{
			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				FString MelodyString = *Melody.Get();

				if (!MelodyString.IsEmpty())
				{
					Playing = true;
					CurrentIndex = 0;
					CurrentBlock = 0;
					FrameTimes = {};
					Frequencies = {};

					FRegexPattern NotePattern = FRegexPattern(TEXT(
						"("
							"[a-gA-G]"			// Base note
							"[b#]?"				// Alteration
							"[0-9]?"				// Octave
						"|P)"						// Pause
						"(d[0-9]*[,.]?[0-9]*)?"		// Duration
						"(i[0-9]*[,.]?[0-9]*)?"		// Intensity
					));

					FRegexMatcher NoteMatcher = FRegexMatcher(NotePattern, MelodyString);

					while (NoteMatcher.FindNext())
					{
						FString FullMatch = NoteMatcher.GetCaptureGroup(0);
						FString NoteMatch = NoteMatcher.GetCaptureGroup(1);

						if (!NoteMatch.IsEmpty() && NoteMatch != "P") {
							float NoteFrequency = NoteToFrequencyNode::GetFrequencyFromNote(NoteMatch);

							Frequencies.Add(NoteFrequency);

							FString IntensityString = NoteMatcher.GetCaptureGroup(3);
							IntensityString.RemoveAt(0);
							float NoteIntensity = IntensityString.IsEmpty() ? 1 : FCString::Atof(*IntensityString);

							Intensities.Add(NoteIntensity);
						}
						else
						{
							Frequencies.Add(0);

							Intensities.Add(0);
						}

						FString DurationString = NoteMatcher.GetCaptureGroup(2);
						DurationString.RemoveAt(0);
						float Duration = DurationString.IsEmpty() ? 1 : FCString::Atof(*DurationString);

						FrameTimes.Add(FMath::Max(0, Duration * SampleRate / NumFramesPerBlock));

						if (FrameTimes.Num() > 1)
						{
							FrameTimes[FrameTimes.Num() - 1] = FrameTimes[FrameTimes.Num() - 1] + FrameTimes[FrameTimes.Num() - 2];
						}
					}
				}
				else
				{
					Playing = false;
					TriggerEnd->TriggerFrame(0);
				}
			}
		);

		TriggerStop->ExecuteBlock(
			[&](int32 StartFrame, int32 EndFrame)
			{
			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				Playing = false;
				TriggerEnd->TriggerFrame(0);
			}
		);
	}

	MelodyPlayerNode::MelodyPlayerNode(const FNodeInitData& InitData)
		: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<MelodyPlayer>())
	{
	}

	// Register node
	METASOUND_REGISTER_NODE(MelodyPlayerNode);
};

#undef LOCTEXT_NAMESPACE