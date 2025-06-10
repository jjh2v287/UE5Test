// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTask.h"
#include "HTNTask_Replan.generated.h"

// A utility task for replanning the current plan
// If used inside a SubPlan (see HTNTask_SubPlan) affects the subplan instance instead of the top-level plan.
// Useful for force-replanning a SubPlan that this node is in without finishing the SubPlan.
UCLASS()
class HTN_API UHTNTask_Replan : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_Replan(const FObjectInitializer& ObjectInitializer);
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID);

private:
	UPROPERTY(EditAnywhere, Category = "Task", Meta = (ShowOnlyInnerProperties))
	FHTNReplanParameters Parameters;
};
