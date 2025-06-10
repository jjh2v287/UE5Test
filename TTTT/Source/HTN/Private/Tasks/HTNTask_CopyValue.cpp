// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_CopyValue.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNTask_CopyValue::UHTNTask_CopyValue(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowTaskNameOnCurrentPlanVisualization = false;
}

void UHTNTask_CopyValue::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* const BBAsset = GetBlackboardAsset())
	{
		SourceKey.ResolveSelectedKey(*BBAsset);
		TargetKey.ResolveSelectedKey(*BBAsset);
	}
	else
	{
		UE_LOG(LogHTN, Warning, TEXT("Can't initialize task: %s, make sure that the HTN specifies a Blackboard asset!"), *GetShortDescription());
	}
}

FString UHTNTask_CopyValue::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	return FString::Printf(TEXT("Copy %s to %s"), *SourceKey.SelectedKeyName.ToString(), *TargetKey.SelectedKeyName.ToString());
}

FString UHTNTask_CopyValue::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription() << TEXT(": from ") << SourceKey.SelectedKeyName.ToString() << TEXT(" to ") << TargetKey.SelectedKeyName.ToString();
	
	if (SourceKey.SelectedKeyType != TargetKey.SelectedKeyType)
	{
		SB << TEXT("\nWARNING: will fail because key types don't match:\n") <<
			*GetNameSafe(SourceKey.SelectedKeyType) << TEXT("\n") <<
			*GetNameSafe(TargetKey.SelectedKeyType);
	}

	return SB.ToString();
}

void UHTNTask_CopyValue::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	if (!SourceKey.SelectedKeyType)
	{
		PlanningTask.SetNodePlanningFailureReason(TEXT("Invalid source key"));
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s: Source Key %s has invalid type"),
			*GetShortDescription(), *SourceKey.SelectedKeyName.ToString());
		return;
	}

	if (!TargetKey.SelectedKeyType)
	{
		PlanningTask.SetNodePlanningFailureReason(TEXT("Invalid target key"));
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s: Target Key %s has invalid type"),
			*GetShortDescription(), *TargetKey.SelectedKeyName.ToString());
		return;
	}

	const TSharedRef<FBlackboardWorldState> WorldStateAfterTask = WorldState->MakeNext();

	const FBlackboard::FKey SourceKeyID = SourceKey.GetSelectedKeyID();
	const FBlackboard::FKey TargetKeyID = TargetKey.GetSelectedKeyID();
	if (WorldStateAfterTask->CopyKeyValue(SourceKeyID, TargetKeyID))
	{
		FString Description;
		UE_IFVLOG(Description = WorldStateAfterTask->DescribeKeyValue(TargetKeyID, EBlackboardDescription::OnlyValue));
		PlanningTask.SubmitPlanStep(this, WorldStateAfterTask, 0, Description);
	}
	else
	{
		PlanningTask.SetNodePlanningFailureReason(TEXT("Failed to copy"));
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s: could not copy from worldstate key %s to %s"),
			*GetShortDescription(),
			*SourceKey.SelectedKeyName.ToString(),
			*TargetKey.SelectedKeyName.ToString());
	}
}
