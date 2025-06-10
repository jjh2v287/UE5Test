// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTask.h"
#include "Utility/WorldstateSetValueContainer.h"
#include "HTNTask_CopyValue.generated.h"

// Copies the value of one Blackboard key to another in the Worldstate during planning. 
// This change is also applied to the Blackboard once the task finishes execution.
UCLASS()
class HTN_API UHTNTask_CopyValue : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_CopyValue(const FObjectInitializer& ObjectInitializer);
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const;

	// Key to copy from
	UPROPERTY(EditAnywhere, Category = Task)
	FBlackboardKeySelector SourceKey;

	// Key to copy to
	UPROPERTY(EditAnywhere, Category = Task)
	FBlackboardKeySelector TargetKey;
};
