// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Services/HTNService_Replan.h"
#include "VisualLogger/VisualLogger.h"

UHTNService_Replan::UHTNService_Replan(const FObjectInitializer& Initializer) : Super(Initializer)
{	
	bNotifyTick = true;

	TickInterval = 0.5f;
	TickIntervalRandomDeviation = 0.05f;

	// Force Replan by default.
	// This way if we're in a SubPlan, we'll actually replan it even if it's not configured to Loop when Failed.
	Parameters.bForceReplan = true;
	
	// The default use case for this service is to try to switch to a higher priority branch of a Prefer node etc. 
	// without actually aborting the current plan if that fails.
	Parameters.PlanningType = EHTNPlanningType::TryToAdjustCurrentPlan;
}

FString UHTNService_Replan::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	return Parameters.ToTitleString();
}

FString UHTNService_Replan::GetStaticDescription() const
{
	TStringBuilder<2048> SB;

	SB << Super::GetStaticDescription() << TEXT(":\n");
	SB << TEXT("Replan Parameters:\n") << Parameters.ToDetailedString();

	return SB.ToString();
}

void UHTNService_Replan::TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime)
{
	if (const UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstanceByNodeMemory(NodeMemory))
	{
		if (!PlanInstance->IsPlanning())
		{
			Replan(OwnerComp, NodeMemory, Parameters);
		}
	}
}
