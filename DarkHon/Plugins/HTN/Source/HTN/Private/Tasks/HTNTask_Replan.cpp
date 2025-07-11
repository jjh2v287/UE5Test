// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_Replan.h"
#include "VisualLogger/VisualLogger.h"
#include "Misc/StringBuilder.h"

UHTNTask_Replan::UHTNTask_Replan(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Because the most common expected use case of this task is to force-replan the subplan we're in regardless of its settings.
	Parameters.bForceReplan = true;
}

void UHTNTask_Replan::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	PlanningTask.SubmitPlanStep(this, WorldState->MakeNext(), 0);
}

EHTNNodeResult UHTNTask_Replan::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	if (UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstanceByNodeMemory(NodeMemory))
	{
		PlanInstance->Replan(Parameters);
		return EHTNNodeResult::Succeeded;
	}
	else
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, 
			TEXT("%s could not find plan instance to replan"), 
			*GetShortDescription());
		return EHTNNodeResult::Failed;
	}
}

FString UHTNTask_Replan::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	return Parameters.ToTitleString();
}

FString UHTNTask_Replan::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription() << TEXT(":\n");
	SB << Parameters.ToDetailedString();

	return SB.ToString();
}
