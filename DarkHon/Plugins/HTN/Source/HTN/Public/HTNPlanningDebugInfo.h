// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNStandaloneNode.h"

// Debug info of an HTN planning process
class HTN_API FHTNPlanningDebugInfo
{
public:
	FHTNPlanningDebugInfo();
	void AddNode(const TSharedPtr<FHTNPlan>& PreviousPlan, const UHTNStandaloneNode* Node, const TSharedPtr<FHTNPlan>& NewPlan,
		const FString& FailReason = TEXT(""), const FString& Description = TEXT(""));
	void MarkAsFinishedPlan(const TSharedPtr<FHTNPlan>& Plan);
	void Reset();

	FString ToString() const;
	
private:
	struct FPlanTreeNode
	{
		TWeakPtr<FHTNPlan> Plan;
		TWeakObjectPtr<const UHTNStandaloneNode> Node;
		TArray<FPlanTreeNode> Successors;

		// If not null, the node represents a failure to add a plan step.
		FString FailReason;
		
		// The description of the plan step supplied by the node during planning.
		FString Description;

		int32 NodeId = INDEX_NONE;
		int32 PlanCost = -1;
		bool bIsPartOfFinalPlan = false;
	};
	
	FPlanTreeNode Root;
	TMap<TWeakPtr<FHTNPlan>, TArray<int32>> PlanToTreePathMap;
	int32 NextNodeId;
};
