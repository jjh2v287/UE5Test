// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNTask.h"
#include "HTNTask_Success.generated.h"

// A utility task that instantly succeeds with configurable cost.
UCLASS()
class HTN_API UHTNTask_Success : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_Success(const FObjectInitializer& ObjectInitializer);
	virtual void Serialize(FArchive& Ar) override;
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;
	virtual FString GetStaticDescription() const override;
	
private:

	// The planning cost of this task.
	UPROPERTY(EditAnywhere, Category = Planning, Meta = (ClampMin = "0"))
	int32 Cost;
};
