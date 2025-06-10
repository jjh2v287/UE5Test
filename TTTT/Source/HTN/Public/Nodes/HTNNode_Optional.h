// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNStandaloneNode.h"
#include "HTNNode_Optional.generated.h"

UENUM()
enum class EHTNNodeOptionalPlanAdjustmentMode : uint8
{
	// When trying to adjust the current plan, the plan is not allowed to diverge from the current plan at this node.
	// The planner will only try to plan through the same branch as in the current plan.
	NoAdjustmentAllowed,
	// When trying to adjust the current plan, the plan will try to diverge from the current plan
	// by switching from the version where the optional did not have the nodes after it to the one where it does if possible.
	// Plan adjustment only succeeds if a divergence like this happens at least at one node.
	TrySwitchToWithContent,
	// When trying to adjust the current plan, the plan will try to diverge from the current plan
	// by switching from the version where the optional had the nodes after it to the one where it doesn't if possible.
	// Plan adjustment only succeeds if a divergence like this happens at least at one node.
	TrySwitchToWithoutContent,
	// When trying to adjust the current plan, the plan will try to diverge from the current plan
	// by switching from the current version (with or without the nodes after the Optional) to the other one.
	// Plan adjustment only succeeds if a divergence like this happens at least at one node.
	TrySwitchToOther
};

// Everything after this node is optional. 
// The planner will try to make a plan that includes tasks after this. 
// If that fails, it will exclude those tasks from the final plan instead of failing the whole planning.
// Identical to having a Prefer node that only uses the top branch.
UCLASS()
class HTN_API UHTNNode_Optional : public UHTNStandaloneNode
{
	GENERATED_BODY()
	
public:
	UHTNNode_Optional(const FObjectInitializer& Initializer);
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;

	virtual FString GetStaticDescription() const override;

	// Determines how this node can participate in the Plan Adjustment process, 
	// (i.e., when triggering a Replan with PlanningType == TryToAdjustExistingPlan).
	// See the tooltip for PlanningType TryToAdjustExistingPlan on the Replan node for more info.
	UPROPERTY(EditAnywhere, Category = Planning)
	EHTNNodeOptionalPlanAdjustmentMode PlanAdjustmentMode;

private:
	bool IsPotentialDivergencePoint(bool bIsTopBranch) const;
};
