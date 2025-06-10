// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNNode_TwoBranches.h"
#include "HTNNode_If.generated.h"

UENUM()
enum class EHTNNodeIfPlanAdjustmentMode : uint8
{
	// When trying to adjust the current plan, the plan is not allowed to diverge from the current plan at this node.
	// If the conditions of the decorators of the If node were true in the current plan but are false now (or vice versa), 
	// planning just fails here.
	NoAdjustmentAllowed,
	// When trying to adjust the current plan, it counts as a divergence 
	// if the current plan took the false branch but the new plan takes the true branch.
	TrySwitchToTrueBranch,
	// When trying to adjust the current plan, it counts as a divergence 
	// if the current plan took the true branch but the new plan takes the false branch.
	TrySwitchToFalseBranch,
	// When trying to adjust the current plan, it counts as a divergence 
	// if the new plan takes a different branch compared to the current plan.
	TrySwitchToOtherBranch
};

// The top branch will be taken if all the decorators on this node pass, otherwise the bottom branch.
// 
// Note that at runtime, the False branch can only ever be interrupted if *all* decorators on the If become true.
// For that to happen, all decorators must either have bCheckConditionOnTick enabled or notify of their condition via an event.
UCLASS()
class HTN_API UHTNNode_If : public UHTNNode_TwoBranches
{
	GENERATED_BODY()

public:
	UHTNNode_If(const FObjectInitializer& ObjectInitializer);
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

	// If unticked, the true branch will not be aborted by this node's decorators during execution and plan recheck.
	UPROPERTY(EditAnywhere, Category = Node)
	uint8 bCanConditionsInterruptTrueBranch : 1;

	// If unticked, the false branch will not be aborted by this node's decorators during execution and plan recheck.
	UPROPERTY(EditAnywhere, Category = Node)
	uint8 bCanConditionsInterruptFalseBranch : 1;

	// Determines how this node can participate in the Plan Adjustment process, 
	// (i.e., when triggering a Replan with PlanningType == TryToAdjustExistingPlan).
	// See the tooltip for PlanningType TryToAdjustExistingPlan on the Replan node for more info.
	UPROPERTY(EditAnywhere, Category = Planning)
	EHTNNodeIfPlanAdjustmentMode PlanAdjustmentMode;

private:
	bool IsPotentialDivergencePoint(bool bIsTrueBranch) const;
};