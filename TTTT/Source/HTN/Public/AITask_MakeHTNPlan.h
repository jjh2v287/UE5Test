// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Tasks/AITask.h"
#include "HTN.h"
#include "HTNPlan.h"
#include "HTNPlanningDebugInfo.h"
#include "HTNStandaloneNode.h"
#include "AITask_MakeHTNPlan.generated.h"

class UHTNComponent;
class UHTNPlanInstance;

// A context object passed to HTN nodes when they are planning.
// Provides information needed during planning and 
// utilities for creating and submitting expansions of the currently planned plan.
struct HTN_API FHTNPlanningContext
{
	TWeakObjectPtr<UAITask_MakeHTNPlan> PlanningTask;
	TWeakObjectPtr<UHTNStandaloneNode> AddingNode;
	
	TSharedPtr<FHTNPlan> PlanToExpand;
	FHTNPlanStepID CurrentPlanStepID;
	TSharedPtr<FBlackboardWorldState> WorldStateAfterEnteringDecorators;
	bool bDecoratorsPassed : 1;

	// The number of plans that were added via this context.
	int32 NumAddedCandidatePlans;
	TSharedPtr<FHTNPlan> PreviouslySubmittedPlan;
	
	FHTNPlanningContext();
	FHTNPlanningContext(UAITask_MakeHTNPlan* PlanningTask, UHTNStandaloneNode* AddingNode,
		TSharedPtr<FHTNPlan> PlanToExpand, const FHTNPlanStepID& PlanStepID, 
		TSharedPtr<FBlackboardWorldState> WorldStateAfterEnteringDecorators, bool bDecoratorsPassed);

	class UHTNComponent* GetHTNComponent() const;
	class UHTNPlanInstance* GetPlanInstance() const;
	
	// When the planning is trying to adjust the currently executing plan
	// (see HTNPlanningType::TryToAdjustCurrentPlan), 
	// this returns the step that was produced by the currently planning node in that plan.
	// See HTNNode_Prefer for an example of usage.
	const FHTNPlanStep* GetExistingPlanStepToAdjust() const;

	// When the planning is trying to adjust the currently executing plan
	// (see HTNPlanningType::TryToAdjustCurrentPlan), 
	// this returns plan we're trying to adjust.
	// See HTNNode_SubNetworkDynamic for an example of usage.
	TSharedPtr<const FHTNPlan> GetExistingPlanToAdjust() const;

	TSharedRef<FHTNPlan> MakePlanCopyWithAddedStep(FHTNPlanStep*& OutStep, FHTNPlanStepID& OutStepID) const;
	int32 AddLevel(FHTNPlan& NewPlan, UHTN* HTN, const FHTNPlanStepID& ParentStepID = FHTNPlanStepID::None) const;
	int32 AddInlineLevel(FHTNPlan& NewPlan, const FHTNPlanStepID& ParentStepID = FHTNPlanStepID::None) const;
	
	void SubmitCandidatePlan(const TSharedRef<FHTNPlan>& CandidatePlan, 
		const FString& AddedStepDescription = TEXT(""), bool bOnlyUseIfPreviousFails = false);
};

// A unique ID for a single planning process performed by an AITask_MakeHTNPlan
// Since AITask_MakeHTNPlan are now pooled and can be reused for multiple planning processes at different times,
// a planning process is no longer uniquely identified by the AITask_MakeHTNPlan instance itself.
// When a node is doing planning over multiple frames, it needs to know if the planning task that asked it to plan
// is still doing that planning or is doing a new one. This is why the planning ID is needed. 
// See HTNTask_EQSQuery for an example of usage.
USTRUCT()
struct HTN_API FHTNPlanningID
{
	GENERATED_BODY()

	FHTNPlanningID();
	static FHTNPlanningID GenerateNewID();

	FORCEINLINE bool IsValid() const { return ID != 0; }
	friend FORCEINLINE bool operator==(const FHTNPlanningID& Lhs, const FHTNPlanningID& Rhs) { return Lhs.ID == Rhs.ID; }
	friend FORCEINLINE bool operator!=(const FHTNPlanningID& Lhs, const FHTNPlanningID& Rhs) { return Lhs.ID != Rhs.ID; }
	friend FORCEINLINE uint32 GetTypeHash(const FHTNPlanningID& Key) { return GetTypeHash(Key.ID); }

	UPROPERTY()
	uint64 ID;
};

// Grows a complete FHTNPlan out of an initial plan. 
// The initial plan is typically empty and just the blackboard and HTN assigned.
// That is not necessarily the case when HTNTask_SubPlan is involved.
//
// The planning process may take multiple frames when nodes request to wait until they're done planning
// (see HTNTask_EQSQuery).
UCLASS()
class HTN_API UAITask_MakeHTNPlan : public UAITask
{
	GENERATED_BODY()
	
public:
	UAITask_MakeHTNPlan(const FObjectInitializer& ObjectInitializer);
	void SetUp(UHTNPlanInstance& PlanInstance, TSharedRef<FHTNPlan> InitialPlan, EHTNPlanningType InPlanningType);
	virtual void ExternalCancel() override;

	FHTNPlanningID GetPlanningID() const;
	UHTNComponent* GetOwnerComponent() const;
	UHTNPlanInstance* GetPlanInstance() const;
	bool WasCancelled() const;
	EHTNPlanningType GetPlanningType() const;
	FString GetLogPrefix() const;
	FString GetCurrentStateDescription() const;

	FHTNPlanningContext& GetCurrentPlanningContext();
	const FHTNPlanningContext& GetCurrentPlanningContext() const;
	TSharedPtr<const FHTNPlan> GetCurrentPlan() const;
	TSharedPtr<FHTNPlan> GetCurrentPlan();
	FHTNPlanStepID GetExpandingPlanStepID() const;
	// When the planning is trying to adjust the currently executing plan
	// (see HTNPlanningType::TryToAdjustCurrentPlan), 
	// this returns plan we're trying to adjust.
	// See HTNNode_SubNetworkDynamic for an example of usage.
	TSharedPtr<const FHTNPlan> GetExistingPlanToAdjust() const;

	bool FoundPlan() const;
	TSharedPtr<struct FHTNPlan> GetFinishedPlan() const;
	void Clear();

	// To be used by tasks when planning
	void SubmitPlanStep(const class UHTNTask* Task, TSharedPtr<class FBlackboardWorldState> WorldState, int32 Cost,
		const FString& Description = TEXT(""),
		// See FHTNPlanStep::bIsPotentialDivergencePointDuringPlanAdjustment
		bool bIsPotentialDivergencePointDuringPlanAdjustment = false,
		// See FHTNPlan::bDivergesFromCurrentPlanDuringPlanAdjustment
		bool bDivergesFromCurrentPlanDuringPlanAdjustment = false);
	void WaitForLatentMakePlanExpansions(const UHTNStandaloneNode* Node);
	void FinishLatentMakePlanExpansions(const UHTNStandaloneNode* Node);
	void WaitForLatentCreatePlanSteps(const class UHTNTask* Task);
	void FinishLatentCreatePlanSteps(const class UHTNTask* Task);

	FHTNPriorityMarker MakePriorityMarker();
	void SetNodePlanningFailureReason(const FString& FailureReason);

	DECLARE_EVENT_TwoParams(UAITask_MakeHTNPlan, FHTNPlanningFinishedSignature, UAITask_MakeHTNPlan&, TSharedPtr<FHTNPlan>);
	FHTNPlanningFinishedSignature OnPlanningFinished;

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	
private:
	void DoPlanning();
	TSharedPtr<FHTNPlan> DequeueCurrentBestPlan();
	void MakeExpansionsOfCurrentPlan();
	void MakeExpansionsOfCurrentPlan(const TSharedPtr<class FBlackboardWorldState>& WorldState, UHTNStandaloneNode* NextNode);

	// When the planning is trying to adjust the currently executing plan, 
	// this returns the step that was produced by this node in that plan.
	// See HTNNode_Prefer for an example of usage.
	const FHTNPlanStep* GetExistingPlanStepToAdjust() const;
	void SubmitCandidatePlan(const TSharedRef<FHTNPlan>& NewPlan, UHTNStandaloneNode* AddedNode, const FString& AddedStepDescription = TEXT(""));

	void OnNodeFinishedMakingPlanExpansions(const UHTNStandaloneNode* Node);

	bool EnterDecorators(bool& bOutDecoratorsPassed, const FHTNPlan& Plan, const FHTNPlanStepID& StepID, const UHTNStandaloneNode& Node) const;
	bool EnterDecorators(bool& bOutDecoratorsPassed, TArrayView<UHTNDecorator* const> Decorators, const FHTNPlan& Plan, const FHTNPlanStepID& StepID, bool bMustPass = true) const;

	bool ExitDecoratorsAndPropagateWorldState(FHTNPlan& Plan, const FHTNPlanStepID& StepID) const;
	bool ExitDecorators(TArrayView<UHTNDecorator* const> Decorators, const FHTNPlan& Plan, const FHTNPlanStepID& StepID, bool bShouldPass = true) const;
	void ModifyStepCost(struct FHTNPlanStep& Step, const TArray<UHTNDecorator*>& Decorators) const;

	void ClearIntermediateState();

	int32 GetNumCandidatePlans() const;
	void AddBlockingPriorityMarkersOf(const FHTNPlan& Plan);
	void RemoveBlockingPriorityMarkersOf(const FHTNPlan& Plan);
	bool IsBlockedByPriorityMarkers(const FHTNPlan& Plan) const;
	void AddUnblockedPlansToFrontier();

	UPROPERTY(Transient)
	FHTNPlanningID PlanningID;

	UPROPERTY(Transient)
	TObjectPtr<UHTNComponent> OwnerComponent;

	UPROPERTY(Transient)
	FHTNPlanInstanceID PlanInstanceID;

	UPROPERTY(Transient)
	TObjectPtr<UHTN> TopLevelHTN;

	// The (empty) plan to start planning from.
	TSharedPtr<FHTNPlan> StartingPlan;

	EHTNPlanningType PlanningType;
	TSharedPtr<FHTNPlan> CurrentlyExecutingPlanToAdjust;

	TArray<TSharedPtr<FHTNPlan>> Frontier;

	// Contains plans that are currently blocked from consideration by higher-priority plans regardless of cost
	// (e.g. the bottom branch plans of an HTNNode_Prefer).
	TArray<TSharedPtr<FHTNPlan>> BlockedPlans;

	// PriorityMarkerCounts[PriorityMarker] tells how many plans with a specific priority marker are in the priority queue.
	// This is used to determine which plans are to be kept in the BlockedPlans array.
	TMap<FHTNPriorityMarker, int32> PriorityMarkerCounts;

	FHTNPriorityMarker NextPriorityMarker;

	TSharedPtr<FHTNPlan> CurrentPlanToExpand;
	FHTNPlanStepID CurrentPlanStepID;
	TSharedPtr<FBlackboardWorldState> CurrentWorldState;
	FHTNNextNodesBuffer NextNodes;
	int32 NextNodesIndex;
	TSharedPtr<FBlackboardWorldState> WorldStateAfterEnteredDecorators;
	FHTNPlanningContext CurrentPlanningContext;
	
	TSharedPtr<FHTNPlan> FinishedPlan;

	UPROPERTY(Transient)
	uint8 bIsWaitingForNodeToMakePlanExpansions : 1;

	uint8 bWasCancelled : 1;

#if HTN_DEBUG_PLANNING
	FHTNPlanningDebugInfo DebugInfo;
	mutable FString NodePlanningFailureReason;
#endif

	friend FHTNPlanningContext;
};

FORCEINLINE FHTNPlanningID UAITask_MakeHTNPlan::GetPlanningID() const { return PlanningID; }
FORCEINLINE UHTNComponent* UAITask_MakeHTNPlan::GetOwnerComponent() const { return OwnerComponent; }
FORCEINLINE bool UAITask_MakeHTNPlan::WasCancelled() const { return bWasCancelled; }
FORCEINLINE EHTNPlanningType UAITask_MakeHTNPlan::GetPlanningType() const { return PlanningType; }

FORCEINLINE FHTNPlanningContext& UAITask_MakeHTNPlan::GetCurrentPlanningContext() { return CurrentPlanningContext; }
FORCEINLINE const FHTNPlanningContext& UAITask_MakeHTNPlan::GetCurrentPlanningContext() const { return CurrentPlanningContext; }
FORCEINLINE TSharedPtr<const FHTNPlan> UAITask_MakeHTNPlan::GetCurrentPlan() const { return CurrentPlanToExpand; }
FORCEINLINE TSharedPtr<FHTNPlan> UAITask_MakeHTNPlan::GetCurrentPlan() { return CurrentPlanToExpand; }
FORCEINLINE FHTNPlanStepID UAITask_MakeHTNPlan::GetExpandingPlanStepID() const { return CurrentPlanStepID; }
FORCEINLINE TSharedPtr<const FHTNPlan> UAITask_MakeHTNPlan::GetExistingPlanToAdjust() const { return CurrentlyExecutingPlanToAdjust; }

FORCEINLINE bool UAITask_MakeHTNPlan::FoundPlan() const { return FinishedPlan.IsValid(); }
FORCEINLINE TSharedPtr<struct FHTNPlan> UAITask_MakeHTNPlan::GetFinishedPlan() const { return FinishedPlan; }

FORCEINLINE FHTNPriorityMarker UAITask_MakeHTNPlan::MakePriorityMarker() { return NextPriorityMarker++; }

#if HTN_DEBUG_PLANNING
FORCEINLINE void UAITask_MakeHTNPlan::SetNodePlanningFailureReason(const FString& FailureReason) { NodePlanningFailureReason = FailureReason; }
#else
FORCEINLINE void UAITask_MakeHTNPlan::SetNodePlanningFailureReason(const FString& FailureReason) {}
#endif

FORCEINLINE int32 UAITask_MakeHTNPlan::GetNumCandidatePlans() const { return Frontier.Num() + BlockedPlans.Num(); }
