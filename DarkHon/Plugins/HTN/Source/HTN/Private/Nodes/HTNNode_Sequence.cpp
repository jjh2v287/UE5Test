// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_Sequence.h"
#include "AITask_MakeHTNPlan.h"

void UHTNNode_Sequence::MakePlanExpansions(FHTNPlanningContext& Context)
{
	FHTNPlanStep* AddedStep = nullptr;
	FHTNPlanStepID AddedStepID;
	const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);

	AddedStep->SubLevelIndex = AddInlinePrimaryLevel(Context, *NewPlan, AddedStepID);
	AddedStep->SecondarySubLevelIndex = AddInlineSecondaryLevel(Context, *NewPlan, AddedStepID);
	
	if (AddedStep->SubLevelIndex != INDEX_NONE && AddedStep->SecondarySubLevelIndex != INDEX_NONE)
	{
		NewPlan->Levels[AddedStep->SecondarySubLevelIndex]->WorldStateAtLevelStart.Reset();
	}

	Context.SubmitCandidatePlan(NewPlan);
}

bool UHTNNode_Sequence::OnSubLevelFinishedPlanning(FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex,
	TSharedPtr<FBlackboardWorldState> WorldState)
{
	const FHTNPlanStep& Step = Plan.GetStep(ThisStepID);

	// First branch finished, set up the second one.
	if (SubLevelIndex == Step.SubLevelIndex && Step.SecondarySubLevelIndex != INDEX_NONE)
	{
		// Copy the second branch before modifying it, as it might be shared with other candidate plans.
		TSharedPtr<FHTNPlanLevel>& NextLevel = Plan.Levels[Step.SecondarySubLevelIndex];
		NextLevel = MakeShared<FHTNPlanLevel>(*NextLevel);
		
		NextLevel->WorldStateAtLevelStart = WorldState;
		return false;
	}

	return true;
}

void UHTNNode_Sequence::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);

	const int32 NumAddedFromTopBranch = Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	if (NumAddedFromTopBranch == 0)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SecondarySubLevelIndex);
	}
}

void UHTNNode_Sequence::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);

	int32 NumAdded = 0;
	if (FinishedSubLevelIndex == Step.SubLevelIndex)
	{
		NumAdded = Context.AddFirstPrimitiveStepsInLevel(Step.SecondarySubLevelIndex);
	}

	if (NumAdded == 0)
	{
		Super::GetNextPrimitiveSteps(Context, ThisStepID, FinishedSubLevelIndex);
	}
}
