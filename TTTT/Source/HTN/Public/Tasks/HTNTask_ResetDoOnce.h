// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTask.h"
#include "HTNTask_ResetDoOnce.generated.h"

UENUM()
enum class EHTNResetDoOnceAffectedDecorators
{
	DoOnceDecoratorsWithGameplayTag,
	DoOnceDecoratorsWithoutGameplayTag,
	AllDoOnceDecorators
};

// During execution, resets the specified DoOnce decorators.
// When resetting by gameplay tag, child tags are also affected.
UCLASS()
class HTN_API UHTNTask_ResetDoOnce : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_ResetDoOnce(const FObjectInitializer& Initializer);

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;

	// Which DoOnce decorators should be reset?
	UPROPERTY(EditAnywhere, Category = "Do Once")
	EHTNResetDoOnceAffectedDecorators AffectedDecorators;

	// DoOnce decorators with this gameplay tag will be affected.
	UPROPERTY(EditAnywhere, Category = "Do Once", Meta = (EditCondition = "AffectedDecorators == EHTNResetDoOnceAffectedDecorators::DoOnceDecoratorsWithGameplayTag"))
	FGameplayTag GameplayTag;

protected:
	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) override;
};
