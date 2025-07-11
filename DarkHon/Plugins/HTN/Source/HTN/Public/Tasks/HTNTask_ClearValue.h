// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/HTNTask_BlackboardBase.h"
#include "Utility/WorldstateSetValueContainer.h"
#include "HTNTask_ClearValue.generated.h"

// Clears the specified key in the worldstate during planning. 
UCLASS()
class HTN_API UHTNTask_ClearValue : public UHTNTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UHTNTask_ClearValue(const FObjectInitializer& ObjectInitializer);
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;
	virtual FString GetStaticDescription() const override;
};
