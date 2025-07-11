// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "BlackboardWorldstate.h"
#include "HTNStandaloneNode.h"

// A step in an plan. Each standalone node contributes one step.
struct HTN_API FHTNPlanStep
{
	// The standalone node of this plan step.
	// Can be a Task, a SubNetwork, or a structural node like If, Parallel, Prefer etc.
	// This is the template node owned by the HTN asset. 
	// If the node is supposed to be instanced, use the NodeMemoryOffset to find the node instance in the HTNComponent.
	TWeakObjectPtr<UHTNStandaloneNode> Node;

	// The worldstate the task returned during planning and then possibly modified by decorators OnPlanExit.
	// Also stores info on which blackboard keys were changed by this plan step.
	// If the plan step execution succeeds, those keys will be copied to the blackboard.
	// Can be null during planning for nodes with incomplete sublevels (e.g. SubNetwork, If, Prefer, Scope, Sequence etc.).
	TSharedPtr<FBlackboardWorldState> WorldState;

	// The cost of this step, as decided by the Node during planning.
	int32 Cost;

	// Non-task standalone nodes (SubNetworks, structural nodes like Prefer etc.) can have one or two sublevels contained within them.
	// This is the index of the primary sublevel into the FHTNPlan::Levels array of the plan this step is in.
	int32 SubLevelIndex;

	// Some structural nodes (Parallel, If, AnyOrder etc.) produce a plan step with two sublevels, one for each of the branches. 
	// This stores the index of the secondary sublevel.
	int32 SecondarySubLevelIndex;

	// Custom data to be used by specific nodes during planning. See HTNNode_Random for an example.
	uint64 CustomData;

	// Extra flags used by specific nodes.
	bool bAnyOrderInversed : 1;
	bool bIsIfNodeFalseBranch : 1;
	bool bCanConditionsInterruptTrueBranch : 1;
	bool bCanConditionsInterruptFalseBranch : 1;
	// When creating plan step(s) in UHTNNode::MakePlanExpansions, a node can set this flag to mark the created step as
	// a potential divergence point during plan adjustment. When replanning with PlanningType TryToAdjustCurrentPlan, 
	// adjustment can only happen once there's at least one divergence point in the current plan. 
	// This is a useful optimization, because we can abandon the replanning early once we've gone through all 
	// divergence points but didn't diverge.
	// 
	// For example, suppose you're executing a plan in the bottom branch of a couple of Prefer nodes with 
	// PlanAdjustmentMode set to TrySwitchToHigherPriorityBranch. Both Prefer nodes marked their plan steps 
	// as potential divergence points during planning, but no other node did. 
	// If you then trigger a Replan with PlanningType TryToAdjustCurrentPlan, the planner will try to plan, 
	// but if it goes past the final Prefer node and it's still in the same branch as before, it will stop planning
	// there and say that plan adjustment failed because it didn't diverge at any of the potential divergence points.
	// 
	// This eliminates the need to plan through the entire HTN when trying to adjust the current plan, 
	// which makes plan adjustment much cheaper and feasible to do often without sacrificing much performance.
	//
	// In summary, set this to true during planning if during plan adjustment it's possible to take a different path at this step.
	bool bIsPotentialDivergencePointDuringPlanAdjustment : 1;

	// Set by the planner. 
	// If the Node has decorators which made changes to the worldstate OnPlanEnter, this will contain the worldstate modified by them. 
	// Changes in this will be applied before executing the task itself.
	TSharedPtr<FBlackboardWorldState> WorldStateAfterEnteringDecorators;

	// Set when initializing for execution.
	// Offsets into the PlanMemory array of HTNComponent for the standalone Node and its Decorators and Services,
	// so it's possible to find the plan-specific memory that was allocated for each node.
	uint16 NodeMemoryOffset;
	// Runtime information of the subnodes at this step, set when the plan is initialized for execution.
	mutable FHTNRuntimeSubNodesInfo SubNodesInfo;
	
	explicit FHTNPlanStep(UHTNStandaloneNode* Node = nullptr, TSharedPtr<FBlackboardWorldState> WorldState = nullptr,
		int32 Cost = 0, int32 SubLevelIndex = INDEX_NONE, int32 ParallelSubLevelIndex = INDEX_NONE) :
		Node(Node),
		WorldState(WorldState),
		Cost(Cost),
		SubLevelIndex(SubLevelIndex),
		SecondarySubLevelIndex(ParallelSubLevelIndex),
		CustomData(0),
		bAnyOrderInversed(false),
		bIsIfNodeFalseBranch(false),
		bCanConditionsInterruptTrueBranch(true),
		bCanConditionsInterruptFalseBranch(true),
		bIsPotentialDivergencePointDuringPlanAdjustment(false),
		NodeMemoryOffset(0)
	{}
};