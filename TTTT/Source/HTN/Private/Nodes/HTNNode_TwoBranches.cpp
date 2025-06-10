// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_TwoBranches.h"
#include "AITask_MakeHTNPlan.h"

UHTNNode_TwoBranches::UHTNNode_TwoBranches(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	NumPrimaryNodes(INDEX_NONE)
{
	bPlanNextNodesAfterThis = false;
}

void UHTNNode_TwoBranches::GetNextNodes(FHTNNextNodesBuffer& OutNextNodes, const FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex)
{
	const FHTNPlanStep* const ThisStep = Plan.FindStep(ThisStepID);
	if (ensure(ThisStep))
	{
		const bool bIsPrimaryBranch = ThisStep->SubLevelIndex == SubLevelIndex;
		OutNextNodes = bIsPrimaryBranch ? GetPrimaryNextNodes() : GetSecondaryNextNodes();
	}
}

bool UHTNNode_TwoBranches::OnSubLevelFinished(UHTNPlanInstance& PlanInstance, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex)
{
	const FHTNPlan& Plan = *PlanInstance.GetCurrentPlan();
	const int32 LastSubLevelIndex = Plan.GetLastSubLevelWithTasksOfStep(ThisStepID);
	return FinishedSubLevelIndex == LastSubLevelIndex;
}

TArrayView<TObjectPtr<UHTNStandaloneNode>> UHTNNode_TwoBranches::GetPrimaryNextNodes() const
{
	TArray<TObjectPtr<UHTNStandaloneNode>>& Nodes = const_cast<UHTNNode_TwoBranches*>(this)->NextNodes;

	if (!ensure(NumPrimaryNodes != INDEX_NONE))
	{
		return TArrayView<TObjectPtr<UHTNStandaloneNode>>(Nodes);
	}
	
	return NumPrimaryNodes ?
		TArrayView<TObjectPtr<UHTNStandaloneNode>>(Nodes).Slice(0, NumPrimaryNodes) :
		TArrayView<TObjectPtr<UHTNStandaloneNode>>(Nodes.GetData(), 0);
}

TArrayView<TObjectPtr<UHTNStandaloneNode>> UHTNNode_TwoBranches::GetSecondaryNextNodes() const
{
	TArray<TObjectPtr<UHTNStandaloneNode>>& Nodes = const_cast<UHTNNode_TwoBranches*>(this)->NextNodes;
	
	if (!ensure(NumPrimaryNodes != INDEX_NONE))
	{
		return TArrayView<TObjectPtr<UHTNStandaloneNode>>(Nodes.GetData(), 0);
	}
	
	const int32 NumSecondaryNodes = NextNodes.Num() - NumPrimaryNodes;
	return NumSecondaryNodes ? 
		TArrayView<TObjectPtr<UHTNStandaloneNode>>(Nodes).Slice(NumPrimaryNodes, NumSecondaryNodes) :
		TArrayView<TObjectPtr<UHTNStandaloneNode>>(Nodes.GetData(), 0);
}

int32 UHTNNode_TwoBranches::AddInlinePrimaryLevel(FHTNPlanningContext& Context, FHTNPlan& Plan, const FHTNPlanStepID& AddedStepID)
{
	return GetPrimaryNextNodes().Num() > 0 ? Context.AddInlineLevel(Plan, AddedStepID) : INDEX_NONE;
}

int32 UHTNNode_TwoBranches::AddInlineSecondaryLevel(FHTNPlanningContext& Context, FHTNPlan& Plan, const FHTNPlanStepID& AddedStepID)
{
	return GetSecondaryNextNodes().Num() > 0 ? Context.AddInlineLevel(Plan, AddedStepID) : INDEX_NONE;
}
