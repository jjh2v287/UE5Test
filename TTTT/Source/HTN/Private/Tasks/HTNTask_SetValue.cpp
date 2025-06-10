// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_SetValue.h"
#include "VisualLogger/VisualLogger.h"

UHTNTask_SetValue::UHTNTask_SetValue(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = TEXT("Set Value");
	bShowTaskNameOnCurrentPlanVisualization = false;
}

void UHTNTask_SetValue::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	if (!BlackboardKey.SelectedKeyType)
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("Blackboard Key %s has invalid type. Task %s can't plan."),
			*BlackboardKey.SelectedKeyName.ToString(), *GetShortDescription());
		return;
	}

	const TSharedRef<FBlackboardWorldState> WorldStateAfterTask = WorldState->MakeNext();
	if (Value.SetValue(*WorldStateAfterTask, BlackboardKey))
	{
		PlanningTask.SubmitPlanStep(this, WorldStateAfterTask, 0);
	}
	else
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s could not set worldstate key %s to (%s)"),
			*GetShortDescription(),
			*BlackboardKey.SelectedKeyName.ToString(),
			*Value.GetValueDescription(GetBlackboardAsset(), BlackboardKey.GetSelectedKeyID()));
	}
}

FString UHTNTask_SetValue::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: Set %s to %s"),
		*Super::GetStaticDescription(),
		*BlackboardKey.SelectedKeyName.ToString(),
		*Value.GetValueDescription(GetBlackboardAsset(), BlackboardKey.GetSelectedKeyID()));
}
