// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNNode_TwoBranches.h"
#include "HTNNode_AnyOrder.generated.h"

// Plans to execute both branches, but considers both doing the first branch and then the second, and the other way around.
UCLASS()
class HTN_API UHTNNode_AnyOrder : public UHTNNode_TwoBranches
{
	GENERATED_BODY()
	
public:
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextNodes(FHTNNextNodesBuffer& OutNextNodes, const FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex) override;
	virtual bool OnSubLevelFinishedPlanning(FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex, TSharedPtr<FBlackboardWorldState> WorldState) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex) override;
};