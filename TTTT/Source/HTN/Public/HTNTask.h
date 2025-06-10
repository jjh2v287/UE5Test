// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNStandaloneNode.h"
#include "HTNTypes.h"
#include "HTNComponent.h"
#include "BlackboardWorldstate.h"
#include "HTNPlan.h"
#include "AITask_MakeHTNPlan.h"

#include "HTNTask.generated.h"

struct HTN_API FHTNTaskSpecialMemory : public FHTNNodeSpecialMemory
{
	// It's not legal to call FinishLatentTask from within some functions (e.g., ExecuteTask, AbortTask).
	// When such a function is executing, this equals the human-readable name of that function.
	FName NameOfCurrentlyExecutingFunctionDisallowingFinishLatentTask;

	// When the task finishes, we set this to the result. 
	// Keeping track of this allows us to catch erroneous calls to FinishLatentTask on tasks that have already finished.
	EHTNNodeResult ExecutionResult = EHTNNodeResult::InProgress;
};

UCLASS(Abstract)
class HTN_API UHTNTask : public UHTNStandaloneNode
{
	GENERATED_BODY()

public:
	UHTNTask(const FObjectInitializer& Initializer);

	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual uint16 GetSpecialMemorySize() const override;
	virtual void InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;

	bool WrappedRecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FBlackboardWorldState& WorldState, const FHTNPlanStep& SubmittedPlanStep) const;
	EHTNNodeResult WrappedExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) const;
	EHTNNodeResult WrappedAbortTask(UHTNComponent& OwnerComp, uint8* NodeMemory) const;
	void WrappedTickTask(UHTNComponent& OwnerComp, uint8* NodeMemory , float DeltaTime) const;
	void WrappedOnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) const;
	void WrappedLogToVisualLog(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStep& SubmittedPlanStep) const;
	
	// If you return InProgress from ExecuteTask or AbortTask, call this at some point after that (usually in TickTask) to actually finish executing.
	// Passing the NodeMemory pointer is optional but recommended. It helps find the correct task instance if a single
	// task from the HTN asset is involved in multiple plan steps in the same plan and those plan steps are executing at the same time.
	// (e.g., if there is recursion or a loop of tasks together with Parallel or SubPlan nodes).
	void FinishLatentTask(UHTNComponent& OwnerComp, EHTNNodeResult TaskResult, const uint8* NodeMemory = nullptr) const;

	// If false, this task won't be shown in location summaries when visualizing the current plan. LogToVisualLog will still be called.
	UPROPERTY(Category = "Planning", EditAnywhere)
	uint8 bShowTaskNameOnCurrentPlanVisualization : 1;

	// If true and this task calls SubmitPlanStep multiple times during planning,
	// each candidate step will only be considered if making a plan with any of the previous ones fails, 
	// similarly to the Prefer node.
	UPROPERTY(Category = "Planning", EditAnywhere)
	uint8 bProcessSubmittedPlanStepsInOrder : 1;
	
protected:
	// Check preconditions and output one (or more, for branching) plan steps with a link to self and a modified worldstate.
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const;

	// Called to recheck if a currently executed plan is still actionable.
	virtual bool RecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FBlackboardWorldState& WorldState, const FHTNPlanStep& SubmittedPlanStep) { return true; }
	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) { return EHTNNodeResult::Succeeded; }
	virtual EHTNNodeResult AbortTask(UHTNComponent& OwnerComp, uint8* NodeMemory) { return EHTNNodeResult::Aborted; }
	virtual void TickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) {}
	virtual void OnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) {}

	virtual void LogToVisualLog(UHTNComponent& OwnerComp, const uint8* NodeMemory, const FHTNPlanStep& SubmittedPlanStep) const {}

	uint8 bNotifyTick : 1;
	uint8 bNotifyTaskFinished : 1;
};