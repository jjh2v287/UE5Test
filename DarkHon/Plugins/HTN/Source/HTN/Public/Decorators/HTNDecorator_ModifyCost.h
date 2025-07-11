// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HTNDecorator.h"
#include "HTNDecorator_ModifyCost.generated.h"

// Modifies the cost of task it's on. Multiple such decorators can be stacked.
UCLASS()
class HTN_API UHTNDecorator_ModifyCost : public UHTNDecorator
{
	GENERATED_BODY()

public:
	UHTNDecorator_ModifyCost(const FObjectInitializer& Initializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (ClampMin = "0"))
	float Scale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (ClampMin = "0"))
	int32 Bias;

	virtual FString GetStaticDescription() const override;
	
protected:

	virtual void ModifyStepCost(UHTNComponent& OwnerComp, FHTNPlanStep& Step) const override;
};