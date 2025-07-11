// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_Wait.h"
#include "HTNObjectVersion.h"
#include "Misc/StringBuilder.h"

UHTNTask_Wait::UHTNTask_Wait(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bWaitForever(false),
	WaitTime(5.0f),
	RandomDeviation(0.0f),
	Cost(0)
{
	bNotifyTick = true;
}

void UHTNTask_Wait::Serialize(FArchive& Ar)
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

uint16 UHTNTask_Wait::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}

void UHTNTask_Wait::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	PlanningTask.SubmitPlanStep(this, WorldState->MakeNext(), FMath::Max(0, Cost));
}

EHTNNodeResult UHTNTask_Wait::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	FNodeMemory* const Memory = CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	Memory->RemainingWaitTime = FMath::FRandRange(FMath::Max(0.0f, WaitTime - RandomDeviation), WaitTime + RandomDeviation);
	
	return EHTNNodeResult::InProgress;
}

void UHTNTask_Wait::TickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (bWaitForever)
	{
		return;
	}

	FNodeMemory* const Memory = CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	
	Memory->RemainingWaitTime -= DeltaSeconds;
	if (Memory->RemainingWaitTime <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EHTNNodeResult::Succeeded, NodeMemory);
	}
}

FString UHTNTask_Wait::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	return bWaitForever ? TEXT("Wait Forever") : TEXT("Wait");
}

FString UHTNTask_Wait::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription() << TEXT(" ");

	if (bWaitForever)
	{
		SB << TEXT("forever");
	}
	else
	{
		SB << FString::SanitizeFloat(WaitTime);
		if (RandomDeviation != 0.0f)
		{
			SB << TEXT("+-") << FString::SanitizeFloat(RandomDeviation);
		}
		SB << TEXT("s");
	}

	if (Cost != 0)
	{
		SB << TEXT("\nCost: ") << Cost;
	}

	return SB.ToString();
}

#if WITH_EDITOR
FName UHTNTask_Wait::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.Wait.Icon");
}
#endif
