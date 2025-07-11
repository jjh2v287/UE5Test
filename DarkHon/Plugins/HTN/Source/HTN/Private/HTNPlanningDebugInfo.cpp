// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNPlanningDebugInfo.h"

#include "HTNPlan.h"
#include "Misc/StringBuilder.h"

FHTNPlanningDebugInfo::FHTNPlanningDebugInfo() :
	NextNodeId(0)
{}

void FHTNPlanningDebugInfo::AddNode(const TSharedPtr<FHTNPlan>& PreviousPlan, const UHTNStandaloneNode* Node, const TSharedPtr<FHTNPlan>& NewPlan,
	const FString& FailReason, const FString& Description)
{
	const TArray<int32>& PreviousPlanPath = PlanToTreePathMap.FindOrAdd(PreviousPlan);
	FPlanTreeNode* TreeNode = &Root;
	for (const int32 ChildIndex : PreviousPlanPath)
	{
		TreeNode = &TreeNode->Successors[ChildIndex];
	}

	const int32 NewIndex = TreeNode->Successors.Add({
		NewPlan, Node, {},
		FailReason, Description,
		NextNodeId++, NewPlan.IsValid() ? NewPlan->Cost : -1});
	PlanToTreePathMap.Add(NewPlan, PreviousPlanPath).Add(NewIndex);
}

void FHTNPlanningDebugInfo::MarkAsFinishedPlan(const TSharedPtr<FHTNPlan>& Plan)
{
	if (TArray<int32>* const PreviousPlanPath = PlanToTreePathMap.Find(Plan))
	{
		FPlanTreeNode* TreeNode = &Root;
		for (const int32 ChildIndex : *PreviousPlanPath)
		{
			TreeNode = &TreeNode->Successors[ChildIndex];
			TreeNode->bIsPartOfFinalPlan = true;
		}
	}
}

void FHTNPlanningDebugInfo::Reset()
{
	Root.Successors.Reset();
	PlanToTreePathMap.Reset();
	NextNodeId = 0;
}

FString FHTNPlanningDebugInfo::ToString() const
{
	TStringBuilder<32768> StringBuilder; // 2^15 characters buffer on the stack
	
	struct Local
	{
		static void AddSubTree(decltype(StringBuilder)& Output, const FPlanTreeNode& TreeNode, int32 IndentationDepth = 0)
		{
			for (int32 I = 0; I < IndentationDepth; ++I)
			{
				Output << TEXT("    ");
			}

			const bool bNodeSuccessfullyAdded = TreeNode.FailReason.IsEmpty();
			
			{
				Output << TEXT("[");

				if (TreeNode.bIsPartOfFinalPlan)
				{
					Output << TEXT("> ");
				}

				Output << FString::Printf(TEXT("#%i "), TreeNode.NodeId);

				if (bNodeSuccessfullyAdded)
				{
					Output << FString::Printf(TEXT("Cost: %i"), TreeNode.PlanCost);
				}
				else
				{
					Output << TreeNode.FailReason;
				}

				Output << TEXT("]");
			}

			{
				if (TreeNode.Node.IsValid())
				{
					Output << TEXT(" ") << TreeNode.Node->GetNodeName();
				}
				else
				{
					Output << TEXT(" [Missing Node]");
				}

				if (!TreeNode.Description.IsEmpty())
				{
					Output << TEXT(" (") << TreeNode.Description << TEXT(")");
				}
			}
			
			Output << TEXT("\n");
			
			for (const FPlanTreeNode& SubNode : TreeNode.Successors)
			{
				AddSubTree(Output, SubNode, IndentationDepth + 1);
			}
		}
	};

	StringBuilder << TEXT("Root\n");
	for (const FPlanTreeNode& TreeNode : Root.Successors)
	{
		Local::AddSubTree(StringBuilder, TreeNode, 1);
	}
	
	return StringBuilder.ToString();
}
