// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNTask.h"
#include "HTNTask_Wait.generated.h"

// During execution, waits for a configurable, randomizable duration and then succeeds.
UCLASS()
class HTN_API UHTNTask_Wait : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_Wait(const FObjectInitializer& Initializer);
	virtual void Serialize(FArchive& Ar) override;
	
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;
	
	virtual uint16 GetInstanceMemorySize() const override;

	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) override;
	virtual void TickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif

	UPROPERTY(EditAnywhere, Category = Wait)
	uint8 bWaitForever : 1;

	UPROPERTY(EditAnywhere, Category = Wait, Meta = (ClampMin = 0, UIMin = 0, EditCondition = "!bWaitForever"))
	float WaitTime;

	UPROPERTY(EditAnywhere, Category = Wait, Meta = (ClampMin = 0, UIMin = 0, EditCondition = "!bWaitForever"))
	float RandomDeviation;
	
	// The planning cost of this task.
	UPROPERTY(EditAnywhere, Category = Planning, Meta = (ClampMin = 0))
	int32 Cost;

private:
	struct FNodeMemory
	{
		float RemainingWaitTime;
	};
};
