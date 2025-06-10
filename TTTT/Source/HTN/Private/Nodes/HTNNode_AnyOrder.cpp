// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_AnyOrder.h"
#include "AITask_MakeHTNPlan.h"
#include "VisualLogger/VisualLogger.h"

void UHTNNode_AnyOrder::MakePlanExpansions(FHTNPlanningContext& Context)
{
	const auto MakeNewPlan = [&](bool bInversedSubLevelOrder)
	{
		FHTNPlanStep* AddedStep = nullptr;
		FHTNPlanStepID AddedStepID;
		const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);

		AddedStep->bAnyOrderInversed = bInversedSubLevelOrder;
		AddedStep->SubLevelIndex = bInversedSubLevelOrder ? 
			AddInlineSecondaryLevel(Context, *NewPlan, AddedStepID) : 
			AddInlinePrimaryLevel(Context, *NewPlan, AddedStepID);
		AddedStep->SecondarySubLevelIndex = bInversedSubLevelOrder ? 
			AddInlinePrimaryLevel(Context, *NewPlan, AddedStepID) : 
			AddInlineSecondaryLevel(Context, *NewPlan, AddedStepID);
		
		if (AddedStep->SubLevelIndex != INDEX_NONE && AddedStep->SecondarySubLevelIndex != INDEX_NONE)
		{
			NewPlan->Levels[AddedStep->SecondarySubLevelIndex]->WorldStateAtLevelStart.Reset();
		}

		static const FString BottomFirstDescription = TEXT("bottom branch first");
		static const FString TopFirstDescription = TEXT("top branch first");
		FString Description;
		UE_IFVLOG(Description = bInversedSubLevelOrder ? BottomFirstDescription : TopFirstDescription);
		Context.SubmitCandidatePlan(NewPlan, Description);
	};

	// If the planning is trying to adjust an existing plan, 
	// this node should plan branches in the same order it did last time instead of trying something new.
	if (const FHTNPlanStep* const StepToReplace = Context.GetExistingPlanStepToAdjust())
	{
		MakeNewPlan(StepToReplace->bAnyOrderInversed);
	}
	else
	{
		MakeNewPlan(false);
		MakeNewPlan(true);
	}
}

void UHTNNode_AnyOrder::GetNextNodes(FHTNNextNodesBuffer& OutNextNodes, const FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex)
{
	const FHTNPlanStep* const ThisStep = Plan.FindStep(ThisStepID);
	if (ensure(ThisStep))
	{
		const bool bIsPrimaryBranch = ThisStep->SubLevelIndex == SubLevelIndex;
		const bool bEffectivePrimaryBranch = ThisStep->bAnyOrderInversed ? !bIsPrimaryBranch : bIsPrimaryBranch;
		OutNextNodes = bEffectivePrimaryBranch ? GetPrimaryNextNodes() : GetSecondaryNextNodes();
	}
}

bool UHTNNode_AnyOrder::OnSubLevelFinishedPlanning(FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex, 
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

void UHTNNode_AnyOrder::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);

	const int32 AddedFromTopBranch = Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	if (AddedFromTopBranch == 0)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SecondarySubLevelIndex);
	}
}

void UHTNNode_AnyOrder::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex)
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
