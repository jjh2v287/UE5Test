// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNStandaloneNode.h"
#include "HTNNode_RandomWeight.generated.h"

// A structural node to be used with the Random node to specify the random weight of a specific branch.
// Otherwise acts identical to Scope.
UCLASS()
class HTN_API UHTNNode_RandomWeight : public UHTNStandaloneNode
{
	GENERATED_BODY()

public:
	UHTNNode_RandomWeight(const FObjectInitializer& Initializer);
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;

	// The proportional likelihood of this node being picked by the Random node. 
	// For example, a node with a RandomWeight of 6 is 3 times a likely to be picked asa node with a RandomWeight of 2.
	// Items with RandomWeight 0 can only ever be picked if all the non-zero options are picked instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random", Meta = (ClampMin = 0))
	float RandomWeight;
};
