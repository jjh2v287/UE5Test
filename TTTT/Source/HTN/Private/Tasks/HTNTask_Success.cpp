// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_Success.h"
#include "HTNObjectVersion.h"

UHTNTask_Success::UHTNTask_Success(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	Cost(0)
{
	bShowTaskNameOnCurrentPlanVisualization = false;
}

void UHTNTask_Success::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FHTNObjectVersion::GUID);

	const int32 HTNObjectVersion = Ar.CustomVer(FHTNObjectVersion::GUID);
	if (HTNObjectVersion < FHTNObjectVersion::DisableDefaultCostOnSomeTasks)
	{
		// When loading an older version asset, restore this value to the old default before loading.
		// If the value was changed in asset, it will be overwritten during loading. 
		// If it wasn't, it'll stay at the old default.
		Cost = 100;
	}

	Super::Serialize(Ar);
}

void UHTNTask_Success::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	PlanningTask.SubmitPlanStep(this, WorldState->MakeNext(), FMath::Max(0, Cost));
}

FString UHTNTask_Success::GetStaticDescription() const
{
	if (Cost > 0)
	{
		return FString::Printf(TEXT("%s: Cost: %i"), *Super::GetStaticDescription(), Cost);
	}

	return Super::GetStaticDescription();
}
