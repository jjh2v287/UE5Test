// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EngineDefines.h"
#include "Misc/EnumClassFlags.h"
#include "Misc/EnumRange.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 6, 0)
#include "Stats/Stats2.h"
#else
#include "Stats/Stats.h"
#endif

#include "HTNTypes.generated.h"

// Miscellaneous types and definitions used in the Hierarchical Task Networks plugin.

#define USE_HTN_DEBUGGER (1 && WITH_EDITORONLY_DATA)
#define HTN_DEBUG_PLANNING (1 && ENABLE_VISUAL_LOG)

// When a new plan begins execution, the nodes that plan may call UBlackboardComponent::RegisterObserver
// to get a callback whenever a blackboard key changes value. 
// This is not safe to do if done from UBlackboardComponent::NotifyObservers,
// since that would mean mutating a collection that's being iterated over. 
// That is an engine bug and this macro enables a workaround for it.
// Change this from 0 to 1 if you build from engine source and have 
// that engine bug fixed locally (or if an engine update fixes it).
#define HTN_ALLOW_START_PLAN_DURING_BLACKBOARD_NOTIFY_OBSERVERS 0

// In 4.25, UProperty etc. were renamed to FProperty etc.
// The code in the plugin uses the new naming, so these preprocessor definitions
// allow the plugin to compile successfully even on versions below 4.25
#if UE_VERSION_OLDER_THAN(4, 25, 0)
	#define FField UField
	#define FProperty UProperty
	#define FStructProperty UStructProperty
#endif

#if UE_VERSION_OLDER_THAN(5, 5, 0)
	#define HTN_DISALLOW_SHRINKING false
#else
	#define HTN_DISALLOW_SHRINKING EAllowShrinking::No
#endif

HTN_API DECLARE_LOG_CATEGORY_EXTERN(LogHTN, Log, All);
HTN_API DECLARE_LOG_CATEGORY_EXTERN(LogHTNCurrentPlan, Display, All);

// Statistics groups for stat tracking and profiling
DECLARE_STATS_GROUP(TEXT("HTN"), STATGROUP_AI_HTN, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Tick"), STAT_AI_HTN_Tick, STATGROUP_AI_HTN, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Planning"), STAT_AI_HTN_Planning, STATGROUP_AI_HTN, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Execution Time"), STAT_AI_HTN_Execution, STATGROUP_AI_HTN, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Cleanup Time"), STAT_AI_HTN_Cleanup, STATGROUP_AI_HTN, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Stop HTN Time"), STAT_AI_HTN_StopHTN, STATGROUP_AI_HTN, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Node Instantiation Time"), STAT_AI_HTN_NodeInstantiation, STATGROUP_AI_HTN, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Node Instance Initialization Time"), STAT_AI_HTN_NodeInstanceInitialization, STATGROUP_AI_HTN, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num AITask_MakeHTNPlan"), STAT_AI_HTN_NumAITask_MakeHTNPlan, STATGROUP_AI_HTN, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Produced Plans"), STAT_AI_HTN_NumProducedPlans, STATGROUP_AI_HTN, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Node Instances Created"), STAT_AI_HTN_NumNodeInstancesCreated, STATGROUP_AI_HTN, );

UENUM(BlueprintType)
enum class EHTNNodeResult : uint8
{
	// Finished as success
	Succeeded,
	// Finished as failure
	Failed,
	// Finished aborting = failure
	Aborted,
	// Not finished yet
	InProgress
};

UENUM(BlueprintType)
enum class EHTNTaskStatus : uint8
{
	Active,
	Aborting,
	Inactive
};

UENUM(BlueprintType)
enum class EHTNPlanExecutionFinishedResult : uint8
{
	Succeeded,
	FailedOrAborted
};

enum class EHTNSubNodeType : int32
{
	Decorator,
	Service
};

namespace FBlackboard
{
	// The location of the character at a particular point in the plan.
	const FName KeySelfLocation = TEXT("SelfLocation");
}

namespace FHTNNames
{
	// Used by HTNTask_EQSQuery to let EQS contexts know if they're running during planning.
	const FName IsPlanTimeQuery = TEXT("HTNInternal_EQSParamName_IsPlanTimeQuery");
}

struct HTN_API FHTNPlanStepID
{
	static const FHTNPlanStepID None;

	// The index (in the FHTNPlan::Levels array) of the step's plan level.
	int32 LevelIndex = INDEX_NONE;

	// The index (in the FHTNPlanLevel::Steps array) of the step in its plan level.
	int32 StepIndex = INDEX_NONE;

	FORCEINLINE friend bool operator==(const FHTNPlanStepID& A, const FHTNPlanStepID& B)
	{
		return A.LevelIndex == B.LevelIndex && A.StepIndex == B.StepIndex;
	}

	FORCEINLINE friend bool operator!=(const FHTNPlanStepID& A, const FHTNPlanStepID& B)
	{
		return A.LevelIndex != B.LevelIndex || A.StepIndex != B.StepIndex;
	}

	FORCEINLINE FHTNPlanStepID MakeNext() const
	{
		return FHTNPlanStepID { LevelIndex, StepIndex + 1 };
	}
};

FORCEINLINE HTN_API uint32 GetTypeHash(const FHTNPlanStepID& StepID)
{
	return HashCombine(GetTypeHash(StepID.LevelIndex), GetTypeHash(StepID.StepIndex));
}

// Used in FHTNPlan::PriorityMarkers to deprioritize some plans relative to others. 
// This is necessary for HTNNode_Prefer to work.
// See FHTNPlan::PriorityMarkers for more info.
using FHTNPriorityMarker = int16;

UENUM(BlueprintType)
enum class EHTNDecoratorConditionCheckType : uint8
{
	// Planner is entering this decorator
	PlanEnter,
	// Planner is exiting this decorator
	PlanExit,
	// Plan is being rechecked during execution
	PlanRecheck,
	// Plan is being executed
	Execution
};

UENUM(BlueprintType)
enum class EHTNDecoratorTestResult : uint8
{
	Failed = 0,
	Passed = 1,
	NotTested = 2
};
EHTNDecoratorTestResult HTN_API CombineHTNDecoratorTestResults(EHTNDecoratorTestResult Lhs, EHTNDecoratorTestResult Rhs, bool bShouldPass = true);
bool HTN_API HTNDecoratorTestResultToBool(EHTNDecoratorTestResult TestResult, bool bShouldPass = true);

// A unique ID for a plan instance. 
// Exists to allow plan instances to be uniquely identified even if pooling of plan instances is introduced.
USTRUCT()
struct HTN_API FHTNPlanInstanceID
{
	GENERATED_BODY()

	enum EGenerateNewIDType
	{
		GenerateNewID
	};

	FHTNPlanInstanceID();
	FHTNPlanInstanceID(EGenerateNewIDType);

	FORCEINLINE bool IsValid() const { return ID != 0; }
	friend FORCEINLINE bool operator==(const FHTNPlanInstanceID& Lhs, const FHTNPlanInstanceID& Rhs) { return Lhs.ID == Rhs.ID; }
	friend FORCEINLINE bool operator!=(const FHTNPlanInstanceID& Lhs, const FHTNPlanInstanceID& Rhs) { return Lhs.ID != Rhs.ID; }
	friend FORCEINLINE uint32 GetTypeHash(const FHTNPlanInstanceID& Key) { return GetTypeHash(Key.ID); }

	UPROPERTY()
	uint64 ID;
};

struct HTN_API FHTNDebugExecutionPerPlanInstanceInfo
{
	TSharedPtr<struct FHTNPlan> HTNPlan;

	TArray<FHTNPlanStepID, TInlineAllocator<8>> ActivePlanStepIDs;
};

// A snapshot of the state of an HTNComponent for debug purposes.
struct HTN_API FHTNDebugExecutionStep
{
	TMap<FHTNPlanInstanceID, FHTNDebugExecutionPerPlanInstanceInfo, TInlineSetAllocator<8>> PerPlanInstanceInfo;

	// Descriptions of blackboard values at this step
	TMap<FName, FString, TInlineSetAllocator<64>> BlackboardValues;

	int32 DebugStepIndex = 0;
};

class HTN_API FHTNDebugSteps
{
public:
	FHTNDebugExecutionStep& Add_GetRef();
	void Reset();
	const FHTNDebugExecutionStep* GetByIndex(int32 Index) const;
	FHTNDebugExecutionStep* GetByIndex(int32 Index);
	int32 GetLastIndex() const;

private:
	TArray<FHTNDebugExecutionStep> Steps;
};

// Information about a currently executing node.
// Note that this struct is not safe to store and should only be used as a temporary container for values.
struct FHTNNodeInPlanInfo
{
	FORCEINLINE explicit operator bool() const { return IsValid(); }
	FORCEINLINE bool IsValid() const { return PlanInstance != nullptr; }

	class UHTNPlanInstance* PlanInstance = nullptr;

	uint8* NodeMemory = nullptr;

	EHTNTaskStatus Status = EHTNTaskStatus::Inactive;

	FHTNPlanStepID PlanStepID;
};

// The status of a UHTNPlanInstance
UENUM()
enum class EHTNPlanInstanceStatus : uint8
{
	// Hasn't started planning or executing yet.
	NotStarted,
	// Still planning or executing
	InProgress,
	// Finished as success
	Succeeded,
	// Failed to make or execute a plan
	Failed
};

UENUM(Meta = (BitFlags, UseEnumValuesAsMaskValuesInEditor))
enum class EHTNLockFlags : uint8
{
	None = 0,
	LockTick = 1 << 0,
	LockAbortPlan = 1 << 1,
	LockOnTaskFinished = 1 << 2
};
ENUM_CLASS_FLAGS(EHTNLockFlags);
ENUM_RANGE_BY_VALUES(EHTNLockFlags, EHTNLockFlags::LockTick, EHTNLockFlags::LockAbortPlan, EHTNLockFlags::LockOnTaskFinished);
FString HTN_API HTNLockFlagsToString(EHTNLockFlags Flags);

// To be used by UHTNPlanInstance to change its LockFlags to defer state changes 
// (e.g., when trying to call Replan while from inside the Tick of an HTN task)
struct HTN_API FHTNScopedLock : FNoncopyable
{
	FHTNScopedLock(EHTNLockFlags& LockFlags, EHTNLockFlags FlagToAdd);
	~FHTNScopedLock();

private:
	EHTNLockFlags& LockFlags;
	EHTNLockFlags FlagToAdd;
	bool bHadFlag;
};

UENUM(BlueprintType)
enum class EHTNPlanningType : uint8
{
	// Try to make a new plan from scratch. This is run in the background (potentially over multiple frames).
	// The current plan is always aborted at the end of this replan and is replaced by the new plan.
	// If replanning fails, the current plan is aborted anyway.
	Normal,
	// Try to create a plan that is an adjustment of the current plan
	// (e.g., try to make a plan that's just like the current plan, but takes the top branch of some Prefer node 
	// instead of the bottom branch). This is run in the background (potentially over multiple frames) 
	// and only replaces the current plan if it succeeds. It only succeeds if the produced plan is actually 
	// different from the current plan. If it fails, it is silently ignored.
	// 
	// The most common use case for this is "change to a higher-priority behavior when it becomes possible" when 
	// "possible" can only be checked by planning out an entire branch of the HTN instead of just checking the conditions
	// of some decorators on an If node. This can be achieved by configuring a 
	// Prefer node with PlanAdjustmentMode == TrySwitchToHigherPriorityBranch and putting an 
	// Replan service with PlanningType == TryToAdjustCurrentPlan on a Scope in the bottom branch.
	// As a result, when executing the bottom branch of a Prefer node, the service will switch to the top branch when it becomes possible.
	//
	// Note that a plan produced in this fashion still starts execution from the beginning and not from the node 
	// at which it diverges from the current plan.
	// 
	// The plan adjustment mechanism requires active participation from the nodes in the current plan. Plan adjustment 
	// is only possible if at least one node in the current plan set bIsPotentialDivergencePointDuringPlanAdjustment
	// to true during planning. A plan is only considered a valid adjustment of the current plan if at least one node
	// set bDivergesFromCurrentPlanDuringPlanAdjustment during planning.
	// 
	// Custom nodes can participate in plan adjustment in various ways, see HTNNode_Prefer, HTNNode_If, 
	// and their PlanAdjustmentMode variable.
	TryToAdjustCurrentPlan
};

USTRUCT(BlueprintType)
struct HTN_API FHTNReplanParameters
{
	GENERATED_BODY()

	static const FHTNReplanParameters Default;

	FHTNReplanParameters();

	FString ToCompactString() const;
	FString ToTitleString() const;
	FString ToDetailedString() const;

	// The debug reason that will be logged in the Visual Logger
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan")
	FString DebugReason;

	// If true, the current plan will be immediately aborted.
	// If false, the current plan will only be aborted once the new plan is ready.
	// Making the new plan may take multiple frames as some tasks take multiple frames to plan
	// (e.g., EQSQuery tasks).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan")
	uint8 bForceAbortPlan : 1;

	// If true, and a new plan is already being made when Replan is called, planning will be restarted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan")
	uint8 bForceRestartActivePlanning : 1;

	// If true, the replan will be delayed to the next frame. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan")
	uint8 bForceDeferToNextFrame : 1;

	// If true, and there are multiple nested plans (see the SubPlan task), 
	// the Replan call will affect the outermost, top-level plan instance.
	// If false, will affect the plan instance that the node is in.
	// 
	// For example, if this is false and Replan is triggered by a node inside a SubPlan, only that SubPlan will be replanned.
	// Note that aborting a subplan causes it to fail, so whether it actually gets replanned is governed by 
	// the OnSubPlanFailed property of the SubPlan task.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan|SubPlan")
	uint8 bReplanOutermostPlanInstance : 1;

	// Aborting a subplan causes it to fail so whether it actually gets replanned 
	// is governed by the OnSubPlanFailed property of the SubPlan task. This setting can override that.
	// If true, and we're in a nested plan (see the SubPlan task) a replan will happen even if the SubPlan task 
	// does not have OnSubPlanFailed set to Loop.
	// 
	// For example, if the SubPlan task has OnSubPlanFailed set to Fail and Replan is called with this set to false,
	// the subplan will Fail instead. If this is true, it will replan regardless of the OnSubPlanFailed setting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan|SubPlan")
	uint8 bForceReplan : 1;

	// If true, and we're in a nested plan whose SubPlan task has bPlanDuringExecution=False, 
	// a new subplan will be planned regardless. 
	// If false and bPlanDuringExecution=False, then the same plan will be restarted. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan|SubPlan")
	uint8 bMakeNewPlanRegardlessOfSubPlanSettings : 1;

	// What kind of replan this is. See tooltips of individual options for more details.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replan")
	EHTNPlanningType PlanningType;
};

// What a plan instance should do once it finishes a run 
// (finished execution of a plan or failed to make a plan)
UENUM()
enum class EHTNPlanInstanceFinishReaction : uint8
{
	// The SubPlan task will Succeed.
	Succeed,
	// The SubPlan task will Fail.
	Fail,
	// The SubPlan task will restart the subplan, possibly replanning it first as specified by PlanningDuringOuterPlanExecution.
	// 
	// Note that if the SubPlan task is being aborted, it will not loop. 
	// To achieve looping until success even when aborting, use two nested SubPlan tasks:
	// the outer SubPlan with OnThisNodeAborted = WaitForSubPlanToFinish, and the inner SubPlan with OnSubPlanFailed = Loop.
	Loop
};

// An enum for UFUNCTIONs with ExpandEnumAsExecs.
UENUM(BlueprintType)
enum class EHTNReturnValueValidity : uint8
{
	Valid,
	NotValid
};

template<typename SubNodeType>
struct HTN_API THTNNodeInfo
{
	SubNodeType* TemplateNode;
	uint16 NodeMemoryOffset;
};

// Runtime information about the subnodes of a particular plan step or plan level.
struct HTN_API FHTNRuntimeSubNodesInfo
{
	TArray<THTNNodeInfo<class UHTNDecorator>> DecoratorInfos;
	TArray<THTNNodeInfo<class UHTNService>> ServiceInfos;

	TOptional<decltype(GFrameCounter)> LastFrameSubNodesTicked;
	uint8 bSubNodesExecuting : 1;

	FHTNRuntimeSubNodesInfo() :
		bSubNodesExecuting(false)
	{}
};

// A group of subnodes (decorators/services) belonging to a particular step of an HTN plan.
struct HTN_API FHTNSubNodeGroup
{
	FHTNRuntimeSubNodesInfo* SubNodesInfo;

	FHTNPlanStepID PlanStepID;
	bool bIsIfNodeFalseBranch : 1;
	bool bCanConditionsInterruptTrueBranch : 1;
	bool bCanConditionsInterruptFalseBranch : 1;

	FHTNSubNodeGroup() :
		SubNodesInfo(nullptr),
		bIsIfNodeFalseBranch(false),
		bCanConditionsInterruptTrueBranch(true),
		bCanConditionsInterruptFalseBranch(true)
	{}

	FHTNSubNodeGroup
	(
		FHTNRuntimeSubNodesInfo& SubNodesInfo,
		FHTNPlanStepID PlanStepID,
		bool bIsIfNodeFalseBranch = false,
		bool bCanConditionsInterruptTrueBranch = true,
		bool bCanConditionsInterruptFalseBranch = true
	) :
		SubNodesInfo(&SubNodesInfo),
		PlanStepID(PlanStepID),
		bIsIfNodeFalseBranch(bIsIfNodeFalseBranch),
		bCanConditionsInterruptTrueBranch(bCanConditionsInterruptTrueBranch),
		bCanConditionsInterruptFalseBranch(bCanConditionsInterruptFalseBranch)
	{}

	FORCEINLINE bool IsValid() const { return SubNodesInfo != nullptr; }
	FORCEINLINE explicit operator bool() const { return IsValid(); }
	bool CanDecoratorConditionsInterruptPlan() const;
};

// A collection subnodes (decorators/services) in an HTN plan. 
// Each FHTNSubNodeGroup in the array comes from a different plan step.
using FHTNSubNodeGroups = TArray<FHTNSubNodeGroup, TInlineAllocator<64>>;

using FHTNNextNodesBuffer = TArray<TObjectPtr<class UHTNStandaloneNode>, TInlineAllocator<16>>;
