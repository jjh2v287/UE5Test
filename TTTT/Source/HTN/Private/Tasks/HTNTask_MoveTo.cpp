// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_MoveTo.h"
#include "AIController.h"
#include "AISystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavMesh/RecastNavMesh.h"
#include "Misc/EngineVersionComparison.h"
#include "VisualLogger/VisualLogger.h"
#include "HTNPlanStep.h"

UHTNTask_MoveTo::UHTNTask_MoveTo(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bObserveBlackboardValue(false),
	bAlwaysRepathWhenBlackboardValueSet(false),
	bTestPathDuringPlanning(true),
	bUsePathCostInsteadOfLength(false),
	bForcePlanTimeStringPulling(false),
	CostPerUnitPathLength(1.0f)
{
	NodeName = TEXT("Move To");
	bNotifyTaskFinished = true;
	
	AcceptableRadius = GET_AI_CONFIG_VAR(AcceptanceRadius);
	bReachTestIncludesGoalRadius = bReachTestIncludesAgentRadius = GET_AI_CONFIG_VAR(bFinishMoveOnGoalOverlap);
	bAllowStrafe = GET_AI_CONFIG_VAR(bAllowStrafing);
	bAllowPartialPath = GET_AI_CONFIG_VAR(bAcceptPartialPaths);
	bTrackMovingGoal = true;
	bRequireNavigableEndLocation = true;
	bProjectGoalLocation = true;
	bUsePathfinding = true;

	ObservedBlackboardValueTolerance = AcceptableRadius * 0.95f;

	// Accept only actors, components, and vectors.
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UHTNTask_MoveTo, BlackboardKey), AActor::StaticClass());
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UHTNTask_MoveTo, BlackboardKey), UActorComponent::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UHTNTask_MoveTo, BlackboardKey));
}

void UHTNTask_MoveTo::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	const FVector RawLocationOnStart = WorldState->GetValue<UBlackboardKeyType_Vector>(FBlackboard::KeySelfLocation);
	if (!FAISystem::IsValidLocation(RawLocationOnStart))
	{
		PlanningTask.SetNodePlanningFailureReason(TEXT("start location was invalid"));
		UE_VLOG(&OwnerComp, LogHTN, Warning, TEXT("%s: start location is invalid!"), *GetShortDescription());
		return;
	}
	
	const FVector RawTargetLocation = WorldState->GetLocation(BlackboardKey);
	if (!FAISystem::IsValidLocation(RawTargetLocation))
	{
		PlanningTask.SetNodePlanningFailureReason(TEXT("target location was invalid"));
		UE_VLOG(&OwnerComp, LogHTN, Warning, TEXT("%s: target location is invalid!"), *GetShortDescription());
		return;
	}
	
	FVector LocationOnEnd;
	float PathCostEstimate = -1.0f;
	if (!PlanTimeTestPath(OwnerComp, RawLocationOnStart, RawTargetLocation, LocationOnEnd, PathCostEstimate))
	{
		PlanningTask.SetNodePlanningFailureReason(TEXT("plan-time test failed"));
		return;
	}

	const TSharedRef<FBlackboardWorldState> NewWorldState = WorldState->MakeNext();
	NewWorldState->SetValue<UBlackboardKeyType_Vector>(FBlackboard::KeySelfLocation, LocationOnEnd);
	PlanningTask.SubmitPlanStep(this, NewWorldState, GetTaskCostFromPathLength(PathCostEstimate));
}

uint16 UHTNTask_MoveTo::GetInstanceMemorySize() const
{
	return sizeof(FHTNMoveToTaskMemory);
}

EHTNNodeResult UHTNTask_MoveTo::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	FHTNMoveToTaskMemory* const Memory = CastInstanceNodeMemory<FHTNMoveToTaskMemory>(NodeMemory);
	Memory->PreviousGoalLocation = FAISystem::InvalidLocation;
	Memory->bObserverCanFinishTask = true;

	const EHTNNodeResult NodeResult = PerformMoveTask(OwnerComp, *Memory);

	// Subscribe to blackboard if needed
	if (NodeResult == EHTNNodeResult::InProgress && bObserveBlackboardValue)
	{
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (ensure(BlackboardComp))
		{
			if (Memory->BBObserverDelegateHandle.IsValid())
			{
				UE_VLOG(&OwnerComp, LogHTN, Warning, TEXT("UHTNTask_MoveTo::ExecuteTask '%s' Old BBObserverDelegateHandle is still valid! Removing old Observer."), *GetShortDescription());
				BlackboardComp->UnregisterObserver(BlackboardKey.GetSelectedKeyID(), Memory->BBObserverDelegateHandle);
			}

			Memory->BBObserverDelegateHandle = BlackboardComp->RegisterObserver(BlackboardKey.GetSelectedKeyID(), this, 
				FOnBlackboardChangeNotification::CreateUObject(this, &ThisClass::OnBlackboardValueChange, NodeMemory));
		}
	}

	return NodeResult;
}

void UHTNTask_MoveTo::OnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult TaskResult)
{
	FHTNMoveToTaskMemory* const Memory = CastInstanceNodeMemory<FHTNMoveToTaskMemory>(NodeMemory);

	if (TaskResult == EHTNNodeResult::Aborted)
	{
		Memory->bObserverCanFinishTask = false;
		if (UAITask_HTNMoveTo* const MoveTask = Memory->AITask.Get())
		{
			MoveTask->ExternalCancel();
		}
		else
		{
			UE_VLOG(&OwnerComp, LogHTN, Error, TEXT("Can't abort path following! The underlying AIMoveTask is null!"));
		}
	}
	
	Memory->AITask.Reset();

	// Unsubscribe from blackboard.
	if (bObserveBlackboardValue)
	{
		UBlackboardComponent* const BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (ensure(BlackboardComp) && Memory->BBObserverDelegateHandle.IsValid())
		{
			BlackboardComp->UnregisterObserver(BlackboardKey.GetSelectedKeyID(), Memory->BBObserverDelegateHandle);
		}

		Memory->BBObserverDelegateHandle.Reset();
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UHTNTask_MoveTo::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), 
		*Super::GetStaticDescription(), 
		*BlackboardKey.SelectedKeyName.ToString());
}

void UHTNTask_MoveTo::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	// AI move task finished
	if (UAITask_HTNMoveTo* const MoveTask = Cast<UAITask_HTNMoveTo>(&Task))
	{
		if (MoveTask->GetAIController() && MoveTask->GetState() != EGameplayTaskState::Paused)
		{
			if (UHTNComponent* const HTNComp = GetHTNComponentByTask(Task))
			{
				// We use FindActiveTaskInfo().NodeMemory instead of directly using the MoveTask->NodeMemory to verify that the memory is still valid.
				// This should guard against situations where the the AI move task got cancelled because of the HTN MoveTo task getting aborted,
				// but didn't actually deactivate until later, when the memory was already invalid.
				const FHTNNodeInPlanInfo TaskInfo = HTNComp->FindActiveTaskInfo(this, MoveTask->NodeMemory);
				if (FHTNMoveToTaskMemory* const Memory = CastInstanceNodeMemory<FHTNMoveToTaskMemory>(TaskInfo.NodeMemory))
				{
					if (Memory && Memory->bObserverCanFinishTask && MoveTask == Memory->AITask)
					{
						const EHTNNodeResult Result = MoveTask->WasMoveSuccessful() ? EHTNNodeResult::Succeeded : EHTNNodeResult::Failed;
						FinishLatentTask(*HTNComp, Result, MoveTask->NodeMemory);
					}
				}
			}
		}
	}
}

EBlackboardNotificationResult UHTNTask_MoveTo::OnBlackboardValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID, uint8* NodeMemory)
{
	UHTNComponent* const HTNComp = Cast<UHTNComponent>(Blackboard.GetBrainComponent());
	if (!HTNComp)
	{
		return EBlackboardNotificationResult::RemoveObserver;
	}

	const FHTNNodeInPlanInfo TaskInfo = HTNComp->FindActiveTaskInfo(this, NodeMemory);
	FHTNMoveToTaskMemory* const Memory = CastInstanceNodeMemory<FHTNMoveToTaskMemory>(TaskInfo.NodeMemory);
	if (!Memory)
	{
		UE_VLOG(HTNComp, LogHTN, Error, TEXT("HTN MoveTo '%s' task observing BB entry while no longer being part of a plan!"), *GetShortDescription());
		return EBlackboardNotificationResult::RemoveObserver;
	}

	if (TaskInfo.Status != EHTNTaskStatus::Active)
	{
		UE_VLOG(HTNComp, LogHTN, Error, TEXT("HTN MoveTo '%s' task observing BB entry while no longer being active!"), *GetShortDescription());
		Memory->BBObserverDelegateHandle.Reset(); //-V595
		return EBlackboardNotificationResult::RemoveObserver;
	}

	if (AAIController* const AIController = HTNComp->GetAIOwner())
	{
		check(AIController->GetPathFollowingComponent());

		// Only update move if the new goal is different enough from the previous one
		bool bUpdateMove = true;
		if (!bAlwaysRepathWhenBlackboardValueSet && BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			const FVector TargetLocation = Blackboard.GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
			bUpdateMove = FVector::DistSquared(TargetLocation, Memory->PreviousGoalLocation) > FMath::Square(ObservedBlackboardValueTolerance);
		}

		if (bUpdateMove)
		{
			const EHTNNodeResult NodeResult = PerformMoveTask(*HTNComp, *Memory);
			if (NodeResult != EHTNNodeResult::InProgress)
			{
				FinishLatentTask(*HTNComp, NodeResult);
			}
		}
	}

	return EBlackboardNotificationResult::ContinueObserving;
}

#if WITH_EDITOR
FName UHTNTask_MoveTo::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.MoveTo.Icon");
}
#endif

EHTNNodeResult UHTNTask_MoveTo::PerformMoveTask(UHTNComponent& OwnerComp, FHTNMoveToTaskMemory& Memory)
{
	const AAIController* const MyController = OwnerComp.GetAIOwner();
	if (!MyController)
	{
		return EHTNNodeResult::Failed;
	}
	
	FAIMoveRequest MoveRequest;
	if (MakeMoveRequest(OwnerComp, Memory, MoveRequest) && ensure(MoveRequest.IsValid()))
	{
		const bool bReuseExistingTask = Memory.AITask.IsValid();
		if (UAITask_HTNMoveTo* const MoveTask = PrepareMoveTask(OwnerComp, Memory, MoveRequest))
		{
			Memory.bObserverCanFinishTask = false;

			if (bReuseExistingTask)
			{
				if (MoveTask->IsActive())
				{
					UE_VLOG(MyController, LogHTN, Verbose, TEXT("'%s' reusing AITask %s"), *GetShortDescription(), *MoveTask->GetName());
					MoveTask->ConditionalPerformMove();
				}
				else
				{
					UE_VLOG(MyController, LogHTN, Verbose, TEXT("'%s' reusing AITask %s, but task is not active - handing over move performing to task mechanics"), *GetShortDescription(), *MoveTask->GetName());
				}
			}
			else
			{
				Memory.AITask = MoveTask;
				UE_VLOG(MyController, LogHTN, Verbose, TEXT("'%s' task implementing move with task %s"), *GetShortDescription(), *MoveTask->GetName());
				MoveTask->ReadyForActivation();
			}

			Memory.bObserverCanFinishTask = true;
			if (MoveTask->GetState() != EGameplayTaskState::Finished)
			{
				return EHTNNodeResult::InProgress;
			}
			return MoveTask->WasMoveSuccessful() ? EHTNNodeResult::Succeeded : EHTNNodeResult::Failed;
		}
	}

	return EHTNNodeResult::Failed;
}

UAITask_HTNMoveTo* UHTNTask_MoveTo::PrepareMoveTask(const UHTNComponent& OwnerComp, const FHTNMoveToTaskMemory& Memory, const FAIMoveRequest& MoveRequest)
{
	if (UAITask_HTNMoveTo* const MoveTask = Memory.AITask.IsValid() ? Memory.AITask.Get() : NewHTNAITask<UAITask_HTNMoveTo>(OwnerComp))
	{
		MoveTask->NodeMemory = reinterpret_cast<const uint8*>(&Memory);
		MoveTask->SetUp(MoveTask->GetAIController(), MoveRequest);
		return MoveTask;
	}

	return nullptr;
}

bool UHTNTask_MoveTo::PlanTimeTestPath(UHTNComponent& OwnerComp, const FVector& RawStartLocation, 
	const FVector& RawGoalLocation, FVector& OutEndLocation, float& OutPathCostEstimate) const
{
	struct Local
	{
		static bool GetPawnAndNavData(UHTNComponent& InOwnerComp, const FVector& InRawStartLocation,
			APawn*& OutPawn, AAIController*& OutAIController, 
			ANavigationData*& OutNavData, const FNavAgentProperties*& OutNavAgentProps)
		{
			if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(InOwnerComp.GetWorld()))
			{
				OutAIController = InOwnerComp.GetAIOwner();
				if (OutAIController)
				{
					if (APawn* const Pawn = OutAIController->GetPawn())
					{
						OutNavAgentProps = &Pawn->GetNavAgentPropertiesRef();
						if (ANavigationData* const NavData = NavSys->GetNavDataForProps(*OutNavAgentProps, InRawStartLocation))
						{
							OutPawn = Pawn;
							OutNavData = NavData;
							return true;
						}
					}
				}
			}

			return false;
		}
	};

	APawn* Pawn = nullptr;
	AAIController* AIController = nullptr;
	ANavigationData* NavData = nullptr;
	const FNavAgentProperties* NavAgentProps = nullptr;
	if (!Local::GetPawnAndNavData(OwnerComp, RawStartLocation, Pawn, AIController, NavData, NavAgentProps))
	{
		UE_VLOG(&OwnerComp, LogHTN, Warning, TEXT("%s: failed to get navigation data for pawn of %s"), 
			*GetShortDescription(), *GetNameSafe(OwnerComp.GetAIOwner()));
		return false;
	}

	const TSubclassOf<UNavigationQueryFilter> EffectiveFilterClass = *FilterClass ? 
		FilterClass : AIController->GetDefaultNavigationFilterClass();
	const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, Pawn, EffectiveFilterClass);
	if (bTestPathDuringPlanning)
	{
		// Prepare pathfinding query
		FPathFindingQuery PathfindingQuery(*Pawn, *NavData, RawStartLocation, RawGoalLocation, QueryFilter);
		PathfindingQuery
			.SetAllowPartialPaths(bAllowPartialPath)
#if !UE_VERSION_OLDER_THAN(5, 2, 0)
			.SetRequireNavigableEndLocation(bRequireNavigableEndLocation)
#endif
			;

		if (UPathFollowingComponent* const PathFollowingComponent = OwnerComp.GetAIOwner()->GetPathFollowingComponent())
		{
			PathFollowingComponent->OnPathfindingQuery(PathfindingQuery);
		}
		if (bForcePlanTimeStringPulling)
		{
			PathfindingQuery.NavDataFlags &= ~ERecastPathFlags::SkipStringPulling;
		}

		// Test path
		const FPathFindingResult PathfindingResult = NavData->FindPath(*NavAgentProps, PathfindingQuery);
		if (PathfindingResult.IsSuccessful())
		{
			const FNavigationPath& Path = *PathfindingResult.Path;
			OutEndLocation = Path.GetEndLocation() + FVector(0.0f, 0.0f, Pawn->GetDefaultHalfHeight());
			OutPathCostEstimate = bUsePathCostInsteadOfLength ? Path.GetCost() : Path.GetLength();
			return true;
		}
		UE_VLOG_ARROW(&OwnerComp, LogHTN, Warning, RawStartLocation, RawGoalLocation, FColor::Red, 
			TEXT("%s: failed plan-time test with AllowPartialPath=%s"), 
			*GetShortDescription(), bAllowPartialPath ? TEXT("true") : TEXT("false"));
	}
	else
	{
		FVector GoalLocation = RawGoalLocation;
		
		const float SearchRadius = NavAgentProps->AgentRadius * 2.0f;
		const FVector ProjectionExtent(SearchRadius, SearchRadius, BIG_NUMBER);
		FNavLocation ProjectedGoalLocation;
		if (NavData->ProjectPoint(RawGoalLocation, ProjectedGoalLocation, ProjectionExtent, QueryFilter))
		{
			GoalLocation = ProjectedGoalLocation.Location;
		}
		
		OutEndLocation = GoalLocation + FVector(0.0f, 0.0f, Pawn->GetDefaultHalfHeight());
		OutPathCostEstimate = FVector::Distance(RawStartLocation, GoalLocation);
		return true;
	}
	
	return false;
}

int32 UHTNTask_MoveTo::GetTaskCostFromPathLength(float PathLength) const
{
	if (PathLength <= 0.0f)
	{
		return 0;
	}

	return FMath::CeilToInt(PathLength * CostPerUnitPathLength);
}

bool UHTNTask_MoveTo::MakeMoveRequest(UHTNComponent& OwnerComp, FHTNMoveToTaskMemory& Memory, FAIMoveRequest& OutMoveRequest) const
{
	const UBlackboardComponent* const MyBlackboard = OwnerComp.GetBlackboardComponent();
	if (!MyBlackboard)
	{
		return false;
	}

	const AAIController* const MyController = OwnerComp.GetAIOwner();
	if (!MyController)
	{
		return false;
	}
	
	OutMoveRequest.SetNavigationFilter(*FilterClass ? FilterClass : MyController->GetDefaultNavigationFilterClass());
	OutMoveRequest.SetAllowPartialPath(bAllowPartialPath);
	OutMoveRequest.SetAcceptanceRadius(AcceptableRadius);
	OutMoveRequest.SetCanStrafe(bAllowStrafe);
	OutMoveRequest.SetReachTestIncludesAgentRadius(bReachTestIncludesAgentRadius);
	OutMoveRequest.SetReachTestIncludesGoalRadius(bReachTestIncludesGoalRadius);\

#if !UE_VERSION_OLDER_THAN(5, 2, 0)
	OutMoveRequest.SetRequireNavigableEndLocation(bRequireNavigableEndLocation);
#endif
	OutMoveRequest.SetProjectGoalLocation(bProjectGoalLocation);
	OutMoveRequest.SetUsePathfinding(bUsePathfinding);

	const auto UseActor = [&](const AActor& Actor)
	{
		if (bTrackMovingGoal)
		{
			OutMoveRequest.SetGoalActor(&Actor);
		}
		else
		{
			OutMoveRequest.SetGoalLocation(Actor.GetActorLocation());
		}
	};

	const auto UseLocation = [&](const FVector& Location)
	{
		OutMoveRequest.SetGoalLocation(Location);
		Memory.PreviousGoalLocation = Location;
	};

	// Set target
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector TargetLocation = MyBlackboard->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
		UseLocation(TargetLocation);
	}
	else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		const UObject* const KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
		if (const AActor* const TargetActor = Cast<const AActor>(KeyValue))
		{
			UseActor(*TargetActor);
		}
		else if (const UActorComponent* const ActorComponent = Cast<const UActorComponent>(KeyValue))
		{
			if (const USceneComponent* const SceneComponent = Cast<const USceneComponent>(KeyValue))
			{
				UseLocation(SceneComponent->GetComponentLocation());
			}
			else if (const AActor* const OwnerActor = ActorComponent->GetOwner(); ensure(OwnerActor))
			{
				UseActor(*OwnerActor);
			}
			else
			{
				UE_VLOG(MyController, LogHTN, Warning, TEXT("UHTNTask_MoveTo::ExecuteTask tried to go to component %s that has no actor"),
					*BlackboardKey.SelectedKeyName.ToString());
				return false;
			}
		}
		else
		{
			UE_VLOG(MyController, LogHTN, Warning, TEXT("UHTNTask_MoveTo::ExecuteTask tried to go to actor while BB %s entry was empty"), 
				*BlackboardKey.SelectedKeyName.ToString());
			return false;
		}
	}
	else
	{
		UE_VLOG(MyController, LogHTN, Warning, TEXT("UHTNTask_MoveTo::ExecuteTask tried to go to an unsupported BB key %s of type %s"), 
			*BlackboardKey.SelectedKeyName.ToString(),
			*GetNameSafe(BlackboardKey.SelectedKeyType));
		return false;
	}
	
	return true;
}
