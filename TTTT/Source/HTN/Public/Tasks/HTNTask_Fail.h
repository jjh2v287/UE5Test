// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNTask.h"
#include "HTNTask_Fail.generated.h"

// A utility task that fails during planning or execution (configurable).
UCLASS()
class HTN_API UHTNTask_Fail : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_Fail(const FObjectInitializer& ObjectInitializer);
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID);

private:
	FString GetFailureMessage() const;

	UPROPERTY(EditAnywhere, Category = "Task")
	FString FailureMessage;

	// If true, will fail during execution, 
	// otherwise will fail during planning.
	UPROPERTY(EditAnywhere, Category = "Task")
	uint8 bFailDuringExecution : 1;
};
