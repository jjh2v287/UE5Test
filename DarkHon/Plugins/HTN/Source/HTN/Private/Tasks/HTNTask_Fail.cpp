// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_Fail.h"
#include "VisualLogger/VisualLogger.h"
#include "Misc/StringBuilder.h"

UHTNTask_Fail::UHTNTask_Fail(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bFailDuringExecution(false)
{}

void UHTNTask_Fail::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const 
{
	if (bFailDuringExecution)
	{
		PlanningTask.SubmitPlanStep(this, WorldState->MakeNext(), 0, TEXT("Will fail during execution"));
	}
	else
	{
		PlanningTask.SetNodePlanningFailureReason(GetFailureMessage());
		// Not submitting any plan steps means failure.
	}
}

EHTNNodeResult UHTNTask_Fail::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	UE_VLOG(&OwnerComp, LogHTN, Log, TEXT("Fail: %s"), *FailureMessage);
	return EHTNNodeResult::Failed;
}

FString UHTNTask_Fail::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	return bFailDuringExecution ? TEXT("Fail during execution") : TEXT("Fail");
}

FString UHTNTask_Fail::GetStaticDescription() const
{
	TStringBuilder<1024> SB;

	SB << Super::GetStaticDescription() << TEXT(": ");

	if (bFailDuringExecution)
	{
		SB << TEXT("Fails during execution");
	}
	else
	{
		SB << TEXT("Fails during planning");
	}

	SB << TEXT("\nLog message: ") << GetFailureMessage();

	return SB.ToString();
}

FString UHTNTask_Fail::GetFailureMessage() const
{
	if (FailureMessage.Len())
	{
		return FailureMessage;
	}

	return *GetNodeName();
}
