// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/HTNTask_BlackboardBase.h"
#include "Utility/WorldstateSetValueContainer.h"
#include "HTNTask_SetValue.generated.h"

// Sets the specified key in the worldstate to the specified value during planning. 
// That value is copied to the blackboard during execution.
UCLASS()
class HTN_API UHTNTask_SetValue : public UHTNTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UHTNTask_SetValue(const FObjectInitializer& ObjectInitializer);
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = Node)
	FWorldstateSetValueContainer Value;
};
