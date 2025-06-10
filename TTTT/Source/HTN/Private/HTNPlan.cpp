// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNPlan.h"
#include "HTNDecorator.h"
#include "HTNService.h"
#include "HTNTask.h"
#include "Nodes/HTNNode_Parallel.h"
#include "Nodes/HTNNode_If.h"

#include "Algo/AnyOf.h"
#include "Algo/Find.h"
#include "Algo/Transform.h"
#include "Misc/EnumClassFlags.h"
#include "VisualLogger/VisualLogger.h"

const FHTNPlanStepID FHTNPlanStepID::None = { INDEX_NONE, INDEX_NONE };

FHTNPlan::FHTNPlan() :
	Cost(0),
	NumPotentialDivergencePointsDuringPlanAdjustment(0),
	NumRemainingPotentialDivergencePointsDuringPlanAdjustment(0),
	bDivergesFromCurrentPlanDuringPlanAdjustment(false)
{}

FHTNPlan::FHTNPlan(UHTN* HTNAsset, TSharedRef<FBlackboardWorldState> WorldStateAtPlanStart, UHTNStandaloneNode* RootNodeOverride) :
	Levels { MakeShared<FHTNPlanLevel>(HTNAsset, WorldStateAtPlanStart) },
	Cost(0),
	RootNodeOverride(RootNodeOverride),
	NumPotentialDivergencePointsDuringPlanAdjustment(0),
	NumRemainingPotentialDivergencePointsDuringPlanAdjustment(0),
	bDivergesFromCurrentPlanDuringPlanAdjustment(false)
{
	// If we have a RootNodeOverride (meaning we're a subplan caused by a HTNTask_SubPlan), 
	// we must prevent our top level from considering the RootDecorators of the HTNAsset as its own.
	Levels[0]->bIsInline = RootNodeOverride != nullptr;
}

TSharedRef<FHTNPlan> FHTNPlan::MakeCopy(int32 IndexOfLevelToCopy, bool bAlsoCopyParentLevel) const
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FHTNPlan::MakeCopy"), STAT_AI_HTN_PlanMakeCopy, STATGROUP_AI_HTN);
	
	struct Local
	{
		static bool CopyLevel(FHTNPlan& Plan, int32 LevelIndex)
		{
			if (ensure(Plan.HasLevel(LevelIndex)))
			{
				Plan.Levels[LevelIndex] = MakeShared<FHTNPlanLevel>(*Plan.Levels[LevelIndex]);
				return true;
			}
			
			return false;
		}
	};
	
	const TSharedRef<FHTNPlan> NewPlan = MakeShared<FHTNPlan>(*this);
	if (Local::CopyLevel(*NewPlan, IndexOfLevelToCopy))
	{
		if (bAlsoCopyParentLevel && IndexOfLevelToCopy > 0)
		{
			const int32 ParentLevelIndex = NewPlan->Levels[IndexOfLevelToCopy]->ParentStepID.LevelIndex;
			Local::CopyLevel(*NewPlan, ParentLevelIndex);
		}
	}

	return NewPlan;
}

bool FHTNPlan::HasLevel(int32 LevelIndex) const
{
	return Levels.IsValidIndex(LevelIndex) && Levels[LevelIndex].IsValid();
}

bool FHTNPlan::IsComplete() const
{
	for (int32 LevelIndex = 0; LevelIndex < Levels.Num(); ++LevelIndex)
	{
		if (!IsLevelComplete(LevelIndex))
		{
			return false;
		}
	}

	return true;
}

bool FHTNPlan::IsLevelComplete(int32 LevelIndex) const
{
	if (!ensure(HasLevel(LevelIndex)))
	{
		return false;
	}

	const FHTNPlanLevel& Level = *Levels[LevelIndex];

	if (!Level.Steps.Num())
	{
		return false;
	}

	const FHTNPlanStep& LastStepInLevel = Level.Steps.Last();

	const bool bHasInlinePrimarySubLevel = LastStepInLevel.SubLevelIndex != INDEX_NONE && Levels[LastStepInLevel.SubLevelIndex]->IsInlineLevel();
	const bool bHasInlineSecondarySubLevel = LastStepInLevel.SecondarySubLevelIndex != INDEX_NONE && Levels[LastStepInLevel.SecondarySubLevelIndex]->IsInlineLevel();
	if (bHasInlinePrimarySubLevel || bHasInlineSecondarySubLevel || (LastStepInLevel.Node.IsValid() && !LastStepInLevel.Node->bPlanNextNodesAfterThis))
	{
		if (bHasInlinePrimarySubLevel && !IsLevelComplete(LastStepInLevel.SubLevelIndex))
		{
			return false;
		}

		if (bHasInlineSecondarySubLevel && !IsLevelComplete(LastStepInLevel.SecondarySubLevelIndex))
		{
			return false;
		}

		return true;
	}

	return LastStepInLevel.Node->NextNodes.Num() == 0;
}

bool FHTNPlan::FindStepToAddAfter(FHTNPlanStepID& OutPlanStepID) const
{
	bool bIncompleteLevelSkippedBecauseWorldStateNotSet = false;

	// Find an incomplete level, starting with the deepest ones.
	for (int32 LevelIndex = Levels.Num() - 1; LevelIndex >= 0; --LevelIndex)
	{
		if (!IsLevelComplete(LevelIndex))
		{
			const FHTNPlanLevel& Level = *Levels[LevelIndex];
			if (Level.WorldStateAtLevelStart.IsValid())
			{
				OutPlanStepID = { LevelIndex, Level.Steps.Num() ? Level.Steps.Num() - 1 : INDEX_NONE };
				return true;
			}
			else
			{
				bIncompleteLevelSkippedBecauseWorldStateNotSet = true;
			}
		}
	}

	ensureMsgf(!bIncompleteLevelSkippedBecauseWorldStateNotSet, TEXT("The only remaining incomplete plan levels don't have a worldstate set!"));
	OutPlanStepID = FHTNPlanStepID::None;
	return false;
}

void FHTNPlan::GetWorldStateAndNextNodes(const FHTNPlanStepID& StepID, TSharedPtr<FBlackboardWorldState>& OutWorldState, FHTNNextNodesBuffer& OutNextNodes) const
{
	const FHTNPlanLevel& Level = *Levels[StepID.LevelIndex];

	// The beginning of a level
	if (StepID.StepIndex == INDEX_NONE)
	{
		check(Level.WorldStateAtLevelStart.IsValid());
		check(Level.HTNAsset.IsValid());

		OutWorldState = Level.WorldStateAtLevelStart;

		if (Level.ParentStepID == FHTNPlanStepID::None && RootNodeOverride.IsValid())
		{
			// We're at the beginning of a subplan.
			RootNodeOverride->GetNextNodes(OutNextNodes, *this);
		}
		else if (!Level.IsInlineLevel())
		{
			// We're on the root node of the HTN asset of this level.
			// Since the root node is not an actual HTNNode we manually get the nodes connected to it.
			OutNextNodes = Level.HTNAsset->StartNodes;
		}
		else if (ensure(Level.ParentStepID != FHTNPlanStepID::None))
		{
			// We're starting a sublevel contained within a node in a higher level. 
			// That node will determine what nodes go in this sublevel.
			const FHTNPlanStep& ParentPlanStep = GetStep(Level.ParentStepID);
			if (ensure(ParentPlanStep.Node.IsValid()))
			{
				ParentPlanStep.Node->GetNextNodes(OutNextNodes, *this, Level.ParentStepID, StepID.LevelIndex);
			}
		}
	}
	// If given a valid step, just return the WS and NextNodes of that step
	else
	{
		check(Level.Steps.IsValidIndex(StepID.StepIndex));
		const FHTNPlanStep& StepToAddAfter = Level.Steps[StepID.StepIndex];
		check(StepToAddAfter.Node.IsValid());
		check(StepToAddAfter.WorldState.IsValid());

		OutWorldState = StepToAddAfter.WorldState;
		StepToAddAfter.Node->GetNextNodes(OutNextNodes, *this, StepID);
	}
}

void FHTNPlan::CheckIntegrity() const
{
#if DO_CHECK
	check(Levels.Num() > 0);
	check(Cost == Levels[0]->Cost);

	for (int32 LevelIndex = 0; LevelIndex < Levels.Num(); ++LevelIndex)
	{	
		check(Levels[LevelIndex].IsValid());
		const FHTNPlanLevel& Level = *Levels[LevelIndex];
		if (!Level.IsDummyLevel())
		{
			check(Level.WorldStateAtLevelStart.IsValid());

			if (LevelIndex == 0)
			{
				check(Level.ParentStepID == FHTNPlanStepID::None);
				check(!Level.IsInlineLevel() || RootNodeOverride.IsValid());
			}
			else
			{
				check(Level.ParentStepID.LevelIndex != LevelIndex);
				check(FindStep(Level.ParentStepID));
				check(!(Level.RootSubNodesInfo.bSubNodesExecuting && !GetStep(Level.ParentStepID).SubNodesInfo.bSubNodesExecuting));
			}

			check(!(!Level.RootSubNodesInfo.bSubNodesExecuting && Level.RootSubNodesInfo.LastFrameSubNodesTicked.IsSet()));

			check(Level.Steps.Num() > 0);
			for (int32 StepIndex = 0; StepIndex < Level.Steps.Num(); ++StepIndex)
			{
				const FHTNPlanStep& Step = Level.Steps[StepIndex];

				if (Step.SubLevelIndex != INDEX_NONE)
				{
					check(HasLevel(Step.SubLevelIndex));

					const FHTNPlanLevel& SubLevel = *Levels[Step.SubLevelIndex];
					if (!SubLevel.IsDummyLevel())
					{
						check(SubLevel.ParentStepID.LevelIndex == LevelIndex);
						check(SubLevel.ParentStepID.StepIndex == StepIndex);
					}
				}

				if (Step.SecondarySubLevelIndex != INDEX_NONE)
				{
					check(HasLevel(Step.SecondarySubLevelIndex));

					const FHTNPlanLevel& SecondarySubLevel = *Levels[Step.SecondarySubLevelIndex];
					if (!SecondarySubLevel.IsDummyLevel())
					{
						check(SecondarySubLevel.ParentStepID.LevelIndex == LevelIndex);
						check(SecondarySubLevel.ParentStepID.StepIndex == StepIndex);
					}
				}

				check(Step.Cost >= 0);

				check(!(!Step.SubNodesInfo.bSubNodesExecuting && Step.SubNodesInfo.LastFrameSubNodesTicked.IsSet()));
				check(!(Step.SubNodesInfo.bSubNodesExecuting && !Level.RootSubNodesInfo.bSubNodesExecuting));
			}
		}
		else
		{
			check(!Level.IsInlineLevel());
			check(Level.HTNAsset == nullptr);
			check(Level.WorldStateAtLevelStart == nullptr);
			check(Level.ParentStepID == FHTNPlanStepID::None);
			check(Level.Cost == 0);
			check(Level.Steps.Num() == 0);
			check(Level.RootSubNodesInfo.DecoratorInfos.Num() == 0);
			check(Level.RootSubNodesInfo.ServiceInfos.Num() == 0);
		}
	}

	if (RootNodeOverride.IsValid())
	{
		check(Levels[0]->IsInlineLevel());
	}
#endif
}

void FHTNPlan::VerifySubNodesAreInactive(const UObject* VisLogOwner) const
{
#if ENABLE_VISUAL_LOG
	const auto DescribeStep = [&](const FHTNPlanStepID& PlanStepID) -> FString
	{
		if (const FHTNPlanStep* const Step = FindStep(PlanStepID))
		{
			return Step->Node.IsValid() ? *Step->Node->GetShortDescription() : TEXT("[missing node]");
		}
		
		return TEXT("root");
	};

	for (int32 LevelIndex = 0; LevelIndex < Levels.Num(); ++LevelIndex)
	{
		FHTNPlanLevel& Level = *Levels[LevelIndex];

		UE_CVLOG_UELOG(Level.RootSubNodesInfo.bSubNodesExecuting,
			VisLogOwner, LogHTN, Error,
			TEXT("Plan Level under '%s' (level %d) unexpectedly has subnodes executing"),
			*DescribeStep(Level.ParentStepID),
			LevelIndex);

		UE_CVLOG_UELOG(Level.RootSubNodesInfo.LastFrameSubNodesTicked.IsSet(),
			VisLogOwner, LogHTN, Error,
			TEXT("Plan Level under '%s' (level %d) unexpectedly has subnodes ticked %d frames ago"),
			*DescribeStep(Level.ParentStepID),
			LevelIndex,
			GFrameNumber - *Level.RootSubNodesInfo.LastFrameSubNodesTicked);

		for (int32 StepIndex = 0; StepIndex < Level.Steps.Num(); ++StepIndex)
		{
			const FHTNPlanStepID StepID { LevelIndex, StepIndex };
			FHTNPlanStep& Step = Level.Steps[StepIndex];

			UE_CVLOG_UELOG(Step.SubNodesInfo.bSubNodesExecuting,
				VisLogOwner, LogHTN, Error,
				TEXT("Plan Step '%s' (level %d step %d) unexpectedly has subnodes executing"),
				*DescribeStep(StepID),
				StepID.LevelIndex, StepID.StepIndex);

			UE_CVLOG_UELOG(Step.SubNodesInfo.LastFrameSubNodesTicked.IsSet(),
				VisLogOwner, LogHTN, Error,
				TEXT("Plan Step '%s' (level %d step %d) unexpectedly has subnodes ticked %d frames ago"),
				*DescribeStep(StepID),
				StepID.LevelIndex, StepID.StepIndex,
				GFrameNumber - *Step.SubNodesInfo.LastFrameSubNodesTicked);
		}
	}
#endif
}

const FHTNPlanStep& FHTNPlan::GetStep(const FHTNPlanStepID& PlanStepID) const
{
	return const_cast<FHTNPlan*>(this)->GetStep(PlanStepID);
}

FHTNPlanStep& FHTNPlan::GetStep(const FHTNPlanStepID& PlanStepID)
{
	check(HasLevel(PlanStepID.LevelIndex));
	FHTNPlanLevel& Level = *Levels[PlanStepID.LevelIndex];
	check(Level.Steps.IsValidIndex(PlanStepID.StepIndex));
	return Level.Steps[PlanStepID.StepIndex];
}

const FHTNPlanStep* FHTNPlan::FindStep(const FHTNPlanStepID& PlanStepID) const
{
	return const_cast<FHTNPlan*>(this)->FindStep(PlanStepID);
}

FHTNPlanStep* FHTNPlan::FindStep(const FHTNPlanStepID& PlanStepID)
{
	if (HasLevel(PlanStepID.LevelIndex))
	{
		FHTNPlanLevel& Level = *Levels[PlanStepID.LevelIndex];
		if (Level.Steps.IsValidIndex(PlanStepID.StepIndex))
		{
			return &Level.Steps[PlanStepID.StepIndex];
		}
	}
	
	return nullptr;
}

bool FHTNPlan::HasStep(const FHTNPlanStepID& StepID, int32 LevelIndex) const
{
	if (!HasLevel(StepID.LevelIndex) || !HasLevel(LevelIndex))
	{
		return false;
	}

	if (StepID.LevelIndex == LevelIndex)
	{
		return true;
	}

	if (StepID.LevelIndex == 0)
	{
		return false;
	}
	
	const FHTNPlanStepID& ParentStepID = Levels[StepID.LevelIndex]->ParentStepID;
	return HasStep(ParentStepID, LevelIndex);
}

bool FHTNPlan::IsSecondaryParallelStep(const FHTNPlanStepID& StepID) const
{
	if (!ensure(HasLevel(StepID.LevelIndex)))
	{
		return false;
	}

	FHTNPlanStepID CurrentStepID = StepID;
	while (CurrentStepID != FHTNPlanStepID::None)
	{
		const FHTNPlanStepID ParentStepID = Levels[CurrentStepID.LevelIndex]->ParentStepID;
		if (ParentStepID == FHTNPlanStepID::None)
		{
			break;
		}

		const FHTNPlanStep& ParentStep = GetStep(ParentStepID);
		if (Cast<UHTNNode_Parallel>(ParentStep.Node))
		{
			return CurrentStepID.LevelIndex == ParentStep.SecondarySubLevelIndex;
		}
		
		CurrentStepID = ParentStepID;
	}

	return false;
}

void FHTNPlan::GetSubNodesAtPlanStep(const FHTNPlanStepID& StepID, FHTNSubNodeGroups& OutSubNodeGroups, bool bOnlyStarting, bool bOnlyEnding) const
{
	if (!ensure(HasStep(StepID)))
	{
		return;
	}
	
	FHTNPlanStepID CurrentStepID = StepID;
	while (true)
	{
		FHTNPlanLevel& Level = *Levels[CurrentStepID.LevelIndex];
		FHTNPlanStep& Step = Level.Steps[CurrentStepID.StepIndex];
		OutSubNodeGroups.Add(FHTNSubNodeGroup(Step.SubNodesInfo, CurrentStepID,
			Step.bIsIfNodeFalseBranch, Step.bCanConditionsInterruptTrueBranch, Step.bCanConditionsInterruptFalseBranch));

		if ((!bOnlyStarting || CurrentStepID.StepIndex == 0) && (!bOnlyEnding || CurrentStepID.StepIndex == Level.Steps.Num() - 1))
		{
			OutSubNodeGroups.Emplace(Level.RootSubNodesInfo, Level.ParentStepID);

			if (CurrentStepID.LevelIndex > 0)
			{
				CurrentStepID = Level.ParentStepID;
				continue;
			}
		}

		break;
	}
}

bool FHTNPlan::FindExecutingSubNodeGroupWithDecoratorWithMemoryOffset(FHTNSubNodeGroup& OutSubNodeGroup, uint16 MemoryOffset, FHTNPlanStepID UnderPlanStepID) const
{
	const auto Match = [&](const FHTNRuntimeSubNodesInfo& SubNodesInfo) -> bool
	{
		return Algo::FindBy(SubNodesInfo.DecoratorInfos, MemoryOffset, &THTNNodeInfo<UHTNDecorator>::NodeMemoryOffset) != nullptr;
	};

	if (UnderPlanStepID.StepIndex == INDEX_NONE && HasLevel(UnderPlanStepID.LevelIndex))
	{
		FHTNPlanLevel& Level = *Levels[UnderPlanStepID.LevelIndex];

		if (Level.RootSubNodesInfo.bSubNodesExecuting)
		{
			// Find in root subnodes of the level.
			if (Match(Level.RootSubNodesInfo))
			{
				OutSubNodeGroup = FHTNSubNodeGroup(Level.RootSubNodesInfo, Level.ParentStepID);
				return true;
			}

			// Find in the steps of the level recursively.
			for (int32 StepIndex = 0; StepIndex < Level.Steps.Num(); ++StepIndex)
			{
				if (FindExecutingSubNodeGroupWithDecoratorWithMemoryOffset(OutSubNodeGroup, MemoryOffset, { UnderPlanStepID.LevelIndex, StepIndex }))
				{
					return true;
				}
			}
		}
	}
	else if (const FHTNPlanStep* const Step = FindStep(UnderPlanStepID))
	{
		if (Step->SubNodesInfo.bSubNodesExecuting)
		{
			// Find in subnodes of the step itself.
			if (Match(Step->SubNodesInfo))
			{
				OutSubNodeGroup = FHTNSubNodeGroup(Step->SubNodesInfo, UnderPlanStepID,
					Step->bIsIfNodeFalseBranch,
					Step->bCanConditionsInterruptTrueBranch,
					Step->bCanConditionsInterruptFalseBranch);
				return true;
			}

			// Find in primary sublevel of the step.
			if (FindExecutingSubNodeGroupWithDecoratorWithMemoryOffset(OutSubNodeGroup, MemoryOffset, { Step->SubLevelIndex, INDEX_NONE }))
			{
				return true;
			}

			// Find in secondary sublevel of the step.
			if (FindExecutingSubNodeGroupWithDecoratorWithMemoryOffset(OutSubNodeGroup, MemoryOffset, { Step->SecondarySubLevelIndex, INDEX_NONE }))
			{
				return true;
			}
		}
	}

	return false;
}

int32 FHTNPlan::GetFirstSubLevelWithTasksOfStep(const FHTNPlanStepID& StepID) const
{
	if (const FHTNPlanStep* const Step = FindStep(StepID))
	{
		if (HasTasksInLevel(Step->SubLevelIndex))
		{
			return Step->SubLevelIndex;
		}

		if (HasTasksInLevel(Step->SecondarySubLevelIndex))
		{
			return Step->SecondarySubLevelIndex;
		}
	}

	return INDEX_NONE;
}

int32 FHTNPlan::GetLastSubLevelWithTasksOfStep(const FHTNPlanStepID& StepID) const
{
	if (const FHTNPlanStep* const Step = FindStep(StepID))
	{
		if (HasTasksInLevel(Step->SecondarySubLevelIndex))
		{
			return Step->SecondarySubLevelIndex;
		}

		if (HasTasksInLevel(Step->SubLevelIndex))
		{
			return Step->SubLevelIndex;
		}
	}

	return INDEX_NONE;
}

bool FHTNPlan::HasTasksInLevel(int32 LevelIndex) const
{
	if (HasLevel(LevelIndex))
	{
		return Algo::AnyOf(Levels[LevelIndex]->Steps, [&](const FHTNPlanStep& Step)
		{
			return Cast<UHTNTask>(Step.Node) || HasTasksInLevel(Step.SubLevelIndex) || HasTasksInLevel(Step.SecondarySubLevelIndex);
		});
	}

	return false;
}

TSharedPtr<const FBlackboardWorldState> FHTNPlan::GetWorldstateBeforeDecoratorPlanEnter(const UHTNDecorator& Decorator, const FHTNPlanStepID& ActiveStepID) const
{
	const FHTNPlanStepID StartStepID = FindDecoratorStartStepID(Decorator, ActiveStepID);
	if (HasLevel(StartStepID.LevelIndex))
	{
		const FHTNPlanLevel& Level = *Levels[StartStepID.LevelIndex];
		return StartStepID.StepIndex > 0 ?
			Level.Steps[StartStepID.StepIndex - 1].WorldState :
			Level.WorldStateAtLevelStart;
	}

	return nullptr;
}

FHTNPlanStepID FHTNPlan::FindDecoratorStartStepID(const UHTNDecorator& Decorator, const FHTNPlanStepID& ActiveStepID) const
{
	if (!Levels.IsValidIndex(ActiveStepID.LevelIndex))
	{
		return FHTNPlanStepID::None;
	}

	const UHTNNode* const TemplateDecorator = Decorator.GetTemplateNode();

	if (ActiveStepID.StepIndex == INDEX_NONE)
	{
		if (Levels[ActiveStepID.LevelIndex]->GetRootDecoratorTemplates().Contains(TemplateDecorator))
		{
			return ActiveStepID;
		}
	}
	else if (const FHTNPlanStep* const Step = FindStep(ActiveStepID))
	{
		if (const UHTNStandaloneNode* const Node = Step->Node.Get())
		{
			if (Node->Decorators.Contains(TemplateDecorator))
			{
				return ActiveStepID;
			}
		}
	}

	FHTNSubNodeGroups SubNodeGroups;
	GetSubNodesAtPlanStep(ActiveStepID, SubNodeGroups);
	for (const FHTNSubNodeGroup& Group : SubNodeGroups)
	{
		if (Group.SubNodesInfo->DecoratorInfos.ContainsByPredicate([&](const THTNNodeInfo<UHTNDecorator>& Info) { return Info.TemplateNode == TemplateDecorator; }))
		{
			return Group.PlanStepID;
		}
	}

	return FHTNPlanStepID::None;
}

int32 FHTNPlan::GetRecursionCount(UHTNNode* Node) const
{
	if (RecursionCounts.IsValid())
	{
		if (const int32* const FoundCount = RecursionCounts->Find(Node))
		{
			return *FoundCount;
		}
	}

	return 0;
}

void FHTNPlan::IncrementRecursionCount(UHTNNode* Node)
{
	if (!RecursionCounts.IsValid())
	{
		RecursionCounts = MakeShared<TMap<TWeakObjectPtr<UHTNNode>, int32>>();
		RecursionCounts->Add(Node, 1);
	}
	else
	{
		RecursionCounts = MakeShared<TMap<TWeakObjectPtr<UHTNNode>, int32>>(*RecursionCounts);
		int32& Count = RecursionCounts->FindOrAdd(Node, 0);
		Count += 1;
	}
}

TSharedRef<FHTNPlanLevel> FHTNPlanLevel::DummyPlanLevel = MakeShared<FHTNPlanLevel>(nullptr, nullptr, FHTNPlanStepID::None, false, true);

TArrayView<TObjectPtr<UHTNDecorator>> FHTNPlanLevel::GetRootDecoratorTemplates() const
{
	if (!IsInlineLevel() && HTNAsset.IsValid())
	{
		return HTNAsset->RootDecorators;
	}

	return {};
}

TArrayView<TObjectPtr<UHTNService>> FHTNPlanLevel::GetRootServiceTemplates() const
{
	if (!IsInlineLevel() && HTNAsset.IsValid())
	{
		return HTNAsset->RootServices;
	}

	return {};
}

void FHTNPlanLevel::InitializeFromAsset(UHTN& Asset) const
{
	for (UHTNDecorator* const RootDecorator : GetRootDecoratorTemplates())
	{
		if (ensure(RootDecorator))
		{
			RootDecorator->InitializeFromAsset(Asset);
		}
	}

	for (UHTNService* const RootService : GetRootServiceTemplates())
	{
		if (ensure(RootService))
		{
			RootService->InitializeFromAsset(Asset);
		}
	}
}

FHTNGetNextStepsContext::FHTNGetNextStepsContext(const UHTNPlanInstance& PlanInstance,
	const FHTNPlan& Plan, bool bIsExecutingPlan,
	TArray<FHTNPlanStepID>& OutStepIds) : 
	OwnerComp(*PlanInstance.GetOwnerComponent()),
	PlanInstance(PlanInstance),
	Plan(Plan),
	bIsExecutingPlan(bIsExecutingPlan),
	OutStepIds(OutStepIds),
	NumSubmittedSteps(0)
{}

void FHTNGetNextStepsContext::SubmitPlanStep(const FHTNPlanStepID& PlanStepID)
{
	OutStepIds.Add(PlanStepID);
	++NumSubmittedSteps;
}

int32 FHTNGetNextStepsContext::AddNextPrimitiveStepsAfter(const FHTNPlanStepID& InStepID)
{
	if (!Plan.HasLevel(InStepID.LevelIndex))
	{
		return 0;
	}

	const FHTNPlanLevel& Level = *Plan.Levels[InStepID.LevelIndex];

	const auto GetNumStepsSubmittedNow = [OldNumSubmittedSteps = NumSubmittedSteps, this]()
	{
		return NumSubmittedSteps - OldNumSubmittedSteps;
	};

	// Check steps of this plan and steps in their sublevels, recursively
	for (int32 StepIndex = InStepID.StepIndex + 1; StepIndex < Level.Steps.Num(); ++StepIndex)
	{
		const FHTNPlanStep& CandidateStep = Level.Steps[StepIndex];
		CandidateStep.Node->GetNextPrimitiveSteps(*this, {InStepID.LevelIndex, StepIndex});
		if (const int32 NumSubmittedNow = GetNumStepsSubmittedNow())
		{
			return NumSubmittedNow;
		}
	}

	// Level finished, check steps of the parent level, recursively.
	if (Level.ParentStepID != FHTNPlanStepID::None)
	{
		const FHTNPlanStep& ParentStep = Plan.GetStep(Level.ParentStepID);
		ParentStep.Node->GetNextPrimitiveSteps(*this, Level.ParentStepID, InStepID.LevelIndex);
	}

	const int32 NumSubmittedNow = GetNumStepsSubmittedNow();
	return NumSubmittedNow;
}

int32 FHTNGetNextStepsContext::AddFirstPrimitiveStepsInLevel(int32 LevelIndex)
{
	return AddNextPrimitiveStepsAfter({ LevelIndex, INDEX_NONE });
}

int32 FHTNGetNextStepsContext::AddFirstPrimitiveStepsInAnySublevelOf(const FHTNPlanStepID& StepID)
{
	if (const FHTNPlanStep* const Step = Plan.FindStep(StepID))
	{
		if (const int32 AddedFromTopBranch = AddFirstPrimitiveStepsInLevel(Step->SubLevelIndex))
		{
			return AddedFromTopBranch;
		}

		return AddFirstPrimitiveStepsInLevel(Step->SecondarySubLevelIndex);
	}

	return 0;
}
