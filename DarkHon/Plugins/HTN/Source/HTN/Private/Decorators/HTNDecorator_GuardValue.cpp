// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_GuardValue.h"
#include "AIController.h"
#include "AITask_MakeHTNPlan.h"
#include "VisualLogger/VisualLogger.h"
#include "HTNComponent.h"
#include "WorldStateProxy.h"

UHTNDecorator_GuardValue::UHTNDecorator_GuardValue(const FObjectInitializer& Initializer) : Super(Initializer),
	bSetValueOnEnterPlan(true),
	bRestoreValueOnExitPlan(true),
	bRestoreValueOnAbort(true)
{
	NodeName = TEXT("Guard Value");

	bCheckConditionOnPlanEnter = false;
	bCheckConditionOnPlanExit = false;
	bCheckConditionOnPlanRecheck = false;
	bCheckConditionOnTick = false;

	bNotifyOnEnterPlan = true;
	bNotifyOnExitPlan = true;
	bNotifyExecutionFinish = true;

	bNotifyOnBlackboardKeyValueChange = false;
}

uint16 UHTNDecorator_GuardValue::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}

void UHTNDecorator_GuardValue::OnEnterPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	if (!BlackboardKey.SelectedKeyType)
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("Blackboard Key %s has invalid type! Decorator %s can't plan."),
			*BlackboardKey.SelectedKeyName.ToString(), *GetShortDescription());
		return;
	}

	if (bSetValueOnEnterPlan)
	{
		if (!Value.SetValue(*OwnerComp.GetPlanningWorldStateProxy()->GetWorldState(), BlackboardKey))
		{
			UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s could not set worldstate key %s to (%s)"),
				*GetShortDescription(),
				*BlackboardKey.SelectedKeyName.ToString(),
				*Value.GetValueDescription(GetBlackboardAsset(), BlackboardKey.GetSelectedKeyID()));
		}
	}
}

void UHTNDecorator_GuardValue::OnExitPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	if (bRestoreValueOnExitPlan)
	{
		RestoreWorldstateValue(OwnerComp, *OwnerComp.GetPlanningWorldStateProxy(), Plan, StepID);
	}
}

void UHTNDecorator_GuardValue::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	FNodeMemory& Memory = *(new(NodeMemory) FNodeMemory);
	Memory.PlanStepID = StepID;
}

void UHTNDecorator_GuardValue::OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{
	Super::OnExecutionFinish(OwnerComp, NodeMemory, Result);

	const FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	if (bRestoreValueOnAbort && Result == EHTNNodeResult::Aborted)
	{
		if (const UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstanceByNodeMemory(NodeMemory))
		{
			if (const TSharedPtr<const FHTNPlan> CurrentPlan = PlanInstance->GetCurrentPlan())
			{
				RestoreWorldstateValue(OwnerComp, *OwnerComp.GetBlackboardProxy(), *CurrentPlan, Memory.PlanStepID);
			}
		}
	}
}

FString UHTNDecorator_GuardValue::GetStaticDescription() const
{
	const FString SetDescription = bSetValueOnEnterPlan ?
		FString::Printf(TEXT("\nSet to %s (on plan enter)"), *Value.GetValueDescription(GetBlackboardAsset(), BlackboardKey.GetSelectedKeyID())) :
		FString();

	TArray<FString, TInlineAllocator<2>> RestorePointDescriptions;
	if (bRestoreValueOnExitPlan)
	{
		RestorePointDescriptions.Add(TEXT("on plan exit"));
	}
	if (bRestoreValueOnAbort)
	{
		RestorePointDescriptions.Add(TEXT("on abort"));
	}
	const FString RestoreDescription = RestorePointDescriptions.Num() ?
		FString::Printf(TEXT("\nRestore to original value (%s)"), *FString::Join(RestorePointDescriptions, TEXT(", "))) :
		FString();

	return FString::Printf(TEXT("%s: %s%s%s"),
		*Super::GetStaticDescription(),
		*BlackboardKey.SelectedKeyName.ToString(),
		*SetDescription,
		*RestoreDescription);
}

bool UHTNDecorator_GuardValue::RestoreWorldstateValue(UHTNComponent& OwnerComp, UWorldStateProxy& WorldStateProxy, const FHTNPlan& CurrentPlan, const FHTNPlanStepID& StepIDWithDecorator) const
{
	const TSharedPtr<const FBlackboardWorldState> WorldStateBeforeEntered = CurrentPlan.GetWorldstateBeforeDecoratorPlanEnter(*this, StepIDWithDecorator);
	if (WorldStateBeforeEntered.IsValid())
	{
		if (!WorldStateProxy.CopyValueFrom(*WorldStateBeforeEntered, BlackboardKey.GetSelectedKeyID()))
		{
			UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s could not restore worldstate key %s"),
				*GetShortDescription(),
				*BlackboardKey.SelectedKeyName.ToString());
		}
		return true;
	}
	
	UE_VLOG_UELOG(OwnerComp.GetAIOwner(), LogHTN, Error, TEXT("Could not find worldstate before decorator %s"), *GetShortDescription());
	return false;
}
