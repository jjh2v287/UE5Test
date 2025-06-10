// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNTask.h"
#include "AIController.h"
#include "GameplayTasksComponent.h"
#include "HTNPlan.h"
#include "VisualLogger/VisualLogger.h"
#include "WorldStateProxy.h"

namespace
{
	struct FHTNCurrentlyExecutingFunctionGuard : public TGuardValue<FName>
	{
		using Super = TGuardValue<FName>;
		FHTNCurrentlyExecutingFunctionGuard(FHTNTaskSpecialMemory& SpecialNodeMemory, FName Value) :
			Super(SpecialNodeMemory.NameOfCurrentlyExecutingFunctionDisallowingFinishLatentTask, Value)
		{}

		FHTNCurrentlyExecutingFunctionGuard(const UHTNNode& Node, uint8* NodeMemory, FName Value) :
			FHTNCurrentlyExecutingFunctionGuard(*Node.GetSpecialNodeMemory<FHTNTaskSpecialMemory>(NodeMemory), Value)
		{}
	};
}

UHTNTask::UHTNTask(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bShowTaskNameOnCurrentPlanVisualization(true),
	bProcessSubmittedPlanStepsInOrder(false),
	bNotifyTick(false),
	bNotifyTaskFinished(false)
{}

void UHTNTask::MakePlanExpansions(FHTNPlanningContext& Context)
{
	UAITask_MakeHTNPlan* const PlanningTask = Context.PlanningTask.Get();
	if (ensure(PlanningTask && PlanningTask->GetOwnerComponent() && Context.WorldStateAfterEnteringDecorators.IsValid()))
	{
		CreatePlanSteps(*PlanningTask->GetOwnerComponent(), *PlanningTask, Context.WorldStateAfterEnteringDecorators.ToSharedRef());
	}
}

uint16 UHTNTask::GetSpecialMemorySize() const { return sizeof(FHTNTaskSpecialMemory); }

void UHTNTask::InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	new(GetSpecialNodeMemory<FHTNTaskSpecialMemory>(NodeMemory)) FHTNTaskSpecialMemory;
}

bool UHTNTask::WrappedRecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, 
	const FBlackboardWorldState& WorldState, const FHTNPlanStep& SubmittedPlanStep) const
{
	check(!IsInstance());
	check(SubmittedPlanStep.Node == this);

	UHTNTask* const Task = StaticCast<UHTNTask*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Task))
	{
		return false;
	}

	static const FName RecheckPlanName(GET_FUNCTION_NAME_CHECKED(ThisClass, RecheckPlan));
	const FHTNCurrentlyExecutingFunctionGuard Guard(*this, NodeMemory, RecheckPlanName);
	return Task->RecheckPlan(OwnerComp, NodeMemory, WorldState, SubmittedPlanStep);
}

EHTNNodeResult UHTNTask::WrappedExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) const
{
	check(!IsInstance());
	UHTNTask* const Task = StaticCast<UHTNTask*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Task))
	{
		return EHTNNodeResult::Failed;
	}

	FHTNTaskSpecialMemory& SpecialMemory = *GetSpecialNodeMemory<FHTNTaskSpecialMemory>(NodeMemory);
	// Explicitly reset this in case this task is part of a looping branch of a Parallel node.
	// This prevents FinishLatentTask from warning that we're trying to finish a task that was already finished.
	SpecialMemory.ExecutionResult = EHTNNodeResult::InProgress;

	static const FName ExecuteTaskName(GET_FUNCTION_NAME_CHECKED(ThisClass, ExecuteTask));
	const FHTNCurrentlyExecutingFunctionGuard Guard(SpecialMemory, ExecuteTaskName);
	UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("execute task: '%s'"), *Task->GetShortDescription());
	const EHTNNodeResult Result = Task->ExecuteTask(OwnerComp, NodeMemory, PlanStepID);
	return Result;
}

EHTNNodeResult UHTNTask::WrappedAbortTask(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	check(!IsInstance());
	UHTNTask* const Task = StaticCast<UHTNTask*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Task))
	{
		return EHTNNodeResult::Aborted;
	}

	static const FName AbortTaskName(GET_FUNCTION_NAME_CHECKED(ThisClass, AbortTask));
	const FHTNCurrentlyExecutingFunctionGuard Guard(*this, NodeMemory, AbortTaskName);
	UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("abort task: '%s'"), *Task->GetShortDescription());
	const EHTNNodeResult Result = Task->AbortTask(OwnerComp, NodeMemory);
	return Result;
}

void UHTNTask::WrappedTickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) const
{
	check(!IsInstance());
	UHTNTask* const Task = StaticCast<UHTNTask*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (ensure(Task) && Task->bNotifyTick)
	{
		UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("tick task: '%s'"), *Task->GetShortDescription());
		Task->TickTask(OwnerComp, NodeMemory, DeltaTime);
	}
}

void UHTNTask::WrappedOnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) const
{
	check(!IsInstance());
	UHTNTask* const Task = StaticCast<UHTNTask*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (!ensure(Task))
	{
		return;
	}

	FHTNTaskSpecialMemory& SpecialMemory = *GetSpecialNodeMemory<FHTNTaskSpecialMemory>(NodeMemory);
	UE_CVLOG_UELOG(SpecialMemory.ExecutionResult != EHTNNodeResult::InProgress, &OwnerComp, LogHTN, Error,
		TEXT("%s: WrappedOnTaskFinished called (with %s) on a task that already finished (with %s)."),
		*GetShortDescription(),
		*StaticEnum<EHTNNodeResult>()->GetNameStringByValue(StaticCast<int64>(Result)),
		*StaticEnum<EHTNNodeResult>()->GetNameStringByValue(StaticCast<int64>(SpecialMemory.ExecutionResult)));
	SpecialMemory.ExecutionResult = Result;

	if (Task->bNotifyTaskFinished)
	{
		static const FName OnTaskFinishedName(GET_FUNCTION_NAME_CHECKED(ThisClass, OnTaskFinished));
		const FHTNCurrentlyExecutingFunctionGuard Guard(SpecialMemory, OnTaskFinishedName);
		UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("on task finished: '%s'"), *Task->GetShortDescription());
		Task->OnTaskFinished(OwnerComp, NodeMemory, Result);
	}
		
	// End gameplay tasks owned by the task.
	if (Task->bOwnsGameplayTasks)
	{
		if (const AAIController* const AIController = OwnerComp.GetAIOwner())
		{
			if (UGameplayTasksComponent* const GTComp = AIController->GetGameplayTasksComponent())
			{
				static const FName EndGameplayTasksOwnedByHTNTaskName(TEXT("EndGameplayTasksOwnedByHTNTask"));
				const FHTNCurrentlyExecutingFunctionGuard Guard(SpecialMemory, EndGameplayTasksOwnedByHTNTaskName);
				GTComp->EndAllResourceConsumingTasksOwnedBy(*Task);
			}
		}
	}
}

void UHTNTask::WrappedLogToVisualLog(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStep& SubmittedPlanStep) const
{
	check(!IsInstance());
	UHTNTask* const Task = StaticCast<UHTNTask*>(GetNodeFromMemory(OwnerComp, NodeMemory));
	if (ensure(Task))
	{
		Task->LogToVisualLog(OwnerComp, NodeMemory, SubmittedPlanStep);
	}
}

void UHTNTask::FinishLatentTask(UHTNComponent& OwnerComp, EHTNNodeResult TaskResult, const uint8* NodeMemory) const
{
	const FHTNNodeInPlanInfo TaskInfo = OwnerComp.FindActiveTaskInfo(this, NodeMemory);
	if (!TaskInfo)
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error,
			TEXT("%s: FinishLatentTask could not find info for task."),
			*GetShortDescription());
		return;
	}

	const FHTNTaskSpecialMemory& SpecialMemory = *GetSpecialNodeMemory<FHTNTaskSpecialMemory>(TaskInfo.NodeMemory);

	if (SpecialMemory.ExecutionResult != EHTNNodeResult::InProgress)
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Warning,
			TEXT("%s: FinishLatentTask called (with %s) on a task that already finished (with %s)."),
			*GetShortDescription(),
			*StaticEnum<EHTNNodeResult>()->GetNameStringByValue(StaticCast<int64>(TaskResult)),
			*StaticEnum<EHTNNodeResult>()->GetNameStringByValue(StaticCast<int64>(SpecialMemory.ExecutionResult)));
		return;
	}

	const FName ExecutingFunctionName = SpecialMemory.NameOfCurrentlyExecutingFunctionDisallowingFinishLatentTask;
	if (!ExecutingFunctionName.IsNone())
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Warning,
			TEXT("%s: FinishLatentTask called from inside %s. This is not legal and will be ignored."),
			*GetShortDescription(),
			*ExecutingFunctionName.ToString());
		return;
	}

	TaskInfo.PlanInstance->OnTaskFinished(this, TaskInfo.NodeMemory, TaskInfo.PlanStepID, TaskResult);
}

void UHTNTask::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	// Dummy plan step
	PlanningTask.SubmitPlanStep(this, WorldState->MakeNext(), 100);
}
