// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNTypes.h"
#include "HTNPlanInstance.generated.h"

class UHTN;
class UHTNComponent;
class UHTNNode;
class UHTNStandaloneNode;
class UHTNTask;
class UHTNDecorator;
class UHTNService;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHTNPlanInstanceStartedPlanExecution, UHTNPlanInstance* /*Sender*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHTNPlanInstanceFinishedPlanExecution, UHTNPlanInstance* /*Sender*/, EHTNPlanExecutionFinishedResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHTNPlanInstanceFinished, UHTNPlanInstance* /*Sender*/);

DECLARE_DELEGATE_RetVal_OneParam(bool, FCanHTNPlanInstanceLoop, const UHTNPlanInstance* /*PlanInstance*/)

// Configuration for making a UHTNPlanInstance. 
// Non-default values are only relevant to non-root plan instances (i.e., ones made by HTNTask_SubPlan). 
USTRUCT()
struct HTN_API FHTNPlanInstanceConfig
{
	GENERATED_BODY()

	static const FHTNPlanInstanceConfig Default;

	FHTNPlanInstanceConfig();

	bool ShouldStopIfSucceeded() const;
	bool ShouldStopIfFailed() const;

	UPROPERTY()
	EHTNPlanInstanceFinishReaction SucceededReaction;

	UPROPERTY()
	EHTNPlanInstanceFinishReaction FailedReaction;

	// An optional pre-made plan for this plan instance to execute.
	TSharedPtr<struct FHTNPlan> PrePlannedPlan;

	// Is this subplan allowed to plan, or should it rely on the PrePlannedPlan?
	UPROPERTY()
	uint8 bPlanDuringExecution : 1;

	// If there's already a PrePlannedPlan available, 
	// should we use that for the first execution even if bPlanDuringExecution is enabled
	// With this off, a new plan will be made even if bPlanDuringExecution is true.
	UPROPERTY()
	uint8 bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable : 1;

	// How nested this plan instance is. 
	// 0 is root instance, 
	// 1 is their subplan instances, 
	// 2 is *their* subplan instances and so on.
	UPROPERTY()
	int32 Depth;

	// If set, planning will happen starting at this node instead of the root node of the current HTN of the OwnerComponent
	UPROPERTY()
	TObjectPtr<UHTNStandaloneNode> RootNodeOverride;

	// The recursion counts of the outer plan that we'll pass on to the subplan during its planning.
	// Doing so allows the MaxRecursionLimit property of HTN nodes to work even when in a loop of SubPlan tasks.
	// See FHTNPlan::RecursionCounts
	TSharedPtr<TMap<TWeakObjectPtr<UHTNNode>, int32>> OuterPlanRecursionCounts;

	// An optional predicate that determines if the plan instance can loop (start another planning after the current round).
	FCanHTNPlanInstanceLoop CanPlanInstanceLoopDelegate;
};

// A wrapper around an HTNPlan that contains and manages the runtime data for it, 
// such as the plan memory and the node instances. It can contain subplans which may be replanned during execution.
// 
// The root plan instance of an HTNComponent will keep executing in a loop forever, 
// but instances of subplans will only do so once and are governed by their respective HTNTask_SubPlan nodes.
UCLASS(DefaultToInstanced)
class HTN_API UHTNPlanInstance : public UObject
{
	GENERATED_BODY()

public:
	UHTNPlanInstance();

	void Initialize(UHTNComponent& InOwnerComponent, FHTNPlanInstanceID InID, const FHTNPlanInstanceConfig& InConfig = FHTNPlanInstanceConfig::Default);
	void OnPreDestroy();
	void DeleteAllWorldStates();

	FHTNPlanInstanceID GetID() const;
	int32 GetDepth() const;
	EHTNPlanInstanceStatus GetStatus() const;
	EHTNLockFlags GetLockFlags() const;
	bool IsPlanning() const;
	bool HasPlan() const;
	bool HasActiveTasks() const;
	bool HasActivePlan() const;
	bool IsAbortingPlan() const;
	bool HasDeferredAbort() const;
	bool IsWaitingForAbortingTasks() const;
	bool CanStartExecutingNewPlan() const;
	UHTNNode* GetNodeInstanceInCurrentPlan(const UHTNNode& NodeTemplate, const uint8* NodeMemory) const;
	bool OwnsNodeMemory(const uint8* NodeMemory) const;
	FHTNNodeInPlanInfo FindActiveTaskInfo(const UHTNTask* Task, const uint8* NodeMemory = nullptr) const;
	FHTNNodeInPlanInfo FindActiveDecoratorInfo(const UHTNDecorator* Decorator, const uint8* NodeMemory = nullptr) const;
	FHTNNodeInPlanInfo FindActiveServiceInfo(const UHTNService* Service, const uint8* NodeMemory = nullptr) const;
	bool IsRootInstance() const;
	FORCEINLINE const UHTNComponent* GetOwnerComponent() const { return OwnerComponent; }
	FORCEINLINE UHTNComponent* GetOwnerComponent() { return OwnerComponent; }
	FORCEINLINE const class UAITask_MakeHTNPlan* GetCurrentPlanningTask() const { return CurrentPlanningTask; }
	FORCEINLINE class UAITask_MakeHTNPlan* GetCurrentPlanningTask() { return CurrentPlanningTask; }
	FORCEINLINE TSharedPtr<const FHTNPlan> GetCurrentPlan() const { return CurrentPlan; }
	FORCEINLINE TSharedPtr<FHTNPlan> GetCurrentPlan() { return CurrentPlan; }
	FORCEINLINE const TArray<FHTNPlanStepID>& GetPendingExecutionStepIDs() const { return PendingExecutionStepIDs; }
	FORCEINLINE const TArray<FHTNPlanStepID>& GetCurrentlyExecutingStepIDs() const { return CurrentlyExecutingStepIDs; }
	FORCEINLINE const TArray<FHTNPlanStepID>& GetCurrentlyAbortingStepIDs() const { return CurrentlyAbortingStepIDs; }

#if USE_HTN_DEBUGGER
	FHTNDebugExecutionPerPlanInstanceInfo GetDebugStepInfo() const;
#endif

#if ENABLE_VISUAL_LOG
	void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
	void VisualizeCurrentPlan() const;
#endif

	FString DescribeCurrentPlan() const;

	TSharedPtr<FHTNPlan> GetPlanPendingExecution() const;
	void SetPlanPendingExecution(TSharedPtr<FHTNPlan> Plan, bool bTryToInitialize = false);

	void Start();
	void Tick(float DeltaTime);
	void Stop(bool bDisregardLatentAbort = false);

	// Verifies that the remaining part of the current plan is still valid.
	// For all steps in the plan yet to be executed, checks if the conditions of the task and its decorators still pass, given the estimated worldstate at that point in the plan.
	// Future worldstates ar estimated by applying planned changes to the current worldstate (one made from the blackboard at this moment, but a custom worldstate can be provided).
	bool RecheckCurrentPlan(const class FBlackboardWorldState* WorldStateOverride = nullptr);

	void Replan(const FHTNReplanParameters& Params = FHTNReplanParameters::Default);
	void AbortCurrentPlan(bool bForceDeferToNextFrame = false);
	void ForceFinishAbortingTasks();
	void CancelActivePlanning();

	void Reset();

	void OnTaskFinished(const UHTNTask* Task, uint8* TaskMemory, const FHTNPlanStepID& FinishedStepID, EHTNNodeResult Result);
	
	// This is called by decorators to notify of their conditions changing in an event-based way. See UHTNDecorator::NotifyEventBasedCondition
	// Note that bFinalConditionValue is not raw: it's after taking into account if the Decorator condition is inversed.
	bool NotifyEventBasedDecoratorCondition(const UHTNDecorator* Decorator, const uint8* NodeMemory, bool bFinalConditionValue, bool bCanAbortPlanInstantly);

	// Gets the memory of a node in the CurrentPlan at the given memory offset (which is determined for each node when initializing a plan for execution).
	uint8* GetNodeMemory(uint16 MemoryOffset) const;
	// Gets the StepID(s) of the plan step(s) that will follow immediately after the given plan step.
	// Set bIsExecutingPlan to false to prevent Parallel secondary branches from looping forever if you intend to traverse the whole plan with this 
	// (e.g., for plan rechecking or vislogging).
	int32 GetNextPrimitiveStepsInCurrentPlan(TArray<FHTNPlanStepID>& OutStepIds, const FHTNPlanStepID& StepID, bool bIsExecutingPlan = true) const;

	FString GetLogPrefix() const;

	FOnHTNPlanInstanceStartedPlanExecution PlanExecutionStartedEvent;
	FOnHTNPlanInstanceFinishedPlanExecution PlanExecutionFinishedEvent;
	FOnHTNPlanInstanceFinished PlanInstanceFinishedEvent;

private:
	void UpdateCurrentExecutionAndPlanning();
	void ProcessDeferredActions();
	void StartPlanningTask(bool bDeferToNextFrame = false);

	void OnPlanningTaskFinished(class UAITask_MakeHTNPlan& Sender, TSharedPtr<FHTNPlan> ProducedPlan);
	void OnNewPlanProduced(TSharedPtr<FHTNPlan> ProducedPlan);
	bool StartPendingPlanExecution();

	// Allocates memory for plan nodes, instances them if required, initializes the instances and prepares for execution.
	void InitializeCurrentPlan(TSharedRef<FHTNPlan> NewPlan);
	void ClearCurrentPlan();

	void TickCurrentPlan(float DeltaTime);
	void StartTasksPendingExecution();

	EHTNNodeResult StartExecuteTask(const FHTNPlanStepID& PlanStepID);
	bool UpdateSubNodes(const FHTNPlanStepID& PlanStepID, uint8 UpdateFlags, float DeltaTime = 0.0f);
	bool CheckConditionsOfDecoratorsInGroup(const struct FHTNSubNodeGroup& Group, EHTNDecoratorConditionCheckType CheckType, const FHTNPlanStepID& CheckedPlanStepID);
	void FinishSubNodesAtPlanStep(const FHTNPlanStepID& PlanStepID, EHTNNodeResult Result);
	void StartSubNodesInSubNodeGroup(const FHTNSubNodeGroup& SubNodeGroup);
	void FinishSubNodesInSubNodeGroup(const FHTNSubNodeGroup& SubNodeGroup, EHTNNodeResult Result);

	void GetSubNodesInCurrentPlanToStart(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const;
	void GetSubNodesInCurrentPlanToTick(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const;
	void GetSubNodesInCurrentPlanToFinish(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const;
	void GetSubNodesInCurrentPlanToAbort(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const;
	void GetSubNodesInCurrentPlanAll(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const;
	bool AnyActiveSubLevelsExcept(int32 SubLevelIndex) const;
	bool IsLevelActive(int32 LevelIndex) const;
	void RemovePendingExecutionPlanSteps();
	void RemovePendingExecutionPlanSteps(TFunctionRef<bool(const FHTNPlanStepID&)> Predicate);

	void NotifySublevelFinishedIfNeeded(const FHTNPlanStepID& FinishedStepID);
	void AbortSecondaryParallelBranchesIfNeeded(const FHTNPlanStepID& FinishedStepID);
	bool IsLevelFinishedWhenStepFinished(const FHTNPlanStepID& FinishedStepID) const;

	UHTNTask& GetTaskInCurrentPlan(const FHTNPlanStepID& ExecutingStepID) const;
	UHTNTask& GetTaskInCurrentPlan(const FHTNPlanStepID& ExecutingStepID, uint8*& OutTaskMemory) const;

	void AbortExecutingPlanStep(FHTNPlanStepID PlanStepID);

	void OnPlanAbortFinished();
	void OnPlanExecutionSuccessfullyFinished();
	void StopInternal(bool bDisregardLatentAbort = false, TOptional<EHTNPlanInstanceStatus> PostStopStatusOverride = TOptional<EHTNPlanInstanceStatus>());

	void NotifyPlanInstanceFinished();

	void NotifyNodesOnPlanExecutionStarted();
	void NotifyNodesOnPlanExecutionFinished(EHTNPlanExecutionFinishedResult Result);
	void NotifyNodesOnPlanExecutionHelper(TFunctionRef<void(UHTNNode* /*TemplateNode*/, uint16 /*NodeMemoryOffset*/)> Callable);

#if ENABLE_VISUAL_LOG
	void DescribeActivePlanStepToVisLog(const FHTNPlanStepID& StepID, struct FVisualLogStatusCategory& LogCategory) const;
#endif

	EHTNPlanInstanceStatus GetStatusAfterSucceeded() const;
	EHTNPlanInstanceStatus GetStatusAfterFailed() const;
	bool CanLoop() const;
	bool ShouldReusePrePlannedPlan() const;

	UPROPERTY()
	EHTNPlanInstanceStatus Status;

	UPROPERTY()
	FHTNPlanInstanceConfig Config;

	UPROPERTY()
	TObjectPtr<UHTNComponent> OwnerComponent;

	UPROPERTY()
	FHTNPlanInstanceID ID;

	UPROPERTY()
	TObjectPtr<class UAITask_MakeHTNPlan> CurrentPlanningTask;

	// Set when a planning task completes. Is stored separately in case the previous plan (CurrentPlan) takes some time to abort. 
	TSharedPtr<FHTNPlan> PlanPendingExecution;

	TSharedPtr<FHTNPlan> CurrentPlan;

	TArray<FHTNPlanStepID> CurrentlyExecutingStepIDs;
	TArray<FHTNPlanStepID> PendingExecutionStepIDs;
	TArray<FHTNPlanStepID> CurrentlyAbortingStepIDs;

	// Memory of nodes in the current plan.
	// 
	// It is important that this is heap-allocated, because that way a pointer 
	// to somewhere in here remains valid even if the containing UHTNPlanInstance gets moved 
	// to elsewhere in memory (e.g., when resizing the UHTNComponent::SubPlanInstances map).
	UPROPERTY()
	TArray<uint8> PlanMemory;

	// Instances of nodes that were created for the current plan.
	// Only for nodes that have bCreateNodeInstance enabled.
	UPROPERTY()
	TArray<TObjectPtr<UHTNNode>> NodeInstances;

	EHTNLockFlags LockFlags;

	// If this is set, then we're in the middle of a Replan with these parameters.
	TOptional<FHTNReplanParameters> ActiveReplanParameters;

	// How many times has this instance executed a plan (or tried to execute one).
	int32 PlanExecutionCount;

	// It is possible that the CurrentPlan is set but hasn't started execution 
	// (e.g., initializing a SubPlan long before actually starting it)
	uint8 bCurrentPlanStartedExecution : 1;

	uint8 bDeferredAbortPlan : 1;
	uint8 bAbortingPlan : 1;
	uint8 bDeferredStartPlanningTask : 1;

	uint8 bBlockNotifyPlanInstanceFinished : 1;
};
