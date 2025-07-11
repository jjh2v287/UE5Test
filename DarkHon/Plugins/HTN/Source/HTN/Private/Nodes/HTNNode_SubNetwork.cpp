// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_SubNetwork.h"
#include "AITask_MakeHTNPlan.h"
#include "VisualLogger/VisualLogger.h"

FString UHTNNode_SubNetwork::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s:\n%s"), *Super::GetStaticDescription(), *GetNameSafe(HTN));
}

void UHTNNode_SubNetwork::MakePlanExpansions(FHTNPlanningContext& Context)
{	
	FHTNPlanStep* AddedStep = nullptr;
	FHTNPlanStepID AddedStepID;
	const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);

	const auto IsSubHTNValid = [&]() -> bool
	{
		if (HTN && HTN->StartNodes.Num())
		{
			if (const UBlackboardComponent* const BlackboardComponent = Context.PlanningTask->GetOwnerComponent()->GetBlackboardComponent())
			{
				if (!HTN->BlackboardAsset)
				{
					UE_VLOG_UELOG(Context.PlanningTask.Get(), LogHTN, Warning, 
						TEXT("HTN %s has no blackboard asset set."), *GetNameSafe(HTN));
					return false;
				}

				if (!BlackboardComponent->IsCompatibleWith(HTN->BlackboardAsset))
				{
					UE_VLOG_UELOG(Context.PlanningTask.Get(), LogHTN, Warning, 
						TEXT("Blackboard asset of subnetwork %s (%s) is not compatible with the blackboard component of the owner (%s)"), 
						*GetNameSafe(HTN), *GetNameSafe(HTN->BlackboardAsset), *GetNameSafe(BlackboardComponent->GetBlackboardAsset()));
					return false;
				}

				return true;
			}
		}

		return false;
	};
	
	// Valid subnetwork, add a sublevel.
	if (IsSubHTNValid())
	{
		AddedStep->SubLevelIndex = Context.AddLevel(*NewPlan, HTN, AddedStepID);
	}
	
	Context.SubmitCandidatePlan(NewPlan);
}

void UHTNNode_SubNetwork::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
}

FString UHTNNode_SubNetwork::GetNodeName() const
{
	if (!HTN || NodeName.Len())
	{
		return Super::GetNodeName();
	}

	return GetSubStringAfterUnderscore(HTN->GetName());
}

#if WITH_EDITOR
FName UHTNNode_SubNetwork::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.RunBehavior.Icon");
}

UObject* UHTNNode_SubNetwork::GetAssetToOpenOnDoubleClick(const UHTNComponent* DebuggedHTNComponent) const
{
	return HTN;
}
#endif