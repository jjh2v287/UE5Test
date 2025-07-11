// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNStandaloneNode.h"
#include "HTNNode_Scope.generated.h"

// A structural node that defines a scope.
// Decorators and services on this node will be active for as long as any nodes after this node (within the same HTN and subnetworks) are active.
UCLASS()
class HTN_API UHTNNode_Scope : public UHTNStandaloneNode
{
	GENERATED_BODY()

public:
	UHTNNode_Scope(const FObjectInitializer& Initializer);
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;
};