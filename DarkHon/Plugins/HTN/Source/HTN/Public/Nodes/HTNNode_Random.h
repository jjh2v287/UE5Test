// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNStandaloneNode.h"
#include "HTNNode_Random.generated.h"

// Picks a random node from the following nodes. 
// Put RandomWeight nodes after to influence their chance of being picked.
// Nodes that aren't RandomWeight will have a weight of 1.
// Optionally, if the picked option fails to plan, picks again from the remaining nodes 
// until either a valid plan is found or there are no nodes left.
UCLASS()
class HTN_API UHTNNode_Random : public UHTNStandaloneNode
{
	GENERATED_BODY()

public:
	UHTNNode_Random(const FObjectInitializer& Initializer);
	virtual FString GetStaticDescription() const override;

	virtual void GetNextNodes(FHTNNextNodesBuffer& OutNextNodes, const FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex) override;
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;

private:
	UPROPERTY(EditAnywhere, Category = "Random", Meta = (ClampMin = 0))
	uint8 bFallBackToOtherBranchesIfSelectedBranchFailsPlanning : 1;
};
