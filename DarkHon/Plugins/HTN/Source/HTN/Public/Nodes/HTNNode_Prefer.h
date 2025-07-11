// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNNode_TwoBranches.h"
#include "HTNNode_Prefer.generated.h"

UENUM()
enum class EHTNNodePreferPlanAdjustmentMode : uint8
{
	// When trying to adjust the current plan, the plan is not allowed to diverge from the current plan at this node.
	// The planner will only try to plan through the same branch as in the current plan.
	NoAdjustmentAllowed,
	// When trying to adjust the current plan, the plan will try to diverge from the current plan
	// by switching from the bottom branch to the top branch if possible.
	// Plan adjustment only succeeds if a divergence like this happens at least at one node.
	TrySwitchToHigherPriorityBranch,
	// When trying to adjust the current plan, the plan will try to diverge from the current plan
	// by switching from the top branch to the bottom branch if possible.
	// Plan adjustment only succeeds if a divergence like this happens at least at one node.
	TrySwitchToLowerPriorityBranch,
	// When trying to adjust the current plan, the plan will try to diverge from the current plan
	// by switching from the current branch to another branch if possible.
	// Plan adjustment only succeeds if a divergence like this happens at least at one node.
	TrySwitchToOtherBranch
};

// Plans one of the branches, but such that the bottom branch is taken only if the top branch can't produce a plan.
UCLASS()
class HTN_API UHTNNode_Prefer : public UHTNNode_TwoBranches
{
	GENERATED_BODY()
	
public:
	UHTNNode_Prefer(const FObjectInitializer& Initializer);
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;

	virtual FString GetStaticDescription() const override;

	// Determines how this node can participate in the Plan Adjustment process, 
	// (i.e., when triggering a Replan with PlanningType == TryToAdjustExistingPlan).
	// See the tooltip for PlanningType TryToAdjustExistingPlan on the Replan node for more info.
	UPROPERTY(EditAnywhere, Category = Planning)
	EHTNNodePreferPlanAdjustmentMode PlanAdjustmentMode;

private:
	bool IsPotentialDivergencePoint(bool bIsTopBranch) const;
};
