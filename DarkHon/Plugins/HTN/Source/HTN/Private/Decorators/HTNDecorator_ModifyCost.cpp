// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Decorators/HTNDecorator_ModifyCost.h"

UHTNDecorator_ModifyCost::UHTNDecorator_ModifyCost(const FObjectInitializer& Initializer) : Super(Initializer),
	Scale(1.0f),
	Bias(0)
{
	NodeName = TEXT("Modify Cost");
	bModifyStepCost = true;

	bCheckConditionOnPlanEnter = false;
	bCheckConditionOnPlanExit = false;
	bCheckConditionOnPlanRecheck = false;
	bCheckConditionOnTick = false;
}

FString UHTNDecorator_ModifyCost::GetStaticDescription() const
{
	if (FMath::IsNearlyEqual(Scale, 1.0f))
	{
		if (Bias == 0)
		{
			return FString::Printf(TEXT("%s: leave unchanged"), *Super::GetStaticDescription());
		}
		else
		{
			return FString::Printf(TEXT("%s: Cost + %i"), *Super::GetStaticDescription(), Bias);
		}
	}
	else
	{
		if (Bias == 0)
		{
			return FString::Printf(TEXT("%s: Cost * %.1f"), *Super::GetStaticDescription(), Scale);
		}
		else
		{
			return FString::Printf(TEXT("%s: Cost * %.1f + %i"), *Super::GetStaticDescription(), Scale, Bias);
		}
	}
}

void UHTNDecorator_ModifyCost::ModifyStepCost(UHTNComponent& OwnerComp, FHTNPlanStep& Step) const
{
	Step.Cost = FMath::CeilToInt(Step.Cost * Scale) + Bias;
}
