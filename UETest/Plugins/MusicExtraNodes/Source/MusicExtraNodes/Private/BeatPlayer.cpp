// Copyright 2023 Davide Socol. All Rights Reserved.

#include "BeatPlayer.h"

#include "Internationalization/Regex.h"
#include "DSP/Dsp.h"

#include <Kismet/KismetStringLibrary.h>

#define LOCTEXT_NAMESPACE "BeatPlayer"

namespace Metasound
{
	// Node parameters
	namespace BeatPlayerNodeNames
	{
		METASOUND_PARAM(BeatPlayerStart, "Start", "Start playing beat");
		METASOUND_PARAM(BeatPlayerStop, "Stop", "Stop playing beat");

		METASOUND_PARAM(BeatPlayerBeatsString, "Beats", "Beats string");
		METASOUND_PARAM(BeatPlayerDoLoop, "DoLoop", "Do loop input");
		METASOUND_PARAM(BeatPlayerBeatTime, "Beat time", "Beat time duration");

		METASOUND_PARAM(BeatPlayerOut, "Out", "Trigger output");
		METASOUND_PARAM(BeatPlayerPause, "Pause", "Pause output");
		METASOUND_PARAM(BeatPlayerEnd, "End", "End output");
	}

	using namespace BeatPlayerNodeNames;

	class BeatPlayer : public TExecutableOperator<BeatPlayer>
	{
	public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& DeclareVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		BeatPlayer(const FOperatorSettings& InSettings, const FTriggerReadRef& InStart, const FTriggerReadRef& InStop, const FStringReadRef& InBeatsString, const FBoolReadRef& InDoLoop, const FTimeReadRef& InBeatTime);

		virtual FDataReferenceCollection GetInputs() const override;
		virtual FDataReferenceCollection GetOutputs() const override;

		void Execute();

	private:
		FTriggerReadRef Start;
		FTriggerReadRef Stop;

		FStringReadRef BeatsString;

		FBoolReadRef DoLoop;

		FTimeReadRef BeatTime;

		FTriggerWriteRef Out;
		FTriggerWriteRef Pause;
		FTriggerWriteRef End;

		float SampleRate;
		float NumFramesPerBlock;

		TArray<float> FrameTimes;

		TArray<bool> Beats;

		int CurrentIndex = 0;
		int CurrentBlock = 0;

		bool Playing = false;
	};

	BeatPlayer::BeatPlayer(const FOperatorSettings& InSettings, const FTriggerReadRef& InStart, const FTriggerReadRef& InStop, const FStringReadRef& InBeatsString, const FBoolReadRef& InDoLoop, const FTimeReadRef& InBeatTime)
		: Start(InStart)
		, Stop(InStop)
		, BeatsString(InBeatsString)
		, DoLoop(InDoLoop)
		, BeatTime(InBeatTime)
		, Out(FTriggerWriteRef::CreateNew(InSettings))
		, Pause(FTriggerWriteRef::CreateNew(InSettings))
		, End(FTriggerWriteRef::CreateNew(InSettings))
		, SampleRate(InSettings.GetSampleRate())
		, NumFramesPerBlock(InSettings.GetNumFramesPerBlock())
	{
	}

	const FNodeClassMetadata& BeatPlayer::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Metadata;
			Metadata.ClassName = { TEXT("UE"), TEXT("BeatPlayer"), TEXT("Audio") };
			Metadata.MajorVersion = 1;
			Metadata.MinorVersion = 0;
			Metadata.DisplayName = METASOUND_LOCTEXT("BeatPlayerDisplayName", "Beat Player");
			Metadata.Description = METASOUND_LOCTEXT("BeatPlayerDesc", "Provided a string of beats (Example \"x x o xx\", beats are divided by spaces, x is a hit o is a pause, number of elements in beat defines beat subdivision) generates triggers");
			Metadata.Author = PluginAuthor;
			Metadata.PromptIfMissing = PluginNodeMissingPrompt;
			Metadata.DefaultInterface = DeclareVertexInterface();
			Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_VolumeNodeCategory", "Music Extras") };

			return Metadata;
		};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	}

	const FVertexInterface& BeatPlayer::DeclareVertexInterface()
	{
		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerStart)),
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerStop)),
				TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerBeatsString)),
				TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerDoLoop)),
				TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerBeatTime))
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerOut)),
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerPause)),
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(BeatPlayerEnd))
			)
		);

		return Interface;
	}

	TUniquePtr<IOperator> BeatPlayer::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		TDataReadReference Start = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(BeatPlayerStart), InParams.OperatorSettings);
		TDataReadReference Stop = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(BeatPlayerStop), InParams.OperatorSettings);
		TDataReadReference BeatsString = InputCollection.GetDataReadReferenceOrConstruct<FString>(METASOUND_GET_PARAM_NAME(BeatPlayerBeatsString));
		TDataReadReference DoLoop = InputCollection.GetDataReadReferenceOrConstruct<bool>(METASOUND_GET_PARAM_NAME(BeatPlayerDoLoop));
		TDataReadReference BeatTime = InputCollection.GetDataReadReferenceOrConstruct<FTime>(METASOUND_GET_PARAM_NAME(BeatPlayerBeatTime));

		return MakeUnique<BeatPlayer>(InParams.OperatorSettings, Start, Stop, BeatsString, DoLoop, BeatTime);
	}

	FDataReferenceCollection BeatPlayer::GetInputs() const
	{
		FDataReferenceCollection InputDataReferences;

		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerStart), Start);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerStop), Stop);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerBeatsString), BeatsString);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerDoLoop), DoLoop);
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerBeatTime), BeatTime);

		return InputDataReferences;
	}

	FDataReferenceCollection BeatPlayer::GetOutputs() const
	{
		FDataReferenceCollection OutputDataReferences;

		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerOut), Out);
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerPause), Pause);
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(BeatPlayerEnd), End);

		return OutputDataReferences;
	}

	void BeatPlayer::Execute()
	{
		Out->AdvanceBlock();
		Pause->AdvanceBlock();
		End->AdvanceBlock();

		if (Playing)
		{
			CurrentBlock++;

			if (CurrentIndex == 0)
			{
				if (Beats[0])
				{
					Out->TriggerFrame(0);
				}
				else
				{
					Pause->TriggerFrame(0);
				}

				CurrentIndex++;

				if (CurrentIndex > FrameTimes.Num())
				{
					if (*DoLoop) {
						CurrentIndex = 0;
						CurrentBlock = 0;
					}
					else {
						Playing = false;
					}

					End->TriggerFrame(0);
				}
			}
			else
			{
				if (CurrentBlock >= FrameTimes[CurrentIndex - 1] * BeatTime->GetSeconds())
				{
					if (CurrentIndex < FrameTimes.Num())
					{
						if (Beats[CurrentIndex])
						{
							Out->TriggerFrame(0);
						}
						else
						{
							Pause->TriggerFrame(0);
						}
					}

					CurrentIndex++;

					if (CurrentIndex > FrameTimes.Num())
					{
						if (*DoLoop) {
							CurrentIndex = 0;
							CurrentBlock = 0;
						}
						else {
							Playing = false;
						}

						int BlocksPerBeat = BeatTime->GetSeconds() * SampleRate / NumFramesPerBlock;

						BlocksPerBeat = FMath::Max(BlocksPerBeat, 1);

						int BlocksInCurrentBeat = CurrentBlock % BlocksPerBeat;

						End->TriggerFrame(BlocksInCurrentBeat <= 0 ? 0 : BlocksPerBeat - BlocksInCurrentBeat);
					}
				}
			}
		}

		Start->ExecuteBlock(
			[&](int32 StartFrame, int32 EndFrame)
			{
			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				FString Test = *BeatsString.Get();

				if (!Test.IsEmpty())
				{
					Playing = true;

					CurrentIndex = 0;
					CurrentBlock = 0;

					FrameTimes = {};
					Beats = {};

					FRegexPattern BeatPattern = FRegexPattern(TEXT(
						"([xXoO]*)"
					));

					FRegexMatcher BeatMatcher = FRegexMatcher(BeatPattern, Test);

					while (BeatMatcher.FindNext())
					{
						FString Match = BeatMatcher.GetCaptureGroup(0);

						for (int u = 0; u < Match.Len(); u++)
						{
							Beats.Add(Match[u] == 'x' || Match[u] == 'X');
							FrameTimes.Add(FMath::Max(0, SampleRate / NumFramesPerBlock / Match.Len()));

							if (FrameTimes.Num() > 1)
							{
								FrameTimes[FrameTimes.Num() - 1] = FrameTimes[FrameTimes.Num() - 1] + FrameTimes[FrameTimes.Num() - 2];
							}
						}
					}
				}
				else
				{
					Playing = false;
					End->TriggerFrame(0);
				}
			}
		);

		Stop->ExecuteBlock(
			[&](int32 StartFrame, int32 EndFrame)
			{
			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				Playing = false;
				End->TriggerFrame(0);
			}
		);
	}

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	BeatPlayerNode::BeatPlayerNode(const FNodeInitData& InitData)
		: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<BeatPlayer>())
	{
	}

	// Register node
	METASOUND_REGISTER_NODE(BeatPlayerNode);
};

#undef LOCTEXT_NAMESPACE