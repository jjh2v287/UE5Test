// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_RandomWeight.h"
#include "AITask_MakeHTNPlan.h"
#include "Misc/StringBuilder.h"

UHTNNode_RandomWeight::UHTNNode_RandomWeight(const FObjectInitializer& Initializer) : Super(Initializer),
	RandomWeight(1.0f)
{
	bPlanNextNodesAfterThis = false;
}

FString UHTNNode_RandomWeight::GetNodeName() const
{
	if (NodeName.Len())
	{
		return UHTNStandaloneNode::GetNodeName();
	}

	return FString::Printf(TEXT("Random Weight %s"), *FString::SanitizeFloat(RandomWeight));
}

FString UHTNNode_RandomWeight::GetStaticDescription() const
{
	TStringBuilder<2048> SB;
	SB << *Super::GetStaticDescription() << TEXT(": ") << FString::SanitizeFloat(RandomWeight);
	return SB.ToString();
}

void UHTNNode_RandomWeight::MakePlanExpansions(FHTNPlanningContext& Context)
{
	FHTNPlanStep* AddedStep = nullptr;
	FHTNPlanStepID AddedStepID;
	const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
	AddedStep->SubLevelIndex = NextNodes.Num() ? Context.AddInlineLevel(*NewPlan, AddedStepID) : INDEX_NONE;
	Context.SubmitCandidatePlan(NewPlan);
}

void UHTNNode_RandomWeight::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	if (Step.SubLevelIndex != INDEX_NONE)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	}
}
