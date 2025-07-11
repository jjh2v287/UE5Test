// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNDecorator_BlackboardBase.h"
#include "Utility/WorldstateSetValueContainer.h"
#include "HTNDecorator_GuardValue.generated.h"

// A decorator for setting a value in the worldstate and/or later restoring it to its original value.
// On activation (during planning) optionally sets the specified key in the worldstate to the specified value.
// On deactivation (during planning) optionally restores the key to the value it had before the decorator activated.
// By default, the value is also restored if the decorator is aborted before properly completing.
UCLASS()
class HTN_API UHTNDecorator_GuardValue : public UHTNDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UHTNDecorator_GuardValue(const FObjectInitializer& Initializer);
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	virtual void OnEnterPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	virtual void OnExitPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;
	
	virtual FString GetStaticDescription() const override;

	bool RestoreWorldstateValue(UHTNComponent& OwnerComp, UWorldStateProxy& WorldStateProxy, const FHTNPlan& CurrentPlan, const FHTNPlanStepID& StepIDWithDecorator) const;
	
private:
	struct FNodeMemory
	{
		FHTNPlanStepID PlanStepID = FHTNPlanStepID::None;
	};

	UPROPERTY(EditAnywhere, Category = Node)
	FWorldstateSetValueContainer Value;

	// If set, the value will be set when the decorator becomes active during planning.
	UPROPERTY(EditAnywhere, Category = Node)
	uint8 bSetValueOnEnterPlan : 1;

	// If set, the value will be restored when the decorator becomes inactive during planning.
	UPROPERTY(EditAnywhere, Category = Node)
	uint8 bRestoreValueOnExitPlan : 1;

	// If set, the value will be restored when the decorator's execution is aborted.
	UPROPERTY(EditAnywhere, Category = Node)
	uint8 bRestoreValueOnAbort : 1;
};