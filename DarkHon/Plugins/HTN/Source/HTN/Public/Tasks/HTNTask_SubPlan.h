// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTask.h"
#include "HTNExtension.h"
#include "HTNTask_SubPlan.generated.h"

UENUM()
enum class EHTNSubPlanNodeAbortedReaction : uint8
{
	// The subplan will be aborted and the SubPlan task will finish aborting as soon as that's done.
	AbortSubPlanExecution,
	// The SubPlan task will wait for the subplan to run to completion and then finish executing.
	// If the subplan is being planned when the SubPlan task is aborted, it will wait until that plan is made and finishes executing, 
	// or until planning of the subplan fails.
	WaitForSubPlanToFinish
};

// Everything after this node will be part of a subplan that can be replanned without interrupting the execution of the outer plan.
UCLASS()
class HTN_API UHTNTask_SubPlan : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_SubPlan(const FObjectInitializer& ObjectInitializer);
	virtual FString GetStaticDescription() const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual uint16 GetInstanceMemorySize() const override;

	void InitializeSubPlanInstance(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, TSharedPtr<FHTNPlan> SubPlan) const;

protected:
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const;
	virtual void CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const;
	virtual bool RecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FBlackboardWorldState& WorldState, const FHTNPlanStep& SubmittedPlanStep) override;
	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) override;
	virtual EHTNNodeResult AbortTask(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) override;
	virtual void OnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;

private:
	struct FNodeMemory
	{
		FNodeMemory();

		FHTNPlanInstanceID PlanInstanceID;
		EHTNNodeResult TaskResult;
		uint8 bAbortTaskCalled : 1;
		uint8 bCanPlanLoop : 1;
		uint8 bCanObserverPerformFinishLatentTask : 1;
		uint8 bDeferredStartPlanInstance : 1;
		uint64 FrameCounter;
		TOptional<uint64> FrameOfLastStartOfPlanInstance;
	};

	void StartPlanInstance(UHTNPlanInstance& PlanInstance, FNodeMemory& Memory) const;
	bool CanPlanInstanceLoop(const UHTNPlanInstance* Sender, uint8* NodeMemory) const;
	void OnSubPlanInstanceFinished(UHTNPlanInstance* Sender, uint8* NodeMemory) const;

	// If true, the subplan will be planned as part of the outer plan (the plan that contains the subnode).
	// This is done as if the SubPlan node was a Scope node.
	UPROPERTY(EditAnywhere, Category = "SubPlan")
	uint8 bPlanDuringOuterPlanPlanning : 1;

	// If true, the subplan will be planned when the SubPlan node begins execution.
	// If this is node is in the looping secondary branch of a Parallel node, a new plan will be made every loop.
	// The first execution can reuse the plan made with bPlanDuringOuterPlanPlanning if bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable is enabled.
	UPROPERTY(EditAnywhere, Category = "SubPlan")
	uint8 bPlanDuringExecution : 1;

	// If this, and both bPlanDuringOuterPlanPlanning and bPlanDuringExecution are true, 
	// then on the first run of this SubPlan in the same outer plan it will use the plan made during planning,
	// but consecutive runs will make a new plan.
	// 
	// The following all count as consecutive runs:
	// - replanning due to call to Replan
	// - replanning due to On Sub Plan Succeeded/Failed being set to Loop
	// - the SubPlan task being executed again due to being in a looping secondary branch of a Parallel node
	UPROPERTY(EditAnywhere, Category = "SubPlan", Meta = (EditCondition = "bPlanDuringOuterPlanPlanning && bPlanDuringExecution", EditConditionHides))
	uint8 bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable : 1;

	// What to do when the subplan has successfully finished executing.
	UPROPERTY(EditAnywhere, Category = "SubPlan")
	EHTNPlanInstanceFinishReaction OnSubPlanSucceeded;

	// What to do when the subplan failed during planning or execution.
	UPROPERTY(EditAnywhere, Category = "SubPlan")
	EHTNPlanInstanceFinishReaction OnSubPlanFailed;

	// What to do when this node is aborted while the subplan is running.
	UPROPERTY(EditAnywhere, Category = "SubPlan")
	EHTNSubPlanNodeAbortedReaction OnThisNodeAborted;
};
