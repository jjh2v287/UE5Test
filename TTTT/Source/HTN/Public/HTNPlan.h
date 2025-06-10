// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNPlanStep.h"

struct HTN_API FHTNPlan
{
	// Each plan level is a sequence of plan steps, each of which may contain another level 
	// (stored as index into this array).
	// SubNetwork, Scope, If, Prefer, etc. all create a plan step, with one or two sublevels.
	// This creates a scope for decorators and services on the node in that plan step.
	// 
	// E.g. if you have an plan with a single subnetwork which only has tasks, there will be two levels.
	TArray<TSharedPtr<struct FHTNPlanLevel>> Levels;

	// The sum of the costs of the Levels.
	int32 Cost;

	// For tasks with a recursion limit, stores how many times each task is present in this plan.
	// Since most plan expansions don't change this, the map is shared between most plans and 
	// only copied when a node with a recursion limit is added.
	TSharedPtr<TMap<TWeakObjectPtr<UHTNNode>, int32>> RecursionCounts;

	// Allows for some plans to be prioritized over others regardless of cost.
	// Positive markers block the corresponding negative markers.
	// For example, a plan with marker -5 will not be considered for as long as there are any possible plans with marker 5.
	// This way, it's possible to make the planner consider all plans in the top branch of a Prefer node 
	// before the ones in the bottom branch.
	TArray<FHTNPriorityMarker, TInlineAllocator<8>> PriorityMarkers;

	// If set, planning will happen starting at this node, instead of the root node of the current HTN of the OwnerComponent
	TWeakObjectPtr<UHTNStandaloneNode> RootNodeOverride;

	// Relevant during planning when replanning in a way that is meant to adjust the currently executing plan 
	// (see HTNPlanningType::TryAdjustCurrentPlan). This is the number of plan steps in this plan where 
	// FHTNPlanStep::bIsPotentialDivergencePointDuringPlanAdjustment is true.
	int32 NumPotentialDivergencePointsDuringPlanAdjustment;

	// Relevant during planning when replanning in a way that is meant to adjust the currently executing plan 
	// (see HTNPlanningType::TryAdjustCurrentPlan). On a candidate plan during planning, 
	// this is the number of opportunitues to diverge from the current plan that we have left.
	// Every time we have a chance to diverge from the current plan but don't, this number is decremented.
	// If this number reaches zero, the candidate plan is discarded.
	// See FHTNPlanStep::bIsPotentialDivergencePointDuringPlanAdjustment.
	int32 NumRemainingPotentialDivergencePointsDuringPlanAdjustment;

	// Relevant during planning when replanning in a way that is meant to adjust the currently executing plan 
	// (see HTNPlanningType::TryAdjustCurrentPlan). It is used by nodes to mark a submitted plan as a valid adjustment 
	// of the current plan (as opposed to an identical plan). Only such plans will be accepted by the planner
	// since we want to replace the current plan with a different (diverging) one.
	// 
	// In summary, set this to true during planning with TryAdjustCurrentPlan if this step diverges from the current plan.
	bool bDivergesFromCurrentPlanDuringPlanAdjustment : 1;

	FHTNPlan();
	FHTNPlan(UHTN* HTNAsset, TSharedRef<FBlackboardWorldState> WorldStateAtPlanStart, UHTNStandaloneNode* RootNodeOverride = nullptr);
	TSharedRef<FHTNPlan> MakeCopy(int32 IndexOfLevelToCopy, bool bAlsoCopyParentLevel = false) const;
	bool HasLevel(int32 LevelIndex) const;
	bool IsComplete() const;
	bool IsLevelComplete(int32 LevelIndex) const;
	
	bool FindStepToAddAfter(FHTNPlanStepID& OutPlanStepID) const;
	void GetWorldStateAndNextNodes(const FHTNPlanStepID& StepID, TSharedPtr<class FBlackboardWorldState>& OutWorldState, FHTNNextNodesBuffer& OutNextNodes) const;
	
	// Performs a number of checks to verify that the plan is valid and all cross-links via array indices are valid.
	void CheckIntegrity() const;

	// Checks if all the subnodes in the plan are not active. If they are, logs an Error into the visual logger and the output log.
	void VerifySubNodesAreInactive(const UObject* VisLogOwner) const;

	const FHTNPlanStep& GetStep(const FHTNPlanStepID& PlanStepID) const;
	FHTNPlanStep& GetStep(const FHTNPlanStepID& PlanStepID);
	const FHTNPlanStep* FindStep(const FHTNPlanStepID& PlanStepID) const;
	FHTNPlanStep* FindStep(const FHTNPlanStepID& PlanStepID);
	bool HasStep(const FHTNPlanStepID& StepID, int32 LevelIndex = 0) const;
	bool IsSecondaryParallelStep(const FHTNPlanStepID& StepID) const;
	
	// Fills OutSubNodeGroups with groups of subnodes (decorators and services) that are active at plan step specified by StepID.
	// The innermost group (the one that starts last) is added first, the outermost group is added last.
	// Optionally only includes the subnodes starting or ending at this plan step.
	void GetSubNodesAtPlanStep(const FHTNPlanStepID& StepID, FHTNSubNodeGroups& OutSubNodeGroups, bool bOnlyStarting = false, bool bOnlyEnding = false) const;

	bool FindExecutingSubNodeGroupWithDecoratorWithMemoryOffset(FHTNSubNodeGroup& OutSubNodeGroup, uint16 MemoryOffset, FHTNPlanStepID UnderPlanStepID = { 0, INDEX_NONE }) const;

	// Gets the index of the first/last sublevel of the given step which actually contains primitive tasks. 
	// Returns INDEX_NONE if there is no such step or if it has no sublevels that meet this condition.
	int32 GetFirstSubLevelWithTasksOfStep(const FHTNPlanStepID& StepID) const;
	int32 GetLastSubLevelWithTasksOfStep(const FHTNPlanStepID& StepID) const;
	bool HasTasksInLevel(int32 LevelIndex) const;

	// Given a decorator and a step ID during which it is active, finds the worldstate that was before the decorator became active.
	// This can be used for restoring worldstate/blackboard values to values they had before a decorator, which is useful for scope guards.
	TSharedPtr<const FBlackboardWorldState> GetWorldstateBeforeDecoratorPlanEnter(const class UHTNDecorator& Decorator, const FHTNPlanStepID& ActiveStepID) const;
	
	// Given a decorator and a step ID during which it is active, finds the first step ID at which this decorator became active.
	FHTNPlanStepID FindDecoratorStartStepID(const class UHTNDecorator& Decorator, const FHTNPlanStepID& ActiveStepID) const;
	
	int32 GetRecursionCount(UHTNNode* Node) const;
	void IncrementRecursionCount(UHTNNode* Node);
};

// A sequence of plan steps.
struct HTN_API FHTNPlanLevel
{
	// This empty level can be used to plug up holes left by removing levels from a plan.
	// For example, when a plan needs to be split up into subplans (see HTNTask_SubPlan::bPlanDuringOuterPlanPlanning).
	static TSharedRef<FHTNPlanLevel> DummyPlanLevel;

	TWeakObjectPtr<UHTN> HTNAsset;
	TSharedPtr<class FBlackboardWorldState> WorldStateAtLevelStart;

	TArray<FHTNPlanStep> Steps;

	// Step ID of the step containing this level
	FHTNPlanStepID ParentStepID;
	
	// The sum of the costs of the Steps.
	int32 Cost;

	uint8 bIsInline : 1;
	uint8 bIsDummyLevel : 1;
	
	// Runtime information of the subnodes on the root node of this level, set when the plan is initialized for execution.
	mutable FHTNRuntimeSubNodesInfo RootSubNodesInfo;

	FHTNPlanLevel(UHTN* HTNAsset, TSharedPtr<class FBlackboardWorldState> WorldStateAtLevelStart,
		const FHTNPlanStepID& ParentStepID = FHTNPlanStepID::None, bool bIsInline = false, bool bIsDummyLevel = false) :
		HTNAsset(HTNAsset),
		WorldStateAtLevelStart(WorldStateAtLevelStart),
		ParentStepID(ParentStepID),
		Cost(0),
		bIsInline(bIsInline),
		bIsDummyLevel(bIsDummyLevel)
	{}

	FORCEINLINE bool IsInlineLevel() const { return bIsInline; }
	FORCEINLINE bool IsDummyLevel() const { return bIsDummyLevel; }
	TArrayView<TObjectPtr<class UHTNDecorator>> GetRootDecoratorTemplates() const;
	TArrayView<TObjectPtr<class UHTNService>> GetRootServiceTemplates() const;

	// Initializes the subnodes at the root node of this level (if any).
	// This is const because it only changes temporary data on the root subnodes without modifying the level itself.
	void InitializeFromAsset(UHTN& Asset) const;
};

struct HTN_API FHTNGetNextStepsContext
{
	const UHTNComponent& OwnerComp;
	const UHTNPlanInstance& PlanInstance;
	const FHTNPlan& Plan;

	// This is false when we're stepping through the plan for rechecking of vislogging purposes.
	// If this is false, avoid looping a level (like Parallel does) or using node memory.
	bool bIsExecutingPlan;

	FHTNGetNextStepsContext(const UHTNPlanInstance& PlanInstance, const FHTNPlan& Plan, bool bIsExecutingPlan, TArray<FHTNPlanStepID>& OutStepIds);

	void SubmitPlanStep(const FHTNPlanStepID& PlanStepID);
	FORCEINLINE int32 GetNumSubmittedSteps() const { return NumSubmittedSteps; }

	int32 AddNextPrimitiveStepsAfter(const FHTNPlanStepID& InStepID);
	int32 AddFirstPrimitiveStepsInLevel(int32 LevelIndex);
	int32 AddFirstPrimitiveStepsInAnySublevelOf(const FHTNPlanStepID& StepID);

private:
	TArray<FHTNPlanStepID>& OutStepIds;
	int32 NumSubmittedSteps;
};
