// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_Parallel.h"
#include "AITask_MakeHTNPlan.h"

UHTNNode_Parallel::UHTNNode_Parallel(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bWaitForSecondaryBranchToComplete(false),
	bLoopSecondaryBranchUntilPrimaryBranchCompletes(false)
{}

void UHTNNode_Parallel::MakePlanExpansions(FHTNPlanningContext& Context)
{
	FHTNPlanStep* AddedStep = nullptr;
	FHTNPlanStepID AddedStepID;
	const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
	
	AddedStep->SubLevelIndex = AddInlinePrimaryLevel(Context, *NewPlan, AddedStepID);
	AddedStep->SecondarySubLevelIndex = AddInlineSecondaryLevel(Context, *NewPlan, AddedStepID);

	Context.SubmitCandidatePlan(NewPlan);
}

bool UHTNNode_Parallel::OnSubLevelFinishedPlanning(FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex, TSharedPtr<FBlackboardWorldState> WorldState)
{
	const FHTNPlanStep& Step = Plan.GetStep(ThisStepID);
	if (Step.SecondarySubLevelIndex == SubLevelIndex && Step.SubLevelIndex != INDEX_NONE)
	{
		// Parallel secondaries do not contribute to worldstate and do not trigger plan-time decorator exits, at least for now.
		// However, they do that if there is no primary branch.
		return false;
	}

	return true;
}

void UHTNNode_Parallel::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);

	Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	
	// If the primary branch doesn't have any tasks, don't do any of the tasks in the secondary branch.
	// Unless bWaitForSecondaryBranchToComplete is true, the secondary branch would need to be aborted immediately anyway.
	// So better not to begin executing it at all.
	if (bWaitForSecondaryBranchToComplete || Context.Plan.HasTasksInLevel(Step.SubLevelIndex))
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SecondarySubLevelIndex);
	}
}

bool UHTNNode_Parallel::OnSubLevelFinished(UHTNPlanInstance& PlanInstance, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex)
{
	const FHTNPlan& Plan = *PlanInstance.GetCurrentPlan();
	const FHTNPlanStep& Step = Plan.GetStep(ThisStepID);
	FNodeMemory* const Memory = CastInstanceNodeMemory<FNodeMemory>(PlanInstance.GetNodeMemory(Step.NodeMemoryOffset));

	check(!Memory->bIsExecutionComplete);
	if (FinishedSubLevelIndex == Step.SubLevelIndex)
	{
		Memory->bIsPrimaryBranchComplete = true;

		// If we're looping the secondary branch, then it doesn't matter if it's completed in the past.
		// The !bLoopSecondaryBranchUntilPrimaryBranchCompletes prevents GetNextPrimitiveSteps from being called a second time when
		// the secondary branch completes its loop once more. This prevents the tasks after the Parallel being started twice.
		Memory->bIsExecutionComplete = 
			(!bWaitForSecondaryBranchToComplete || !Plan.HasTasksInLevel(Step.SecondarySubLevelIndex)) ||
			(Memory->bIsSecondaryBranchComplete && !bLoopSecondaryBranchUntilPrimaryBranchCompletes);
	}
	else if (ensure(FinishedSubLevelIndex == Step.SecondarySubLevelIndex))
	{
		Memory->bIsSecondaryBranchComplete = true;
		Memory->bIsExecutionComplete = Memory->bIsPrimaryBranchComplete;
	}

	return Memory->bIsExecutionComplete;
}

void UHTNNode_Parallel::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex)
{
	if (!Context.bIsExecutingPlan)
	{
		Super::GetNextPrimitiveSteps(Context, ThisStepID, FinishedSubLevelIndex);
		return;
	}
	
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	const FNodeMemory* const Memory = CastInstanceNodeMemory<FNodeMemory>(Context.PlanInstance.GetNodeMemory(Step.NodeMemoryOffset));
	if (Memory->bIsExecutionComplete)
	{
		Super::GetNextPrimitiveSteps(Context, ThisStepID, FinishedSubLevelIndex);
	}
	else if (FinishedSubLevelIndex == Step.SecondarySubLevelIndex)
	{
		if (bLoopSecondaryBranchUntilPrimaryBranchCompletes && !Memory->bSecondaryBranchReentryFlag)
		{
			FGuardValue_Bitfield(Memory->bSecondaryBranchReentryFlag, true);
			Context.AddFirstPrimitiveStepsInLevel(FinishedSubLevelIndex);
		}
	}
}

void UHTNNode_Parallel::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	new(NodeMemory) FNodeMemory;
}

UHTNNode_Parallel::FNodeMemory::FNodeMemory() :
	bIsPrimaryBranchComplete(false),
	bIsSecondaryBranchComplete(false),
	bIsExecutionComplete(false),
	bSecondaryBranchReentryFlag(false)
{}

FString UHTNNode_Parallel::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s%s%s"),
		*Super::GetStaticDescription(),
		bWaitForSecondaryBranchToComplete ? TEXT("\n(waits for secondary branch to complete)" : TEXT("")),
		bLoopSecondaryBranchUntilPrimaryBranchCompletes ? TEXT("\n(loops secondary branch until primary branch completes)" : TEXT("")));
}

#if WITH_EDITOR
FName UHTNNode_Parallel::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Composite.SimpleParallel.Icon");
}
#endif
