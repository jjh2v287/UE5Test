// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_Scope.h"
#include "AITask_MakeHTNPlan.h"
#include "HTNDecorator.h"
#include "Misc/StringBuilder.h"

UHTNNode_Scope::UHTNNode_Scope(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bPlanNextNodesAfterThis = false;
}

FString UHTNNode_Scope::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	TStringBuilder<2048> SB;
	SB << TEXT("Scope");

	int32 I = 0;
	for (UHTNDecorator* const Decorator : Decorators)
	{
		if (IsValid(Decorator))
		{
			SB << FStringView(I++ == 0 ? TEXT(" ") : TEXT(", "));
			SB << Decorator->GetNodeName();
		}
	}

	return *SB;
}

FString UHTNNode_Scope::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s:\nScope for decorators and services."), *Super::GetStaticDescription());
}

void UHTNNode_Scope::MakePlanExpansions(FHTNPlanningContext& Context)
{
	FHTNPlanStep* AddedStep = nullptr;
	FHTNPlanStepID AddedStepID;
	const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
	AddedStep->SubLevelIndex = NextNodes.Num() ? Context.AddInlineLevel(*NewPlan, AddedStepID) : INDEX_NONE;
	Context.SubmitCandidatePlan(NewPlan);
}

void UHTNNode_Scope::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	if (Step.SubLevelIndex != INDEX_NONE)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	}
}