// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTask.h"
#include "HTNTask_ResetCooldown.generated.h"

UENUM()
enum class EHTNResetCooldownAffectedCooldowns
{
	CooldownsWithGameplayTag,
	CooldownsWithoutGameplayTag,
	AllCooldowns
};

// During execution, resets the specified Cooldown decorators.
// When resetting by gameplay tag, child tags are also affected.
UCLASS()
class HTN_API UHTNTask_ResetCooldown : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_ResetCooldown(const FObjectInitializer& Initializer);

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;

	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;

	// Which Cooldown decorators should be reset?
	UPROPERTY(EditAnywhere, Category = "Cooldown")
	EHTNResetCooldownAffectedCooldowns AffectedCooldowns;

	// Cooldown decorators with this gameplay tag will be affected.
	UPROPERTY(EditAnywhere, Category = "Cooldown", Meta = (EditCondition = "AffectedCooldowns == EHTNResetCooldownAffectedCooldowns::CooldownsWithGameplayTag"))
	FGameplayTag GameplayTag;

protected:
	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) override;
};
