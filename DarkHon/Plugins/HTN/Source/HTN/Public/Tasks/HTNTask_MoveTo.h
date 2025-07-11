// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "Tasks/HTNTask_BlackboardBase.h"
#include "AITask_HTNMoveTo.h"
#include "HTNTask_MoveTo.generated.h"

struct FHTNMoveToTaskMemory
{
	FDelegateHandle BBObserverDelegateHandle;
	FVector PreviousGoalLocation;

	TWeakObjectPtr<UAITask_HTNMoveTo> AITask;
	uint8 bObserverCanFinishTask : 1;
};

UCLASS(config=Game)
class HTN_API UHTNTask_MoveTo : public UHTNTask_BlackboardBase
{
	GENERATED_BODY()

public:
	// Fixed distance added to threshold between AI and goal location in destination reach test
	UPROPERTY(config, Category = Node, EditAnywhere, Meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AcceptableRadius;

	// "None" will result in default filter being used
	UPROPERTY(Category = Node, EditAnywhere)
	TSubclassOf<UNavigationQueryFilter> FilterClass;
	
	// If task is expected to react to changes to location represented by BB key
	// this property can be used to tweak sensitivity of the mechanism. Value is
	// recommended to be less then AcceptableRadius.
	// This only matters if the BB key is a vector key. For other types, the path is always recalculated as long as bObserveBlackboardValue is enabled.
	UPROPERTY(Category = Blackboard, EditAnywhere, Meta = (ClampMin = "1", UIMin = "1", EditCondition = "bObserveBlackboardValue && !bAlwaysRepathWhenBlackboardValueSet", DisplayAfter = "bAlwaysRepathWhenBlackboardValueSet"))
	float ObservedBlackboardValueTolerance;
	
	// If move goal in BB changes the move will be redirected to new location
	UPROPERTY(Category = Blackboard, EditAnywhere)
	uint32 bObserveBlackboardValue : 1;

	// If true, whenever the BB key is set, a new path will be calculated even if the location did not change.
	UPROPERTY(Category = Blackboard, EditAnywhere, Meta = (EditCondition = "bObserveBlackboardValue"))
	uint32 bAlwaysRepathWhenBlackboardValueSet : 1;

	UPROPERTY(Category = Node, EditAnywhere)
	uint32 bAllowStrafe : 1;

	// If set, use incomplete path when goal can't be reached
	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay)
	uint32 bAllowPartialPath : 1;

	// If set, path to goal actor will update itself when actor moves.
	// Note that this does not work if the target is a vector or scene component. 
	// It only works when moving to actors or non-scene components.
	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay)
	uint32 bTrackMovingGoal : 1;

	// If set, the goal location will need to be navigable (will need to be on the navmesh).
	// Set this to false and "Allow Partial Path" to true if the destination is the center of a building that blocks the navmesh.
	// In that case the pawn will move to a point as close as possible to the destination.
	//
	// Note: this feature is only available in UE5.2 and up.
	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay)
	uint32 bRequireNavigableEndLocation : 1;

	// If set, goal location will be projected on navigation data (navmesh) before using
	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay)
	uint32 bProjectGoalLocation : 1;

	// If set, radius of AI's capsule will be added to threshold between AI and goal location in destination reach test
	UPROPERTY(Category = Node, EditAnywhere)
	uint32 bReachTestIncludesAgentRadius : 1;

	// if set, radius of goal's capsule will be added to threshold between AI and goal location in destination reach test
	UPROPERTY(Category = Node, EditAnywhere)
	uint32 bReachTestIncludesGoalRadius : 1;

	uint32 bUsePathfinding : 1;

	// If set, the planning cost of the task will be determined by the estimated pathfinding length (or cost), computed during planning.
	// If not set, the distance between the start location and the goal location, projected to navmesh, will be used instead.
	UPROPERTY(EditAnywhere, Category = Planning)
	uint32 bTestPathDuringPlanning : 1;

	// Only relevant if Test Path During Planning is set.
	// If set, estimated path cost will be used instead of path length when computing the planning cost of the MoveTo task.
	UPROPERTY(EditAnywhere, Category = Planning, Meta = (EditCondition = "bTestPathDuringPlanning"))
	uint32 bUsePathCostInsteadOfLength : 1;

	// Only relevant if Test Path During Planning is set.
	// String pulling is a post processing step of the pathfinding system, which certain pathfinding components (such as UCrowdFollowingComponent) disable.
	// If this is set, string pulling will be enabled regardless for the plan-time pathfinding test.
	// 
	// Without string pulling the computed path length might be longer than the actual shortest path length, which may produce an inaccurate plan cost and an incorrect path end location.
	// Ticking this avoids that issue.
	UPROPERTY(EditAnywhere, Category = Planning, Meta = (EditCondition = "bTestPathDuringPlanning"))
	uint32 bForcePlanTimeStringPulling : 1;
	
	// Cost multiplier for the planning cost of the task.
	UPROPERTY(EditAnywhere, Category = Planning, Meta = (ClampMin = "0"))
	float CostPerUnitPathLength;

	UHTNTask_MoveTo(const FObjectInitializer& ObjectInitializer);

	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;

	virtual uint16 GetInstanceMemorySize() const override;
	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) override;
	virtual void OnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult TaskResult) override;
	virtual FString GetStaticDescription() const override;
	
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;
	EBlackboardNotificationResult OnBlackboardValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory);

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif
	
protected:
	EHTNNodeResult PerformMoveTask(UHTNComponent& OwnerComp, FHTNMoveToTaskMemory& Memory);
	virtual UAITask_HTNMoveTo* PrepareMoveTask(const UHTNComponent& OwnerComp, const FHTNMoveToTaskMemory& Memory, const FAIMoveRequest& MoveRequest);

private:
	bool PlanTimeTestPath(UHTNComponent& OwnerComp, const FVector& RawStartLocation, const FVector& RawGoalLocation, FVector& OutEndLocation, float& OutPathCostEstimate) const;
	int32 GetTaskCostFromPathLength(float PathLength) const;
	
	bool MakeMoveRequest(UHTNComponent& OwnerComp, FHTNMoveToTaskMemory& Memory, FAIMoveRequest& OutMoveRequest) const;
};
