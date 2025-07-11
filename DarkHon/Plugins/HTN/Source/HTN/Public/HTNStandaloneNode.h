// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNNode.h"
#include "HTNStandaloneNode.generated.h"

// The base class for standalone nodes (as opposed to subnodes, like decorators or services).
UCLASS(Abstract)
class HTN_API UHTNStandaloneNode : public UHTNNode
{
	GENERATED_BODY()
	
public:
	UHTNStandaloneNode(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeFromAsset(class UHTN& Asset) override;
	virtual FString GetStaticDescription() const override;

	// Called during planning when planning reaches this node. Should create zero or more new plans and submit them via the planning context.
	virtual void MakePlanExpansions(struct FHTNPlanningContext& Context) {}
	// Called during planning to determine what nodes can come after this one. 
	// One of the NextNodes will end up producing the next plan step to go after this one.
	// Default implementation returns all the nodes to which this node has outgoing arrows in the editor.
	// ThisStepID is the ID of the plan step in the Plan tht contains this node. 
	// It may be None if this is the root node of a plan (e.g., SubPlan node)
	// SubLevelIndex is the index of the sublevel for which we're gathering the next nodes. 
	// This allows (for example) a Sequence node to determine which nodes to put in the top branch and which in the bottom branch.
	virtual void GetNextNodes(FHTNNextNodesBuffer& OutNextNodes, const struct FHTNPlan& Plan,
		const FHTNPlanStepID& ThisStepID = FHTNPlanStepID::None, int32 SubLevelIndex = INDEX_NONE);
	// Called during planning when one of the sublevels of this node finished planning. Returns true if this node is finished.
	virtual bool OnSubLevelFinishedPlanning(struct FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex, TSharedPtr<FBlackboardWorldState> WorldState);
	// Called during execution to decide what to execute when execution reaches this node.
	virtual void GetNextPrimitiveSteps(struct FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID);
	// Called during execution when one execution finishes in one of the sublevels of this node. 
	// If this returns true, this node will be considered finished too. In that case, if the node is the last in its level, this will also be called on the node that contains this level.
	// (e.g., for the Sequence node, this only returns true once the last sublevel of the sequence was finished)
	virtual bool OnSubLevelFinished(UHTNPlanInstance& PlanInstance, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex);
	// Called during execution to decide what to execute next when execution finishes in one of the sublevels of this node.
	virtual void GetNextPrimitiveSteps(struct FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex);

	// The maximum number of times this node can be present in a single plan. 0 means no limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning, Meta = (ClampMin = "0"))
	int32 MaxRecursionLimit;

	// If true (default), the planner will try to add one of NextNodes after this node in the plan.
	// Disable this in the constructor if you want the node to put its nextnodes in the sublevels.
	// In that case the node will be expected to create inline sublevels in the plans it makes in MakePlanExpansions and override GetNextPrimitiveSteps. 
	// See HTNNode_Prefer/Scope/Sequence for examples of that.
	UPROPERTY()
	uint8 bPlanNextNodesAfterThis : 1;
	
	// If true (default false), the node will still be allowed to plan even if some decorators on it failed during planning. See HTNNode_If.
	UPROPERTY()
	uint8 bAllowFailingDecoratorsOnNodeDuringPlanning : 1;
	
	// Nodes that this node connects to with outgoing arrows.
	UPROPERTY()
	TArray<TObjectPtr<UHTNStandaloneNode>> NextNodes;

	UPROPERTY()
	TArray<TObjectPtr<class UHTNDecorator>> Decorators;

	UPROPERTY()
	TArray<TObjectPtr<class UHTNService>> Services;
};
