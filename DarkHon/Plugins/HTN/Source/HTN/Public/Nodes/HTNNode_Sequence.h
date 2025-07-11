// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNNode_TwoBranches.h"
#include "HTNNode_Sequence.generated.h"

// Plans first the top branch, then the bottom branch.
UCLASS()
class HTN_API UHTNNode_Sequence : public UHTNNode_TwoBranches
{
	GENERATED_BODY()
	
public:
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual bool OnSubLevelFinishedPlanning(FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex, TSharedPtr<FBlackboardWorldState> WorldState) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex) override;
};