// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_ClearValue.h"
#include "VisualLogger/VisualLogger.h"

UHTNTask_ClearValue::UHTNTask_ClearValue(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = TEXT("Clear Value");
	bShowTaskNameOnCurrentPlanVisualization = false;
}

void UHTNTask_ClearValue::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	if (!BlackboardKey.SelectedKeyType)
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("Blackboard Key %s has invalid type. Task %s can't plan."),
			*BlackboardKey.SelectedKeyName.ToString(), *GetShortDescription());
		return;
	}

	const TSharedRef<FBlackboardWorldState> WorldStateAfterTask = WorldState->MakeNext();
	WorldStateAfterTask->ClearValue(BlackboardKey.GetSelectedKeyID());
	PlanningTask.SubmitPlanStep(this, WorldStateAfterTask, 0);
}

FString UHTNTask_ClearValue::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: Clear %s"),
		*Super::GetStaticDescription(),
		*BlackboardKey.SelectedKeyName.ToString());
}
