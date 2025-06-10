// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "AITask_MakeHTNPlan.h"
#include "HTN.h"
#include "HTNPlan.h"
#include "HTNDecorator.h"
#include "HTNTask.h"
#include "WorldStateProxy.h"

#include "Algo/Accumulate.h"
#include "Algo/AnyOf.h"
#include "Algo/MinElement.h"
#include "Algo/NoneOf.h"
#include "Algo/Partition.h"
#include "GameplayTasksComponent.h"
#include "Misc/RuntimeErrors.h"
#include "Misc/ScopeExit.h"
#include "VisualLogger/VisualLogger.h"
#include <atomic>

#if HTN_DEBUG_PLANNING && ENABLE_VISUAL_LOG

#define SAVE_PLANNING_STEP_SUCCESS(AddedNode, NewPlan, StepDescription) if (FVisualLogger::IsRecording()) DebugInfo.AddNode(CurrentPlanToExpand, AddedNode, NewPlan, TEXT(""), StepDescription);
#define SAVE_PLANNING_STEP_FAILURE(AddedNode, FailureMessage) if (FVisualLogger::IsRecording()) DebugInfo.AddNode(CurrentPlanToExpand, AddedNode, nullptr, FailureMessage);
#define SET_NODE_FAILURE_REASON(FailureMessage) if (FVisualLogger::IsRecording()) NodePlanningFailureReason = FailureMessage;

#else

#define SAVE_PLANNING_STEP_SUCCESS(AddedNode, NewPlan, StepDescription)
#define SAVE_PLANNING_STEP_FAILURE(AddedNode, FailureMessage)
#define SET_NODE_FAILURE_REASON(FailureMessage)

#endif

namespace
{
	struct FCompareHTNPlanCosts
	{
		FORCEINLINE bool operator()(const TSharedPtr<FHTNPlan>& A, const TSharedPtr<FHTNPlan>& B) const
		{
			return A.IsValid() && B.IsValid() ?
				A->Cost < B->Cost :
				B.IsValid();
		}
	};

	int32 GetTotalNumSteps(const FHTNPlan& Plan)
	{
		const int32 NumSteps = Algo::Accumulate(Plan.Levels, 0, [](int32 Sum, const TSharedPtr<FHTNPlanLevel>& Level) -> int32 { return Sum + Level->Steps.Num(); });
		return NumSteps;
	}
}

FHTNPlanningContext::FHTNPlanningContext() :
	PlanningTask(nullptr),
	AddingNode(nullptr),
	PlanToExpand(nullptr),
	CurrentPlanStepID(FHTNPlanStepID::None),
	WorldStateAfterEnteringDecorators(nullptr),
	bDecoratorsPassed(false),
	NumAddedCandidatePlans(0)
{}

FHTNPlanningContext::FHTNPlanningContext(UAITask_MakeHTNPlan* PlanningTask, UHTNStandaloneNode* AddingNode,
	TSharedPtr<FHTNPlan> PlanToExpand, const FHTNPlanStepID& PlanStepID,
	TSharedPtr<FBlackboardWorldState> WorldStateAfterEnteringDecorators, bool bDecoratorsPassed
) :
	PlanningTask(PlanningTask),
	AddingNode(AddingNode),
	PlanToExpand(PlanToExpand),
	CurrentPlanStepID(PlanStepID),
	WorldStateAfterEnteringDecorators(WorldStateAfterEnteringDecorators),
	bDecoratorsPassed(bDecoratorsPassed),
	NumAddedCandidatePlans(0)
{}

UHTNComponent* FHTNPlanningContext::GetHTNComponent() const
{
	if (UAITask_MakeHTNPlan* const PlanningTaskRaw = PlanningTask.Get())
	{
		return PlanningTaskRaw->GetOwnerComponent();
	}
	
	return nullptr;
}

UHTNPlanInstance* FHTNPlanningContext::GetPlanInstance() const
{
	if (UAITask_MakeHTNPlan* const PlanningTaskRaw = PlanningTask.Get())
	{
		return PlanningTaskRaw->GetPlanInstance();
	}

	return nullptr;
}

const FHTNPlanStep* FHTNPlanningContext::GetExistingPlanStepToAdjust() const
{
	if (PlanningTask.IsValid())
	{
		return PlanningTask->GetExistingPlanStepToAdjust();
	}

	return nullptr;
}

TSharedPtr<const FHTNPlan> FHTNPlanningContext::GetExistingPlanToAdjust() const
{
	if (PlanningTask.IsValid())
	{
		return PlanningTask->GetExistingPlanToAdjust();
	}

	return nullptr;
}

TSharedRef<FHTNPlan> FHTNPlanningContext::MakePlanCopyWithAddedStep(FHTNPlanStep*& OutAddedStep, FHTNPlanStepID& OutAddedStepID) const
{
	const TSharedRef<FHTNPlan> PlanCopy = PlanToExpand->MakeCopy(CurrentPlanStepID.LevelIndex);

	FHTNPlanLevel& Level = *PlanCopy->Levels[CurrentPlanStepID.LevelIndex];
	OutAddedStep = &Level.Steps.Emplace_GetRef(AddingNode.Get());
	OutAddedStep->WorldStateAfterEnteringDecorators = WorldStateAfterEnteringDecorators;

	OutAddedStepID = { CurrentPlanStepID.LevelIndex, Level.Steps.Num() - 1 };

	return PlanCopy;
}

int32 FHTNPlanningContext::AddLevel(FHTNPlan& NewPlan, UHTN* HTN, const FHTNPlanStepID& ParentStepID) const
{
	return NewPlan.Levels.Add(MakeShared<FHTNPlanLevel>(HTN, WorldStateAfterEnteringDecorators, ParentStepID));
}

int32 FHTNPlanningContext::AddInlineLevel(FHTNPlan& NewPlan, const FHTNPlanStepID& ParentStepID) const
{
	const FHTNPlanStepID StepID = ParentStepID != FHTNPlanStepID::None ? ParentStepID : CurrentPlanStepID;
	UHTN* const HTN = NewPlan.Levels[StepID.LevelIndex]->HTNAsset.Get();
	return NewPlan.Levels.Add(MakeShared<FHTNPlanLevel>(HTN, WorldStateAfterEnteringDecorators, ParentStepID, /*bIsInline=*/true));
}

void FHTNPlanningContext::SubmitCandidatePlan(const TSharedRef<FHTNPlan>& CandidatePlan,
	const FString& AddedStepDescription, bool bOnlyUseIfPreviousFails)
{
	const FHTNPlanStepID AddedStepID = CurrentPlanStepID.MakeNext();

#if DO_CHECK
	const FHTNPlanLevel& Level = *CandidatePlan->Levels[AddedStepID.LevelIndex];
	check(Level.Steps.IsValidIndex(AddedStepID.StepIndex));
	check(Level.Steps.Num() - 1 == AddedStepID.StepIndex);
#endif
	
	FHTNPlanStep& AddedStep = CandidatePlan->GetStep(AddedStepID);
	check(AddedStep.Node == AddingNode);

	// If a node could've diverged from the plan we're adjusting at the AddedStep but didn't, 
	// decrement the number of remaining opportunities to do so. Once it reaches zero, 
	// we'll know that we can't produce an adjustment of the current plan so we can stop trying.
	if (PlanningTask->GetPlanningType() == EHTNPlanningType::TryToAdjustCurrentPlan &&
		!CandidatePlan->bDivergesFromCurrentPlanDuringPlanAdjustment)
	{
		if (const FHTNPlanStep* const PlanStepToReplace = GetExistingPlanStepToAdjust())
		{
			if (PlanStepToReplace->bIsPotentialDivergencePointDuringPlanAdjustment)
			{
				CandidatePlan->NumRemainingPotentialDivergencePointsDuringPlanAdjustment -= 1;
			}
		}
	}

	if (AddedStep.bIsPotentialDivergencePointDuringPlanAdjustment)
	{
		CandidatePlan->NumPotentialDivergencePointsDuringPlanAdjustment += 1;
	}

	if (AddingNode->MaxRecursionLimit > 0)
	{
		CandidatePlan->IncrementRecursionCount(AddingNode.Get());
	}

	// If the node has no sublevels and no worldstate, set the worldstate to what it was after entering decorators.
	if (AddedStep.SubLevelIndex == INDEX_NONE && AddedStep.SecondarySubLevelIndex == INDEX_NONE && !AddedStep.WorldState.IsValid())
	{
		AddedStep.WorldState = AddedStep.WorldStateAfterEnteringDecorators;
	}

	if (!AddedStep.WorldState.IsValid() || PlanningTask->ExitDecoratorsAndPropagateWorldState(*CandidatePlan, AddedStepID))
	{
		++NumAddedCandidatePlans;

		if (bOnlyUseIfPreviousFails && PreviouslySubmittedPlan.IsValid())
		{
			const FHTNPriorityMarker PriorityMarker = PlanningTask->MakePriorityMarker();
			PreviouslySubmittedPlan->PriorityMarkers.Add(PriorityMarker);
			PlanningTask->PriorityMarkerCounts.FindOrAdd(PriorityMarker) += 1;
			CandidatePlan->PriorityMarkers.Add(-PriorityMarker);
		}

		PlanningTask->SubmitCandidatePlan(CandidatePlan, AddingNode.Get(), AddedStepDescription);
		PreviouslySubmittedPlan = CandidatePlan;
	}
}

FHTNPlanningID::FHTNPlanningID() :
	ID(0)
{}

FHTNPlanningID FHTNPlanningID::GenerateNewID()
{
	static std::atomic<uint64> NextID { StaticCast<uint64>(1) };

	FHTNPlanningID Result;
	Result.ID = ++NextID;

	// Check for the next-to-impossible event that we wrap round to 0, because 0 is the invalid ID.
	if (Result.ID == 0)
	{
		Result.ID = ++NextID;
	}

	return Result;
}

UAITask_MakeHTNPlan::UAITask_MakeHTNPlan(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	OwnerComponent(nullptr),
	TopLevelHTN(nullptr),
	PlanningType(EHTNPlanningType::Normal),
	NextPriorityMarker(1),
	CurrentPlanStepID(FHTNPlanStepID::None),
	NextNodesIndex(0),
	bIsWaitingForNodeToMakePlanExpansions(false),
	bWasCancelled(false)
{
	bIsPausable = false;
}

void UAITask_MakeHTNPlan::SetUp(UHTNPlanInstance& PlanInstance, TSharedRef<FHTNPlan> InitialPlan, EHTNPlanningType InPlanningType)
{
	// In case the AIController is now possessing a different pawn than 
	// the one we had the last time this task was used for planning.
	TasksComponent = TaskOwner->GetGameplayTasksComponent(*this);

	// If we're Finished, that means the task is being reused from a pool, so we need to reset its state.
	// We don't do that when returning it to the pool because on that frame there may be multiple invocations of EndTask 
	// which calls OnDestroy if the state is not Finished. To prevent multiple calls to OnDestroy, we only reset 
	// the state when taking the task out of the pool.
	check(TaskState == EGameplayTaskState::AwaitingActivation || TaskState == EGameplayTaskState::Finished);
	TaskState = EGameplayTaskState::AwaitingActivation;

	PlanningID = FHTNPlanningID::GenerateNewID();
	PlanInstanceID = PlanInstance.GetID();
	OwnerComponent = PlanInstance.GetOwnerComponent();
	TopLevelHTN = InitialPlan->Levels.Num() > 0 ? InitialPlan->Levels[0]->HTNAsset.Get() : nullptr;
	StartingPlan = InitialPlan;
	PlanningType = InPlanningType;

	if (PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan)
	{
		CurrentlyExecutingPlanToAdjust = PlanInstance.GetCurrentPlan();

		// Note that this assumes that we have complete ownership of the InitialPlan and it won't be used by anything else.
		if (CurrentlyExecutingPlanToAdjust.IsValid())
		{
			StartingPlan->NumRemainingPotentialDivergencePointsDuringPlanAdjustment =
				CurrentlyExecutingPlanToAdjust->NumPotentialDivergencePointsDuringPlanAdjustment;
		}
	}
	
	check(IsValid(OwnerComponent));
	check(IsValid(TopLevelHTN));

	REDIRECT_TO_VLOG(OwnerComponent);
}

void UAITask_MakeHTNPlan::ExternalCancel()
{
	bWasCancelled = true;
	Super::ExternalCancel();
}

FString UAITask_MakeHTNPlan::GetLogPrefix() const
{
	return FString::Printf(TEXT("%s (planning ID %d)"), *GetName(), PlanningID.ID);
}

FString UAITask_MakeHTNPlan::GetCurrentStateDescription() const
{
	if (bIsWaitingForNodeToMakePlanExpansions)
	{
		return FString::Printf(TEXT("Waiting for node: %s"), 
			CurrentPlanningContext.AddingNode.IsValid() ?  *CurrentPlanningContext.AddingNode->GetShortDescription() : 
			TEXT("[missing node]"));
	}

	return {};
}

UHTNPlanInstance* UAITask_MakeHTNPlan::GetPlanInstance() const
{
	return OwnerComponent->FindPlanInstance(PlanInstanceID);
}

void UAITask_MakeHTNPlan::Clear()
{
	ClearIntermediateState();
	StartingPlan.Reset();
	Frontier.Reset();
	BlockedPlans.Reset();
	PriorityMarkerCounts.Reset();
	FinishedPlan = nullptr;
	NextPriorityMarker = 1;
	bWasCancelled = false;

#if HTN_DEBUG_PLANNING
	DebugInfo.Reset();
	NodePlanningFailureReason.Reset();
#endif
}

void UAITask_MakeHTNPlan::SubmitPlanStep(const UHTNTask* Task, TSharedPtr<FBlackboardWorldState> WorldState, int32 Cost, 
	const FString& Description,
	bool bIsPotentialDivergencePointDuringPlanAdjustment,
	bool bDivergesFromCurrentPlanDuringPlanAdjustment)
{
	if (!ensure(Task && Task == CurrentPlanningContext.AddingNode))
	{
		return;
	}

	// Create a new plan with a new step for this task.
	FHTNPlanStep* AddedStep = nullptr;
	FHTNPlanStepID AddedStepID;
	const TSharedRef<FHTNPlan> NewPlan = CurrentPlanningContext.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
	check(AddedStep->Node == Task);
	AddedStep->bIsPotentialDivergencePointDuringPlanAdjustment = bIsPotentialDivergencePointDuringPlanAdjustment;
	AddedStep->WorldState = WorldState;
	NewPlan->bDivergesFromCurrentPlanDuringPlanAdjustment |= bDivergesFromCurrentPlanDuringPlanAdjustment;
	
	// Calculate final step cost
	if (Cost < 0)
	{
		UE_VLOG_UELOG(this, LogHTN, Warning, TEXT("Plan step cost given by %s is %i. Negative costs aren't allowed, resetting to zero."),
			*Task->GetShortDescription(), Cost);
	}
	AddedStep->Cost = FMath::Max(0, Cost);
	ModifyStepCost(*AddedStep, Task->Decorators);

	// Propagate step cost to level and plan
	FHTNPlanLevel& LevelInNewPlan = *NewPlan->Levels[AddedStepID.LevelIndex];
	LevelInNewPlan.Cost += AddedStep->Cost;
	NewPlan->Cost += AddedStep->Cost;

	CurrentPlanningContext.SubmitCandidatePlan(NewPlan, Description, /*bOnlyUseIfPreviousFails=*/Task->bProcessSubmittedPlanStepsInOrder);
}

void UAITask_MakeHTNPlan::WaitForLatentMakePlanExpansions(const UHTNStandaloneNode* Node)
{
	if (ensure(Node && Node == CurrentPlanningContext.AddingNode))
	{
		bIsWaitingForNodeToMakePlanExpansions = true;
	}
}

void UAITask_MakeHTNPlan::FinishLatentMakePlanExpansions(const UHTNStandaloneNode* Node)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_Planning);

	if (!ensure(IsValid(Node)))
	{
		UE_VLOG(this, LogHTN, Log, 
			TEXT("%s: FinishLatentMakePlanExpansions called by an invalid node, ignoring."), 
			*GetLogPrefix());
		return;
	}

	if (WasCancelled())
	{
		UE_VLOG(this, LogHTN, Log, 
			TEXT("%s: FinishLatentMakePlanExpansions called when the planning was already cancelled, ignoring."), 
			*GetLogPrefix());
		return;
	}

	if (!ensure(Node == CurrentPlanningContext.AddingNode))
	{
		UE_VLOG_UELOG(this, LogHTN, Error, 
			TEXT("%s: FinishLatentMakePlanExpansions called by node %s even though the currently planning node is %s"), 
			*GetLogPrefix(), 
			*Node->GetShortDescription(),
			CurrentPlanningContext.AddingNode.IsValid() ? *CurrentPlanningContext.AddingNode->GetShortDescription() : TEXT("None"));
		return;
	}

	if (!ensure(bIsWaitingForNodeToMakePlanExpansions))
	{
		UE_VLOG_UELOG(this, LogHTN, Error, 
			TEXT("%s: FinishLatentMakePlanExpansions called with task %s even though the planner is not waiting for latent MakePlanExpansions. Did you not call WaitForLatentMakePlanExpansions or called FinishLatentMakePlanExpansions twice?"), 
			*GetLogPrefix(), *Node->GetShortDescription());
		return;
	}

	UE_VLOG(this, LogHTN, Log, TEXT("%s: FinishLatentMakePlanExpansions (node %s)"), *GetLogPrefix(), *Node->GetShortDescription());

	bIsWaitingForNodeToMakePlanExpansions = false;
	OnNodeFinishedMakingPlanExpansions(Node);

	// Move to the next possible node
	NextNodesIndex += 1;
	DoPlanning();
}

void UAITask_MakeHTNPlan::WaitForLatentCreatePlanSteps(const UHTNTask* Task)
{
	WaitForLatentMakePlanExpansions(Task);
}

void UAITask_MakeHTNPlan::FinishLatentCreatePlanSteps(const UHTNTask* Task)
{
	FinishLatentMakePlanExpansions(Task);
}

void UAITask_MakeHTNPlan::Activate()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_Planning);
	
	check(IsValid(OwnerComponent));
	check(IsValid(TopLevelHTN));

	const TSharedPtr<FHTNPlan> CachedStartingPlan = StartingPlan;
	Clear();
	Frontier.HeapPush(CachedStartingPlan, FCompareHTNPlanCosts());
	
	DoPlanning();
}

void UAITask_MakeHTNPlan::OnDestroy(bool bInOwnerFinished)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_Planning);

	if (ensureMsgf(IsValid(this), TEXT("OnDestroy called on invalid gameplay task")))
	{
		const FString OwnerName = TaskOwner.IsValid() ? TaskOwner.GetObject()->GetName() : TEXT("Invalid GameplayTask Owner");
		ensureMsgf(TaskState != EGameplayTaskState::Finished, TEXT("%s OnDestroy called, current state: %i, owner name: %s"), 
			*GetLogPrefix(), TaskState, *OwnerName);
	}
	
	ClearIntermediateState();
	StartingPlan.Reset();
	CurrentlyExecutingPlanToAdjust.Reset();
	Frontier.Reset();
	BlockedPlans.Reset();

#if HTN_DEBUG_PLANNING
	if (FoundPlan())
	{
		DebugInfo.MarkAsFinishedPlan(FinishedPlan);
	}
	
	UE_VLOG(this, LogHTN, Log,
		TEXT("Planning task %s %s%s. Recorded planspace traversal:\n(Note that results may be misleading if the visual logger wasn't recording for the entire duration of planning)\n%s"),
		*GetLogPrefix(),
		PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan ? TEXT("(attempt to adjust current plan) ") : TEXT(""),
		WasCancelled() ? TEXT("was cancelled") : FoundPlan() ? TEXT("succeeded") : TEXT("failed"),
		*DebugInfo.ToString());

	DebugInfo.Reset();
#endif

	UE_LOG(LogHTN, Verbose, TEXT("Planning task %s of %s %s%s"),
		*GetLogPrefix(),
		*GetNameSafe(GetOwnerActor()),
		PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan ? TEXT("(attempt to adjust current plan) ") : TEXT(""),
		WasCancelled() ? TEXT("was cancelled") : FoundPlan() ? TEXT("succeeded") : TEXT("failed"));

	OnPlanningFinished.Broadcast(*this, FinishedPlan);

	// Instead of calling Super::OnDestroy(bInOwnerFinished); we do what it does but without marking the task as garbage 
	// since AITask_MakeHTNPlan are now reused by the plan instance to reduce the load on Garbage Collection.
	TaskState = EGameplayTaskState::Finished;
	if (UGameplayTasksComponent* const TasksPtr = TasksComponent.Get())
	{
		TasksPtr->OnGameplayTaskDeactivated(*this);
	}

	Clear();
	OnPlanningFinished.Clear();
	PlanningID = {};

	if (bInOwnerFinished)
	{
		MarkAsGarbage();
	}
	else if (ensure(IsValid(OwnerComponent)))
	{
		OwnerComponent->ReturnPlanningTaskToPool(*this);
	}
}

void UAITask_MakeHTNPlan::DoPlanning()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_Planning);
	
	check(!FinishedPlan.IsValid());

	while (!bIsWaitingForNodeToMakePlanExpansions)
	{
		if (!CurrentPlanToExpand.IsValid())
		{
			CurrentPlanToExpand = DequeueCurrentBestPlan();
			if (!CurrentPlanToExpand.IsValid())
			{
				// Planning failed
				EndTask();
				return;
			}
			
			if (CurrentPlanToExpand->IsComplete())
			{
				// When we're trying to adjust the currently executing plan, 
				// and we reach a plan that is exactly the same as the current plan, 
				// that means the most optimal plan is the same one we're in right now.
				// In this case we fail the planning task and let the currently executing plan continue executing.
				if (PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan && 
					!CurrentPlanToExpand->bDivergesFromCurrentPlanDuringPlanAdjustment)
				{
					UE_VLOG(this, LogHTN, Log,
						TEXT("Planning task %s could not adjust the current plan because the current plan is still the best possible plan."),
						*GetLogPrefix());

					CurrentPlanToExpand.Reset();
					EndTask();
					return;
				}

				// Planning succeeded
				FinishedPlan = CurrentPlanToExpand;
				EndTask();
				return;
			}
			
			if (PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan && 
				!CurrentPlanToExpand->bDivergesFromCurrentPlanDuringPlanAdjustment && 
				CurrentPlanToExpand->NumRemainingPotentialDivergencePointsDuringPlanAdjustment <= 0)
			{
				// This is true (e.g.) when the CurrentPlanToExpand is in the top branch of a Prefer that wants to switch to a lower branch during plan adjustment.
				// In that case we should continue planning the top branch in case it fails and we fall back to the diverging branch below.
				const bool bCurrentPlanToExpandBlocksAnyPotentiallyDivergingPlans = Algo::AnyOf(BlockedPlans, [&](const TSharedPtr<FHTNPlan>& BlockedPlan)
				{
					return BlockedPlan->bDivergesFromCurrentPlanDuringPlanAdjustment;
					// We don't bother checking that the BlockedPlan is actually blocked *by* the CurrentPlanToExpand for a slight improvement in performance
					// and because that is implicitly guaranteed by the fact that the CurrentPlanToExpand is the best plan in the frontier.
					// In other words we know that expanding the CurrentPlanToExpand will lead to unblocking the BlockedPlans.
					/*&& Algo::AnyOf(BlockedPlan->PriorityMarkers, [&](FHTNPriorityMarker Marker)
					{ 
						return Marker < 0 && CurrentPlanToExpand->PriorityMarkers.Contains(-Marker); 
					});*/
				});

				if (!bCurrentPlanToExpandBlocksAnyPotentiallyDivergingPlans)
				{
					UE_VLOG(this, LogHTN, Log,
						TEXT("Planning task %s could not adjust the current plan because there are no more steps in the current plan that were marked as potential adjustment points."),
						*GetLogPrefix());
					// Planning failed
					CurrentPlanToExpand.Reset();
					EndTask();
					return;
				}
			}
		}

		MakeExpansionsOfCurrentPlan();
	}
}

TSharedPtr<FHTNPlan> UAITask_MakeHTNPlan::DequeueCurrentBestPlan()
{
	AddUnblockedPlansToFrontier();
	if (Frontier.Num())
	{
		TSharedPtr<FHTNPlan> Plan;
		Frontier.HeapPop(Plan, FCompareHTNPlanCosts());
		check(Plan.IsValid());

		RemoveBlockingPriorityMarkersOf(*Plan);

		if (GetTotalNumSteps(*Plan) > OwnerComponent->MaxPlanLength)
		{
			UE_VLOG_UELOG(this, LogHTN, Error, 
				TEXT("Max plan length exceeded (%i steps), planning failed. Check if you have infinite recursion in an HTN, or increase the MaxPlanLength variable on the HTNComponent"), 
				OwnerComponent->MaxPlanLength);
			return nullptr;
		}

		return Plan;
	}

	return nullptr;
}

void UAITask_MakeHTNPlan::MakeExpansionsOfCurrentPlan()
{
	check(CurrentPlanToExpand.IsValid());

	if (CurrentPlanStepID == FHTNPlanStepID::None)
	{
		const bool bSuccess = CurrentPlanToExpand->FindStepToAddAfter(CurrentPlanStepID);
		check(bSuccess);
		check(CurrentPlanStepID != FHTNPlanStepID::None);
		check(NextNodesIndex == 0);

		CurrentPlanToExpand->GetWorldStateAndNextNodes(CurrentPlanStepID, CurrentWorldState, NextNodes);
	}

	// Go through the nodes that are connected by outgoing arrows from the current node.
	check(CurrentPlanToExpand->HasLevel(CurrentPlanStepID.LevelIndex));
	check(CurrentWorldState.IsValid());
	check(NextNodes.IsValidIndex(NextNodesIndex) || NextNodesIndex == NextNodes.Num());
	for (; NextNodesIndex < NextNodes.Num(); ++NextNodesIndex)
	{
		UHTNStandaloneNode* const Node = NextNodes[NextNodesIndex];
		if (!ensureAsRuntimeWarning(IsValid(Node)))
		{
			SAVE_PLANNING_STEP_FAILURE(Node, TEXT("Invalid node"));
			continue;
		}

		// If we're adjusting an existing plan and there are multiple NextNodes, 
		// we should only consider the same one that was picked in the currently executing plan.
		if (const FHTNPlanStep* ExistingStepToAdjust = GetExistingPlanStepToAdjust())
		{
			if (ExistingStepToAdjust->Node != Node)
			{
				UE_CVLOG_UELOG(!NextNodes.Contains(ExistingStepToAdjust->Node), this, LogHTN, Warning,
					TEXT("The node in the existing plan step to adjust (%s) is not one of the NextNodes we expect here: [%s]. This is likely the result of some node not setting bDivergesFromCurrentPlanDuringPlanAdjustment during plan adjustment."), 
						ExistingStepToAdjust->Node.IsValid() ? *ExistingStepToAdjust->Node->GetShortDescription() : TEXT("None"),
						*FString::JoinBy(NextNodes, TEXT(", "), &UHTNNode::GetShortDescription));
				SAVE_PLANNING_STEP_FAILURE(Node, TEXT("Node was not in plan to adjust"));
				continue;
			}
		}

		// We skip nodes that are already present in the plan too many times 
		// (that can happen wiht recursive or looping HTNs).
		if (Node->MaxRecursionLimit > 0 && CurrentPlanToExpand->GetRecursionCount(Node) >= Node->MaxRecursionLimit)
		{
			SAVE_PLANNING_STEP_FAILURE(Node, FString::Printf(TEXT("Node's MaxRecursionLimit (%i) already reached"), Node->MaxRecursionLimit));
			continue;
		}
		
		MakeExpansionsOfCurrentPlan(CurrentWorldState, Node);
		if (bIsWaitingForNodeToMakePlanExpansions || FinishedPlan.IsValid())
		{
			break;
		}
	}

	if (!bIsWaitingForNodeToMakePlanExpansions)
	{
		ClearIntermediateState();
	}
#if HTN_DEBUG_PLANNING
	else
	{
		UE_VLOG_UELOG(this, LogHTN, VeryVerbose, 
			TEXT("Planning task %s%s is waiting for node '%s' to produce plan steps.\nRecorded planspace traversal so far:\n(Note that results may be misleading if the visual logger wasn't recording for the entire duration of planning)\n%s"),
			*GetLogPrefix(),
			PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan ? TEXT(" (attempt to adjust current plan)") : TEXT(""),
			CurrentPlanningContext.AddingNode.IsValid() ? *CurrentPlanningContext.AddingNode->GetShortDescription() : TEXT("[missing node]"),
			*DebugInfo.ToString());
	}
#endif
}

void UAITask_MakeHTNPlan::MakeExpansionsOfCurrentPlan(const TSharedPtr<FBlackboardWorldState>& WorldState, UHTNStandaloneNode* Node)
{
	check(CurrentPlanToExpand.IsValid());
	check(Node);
	check(OwnerComponent);
	// Initialize the node with asset if hasn't been initialized with an asset already.
	// This is to make sure that blackboard key selectors are resolved etc before planning reaches the node.
	Node->InitializeFromAsset(*TopLevelHTN);
	// For the same reason, initialize the decorators on the root node if we're just starting the level.
	if (CurrentPlanStepID.StepIndex == INDEX_NONE)
	{
		const FHTNPlanLevel& Level = *CurrentPlanToExpand->Levels[CurrentPlanStepID.LevelIndex];
		Level.InitializeFromAsset(*TopLevelHTN);
	}

	// Set up the worldstate for the planning step
	WorldStateAfterEnteredDecorators = WorldState->MakeNext();
	check(OwnerComponent);
	OwnerComponent->SetPlanningWorldState(WorldStateAfterEnteredDecorators);

	// Plan-enter the decorators on the node (and the root node decorators if needed).
	SET_NODE_FAILURE_REASON(TEXT(""));
	bool bDecoratorsPassed = false;
	if (!EnterDecorators(bDecoratorsPassed, *CurrentPlanToExpand, CurrentPlanStepID, *Node))
	{
		SAVE_PLANNING_STEP_FAILURE(Node, NodePlanningFailureReason);
		return;
	}

	// Let the node itself plan.
	CurrentPlanningContext = FHTNPlanningContext(this, Node,
		CurrentPlanToExpand, CurrentPlanStepID,
		WorldStateAfterEnteredDecorators, bDecoratorsPassed);
	check(!bIsWaitingForNodeToMakePlanExpansions);
	Node->MakePlanExpansions(CurrentPlanningContext);
	if (!bIsWaitingForNodeToMakePlanExpansions)
	{
		OnNodeFinishedMakingPlanExpansions(Node);
	}
}

const FHTNPlanStep* UAITask_MakeHTNPlan::GetExistingPlanStepToAdjust() const
{
	// If the candidate plan we're expanding is already an adjustment of the current plan
	// (e.g., because we already took a different branch in some earlier Prefer node),
	// then there is no matching step in the currently executing plan to adjust: 
	// we're planning through uncharted territory now.
	if (ensure(CurrentPlanToExpand.IsValid()) && !CurrentPlanToExpand->bDivergesFromCurrentPlanDuringPlanAdjustment)
	{
		if (PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan && CurrentlyExecutingPlanToAdjust.IsValid())
		{
			return CurrentlyExecutingPlanToAdjust->FindStep(CurrentPlanStepID.MakeNext());
		}
	}

	return nullptr;
}

void UAITask_MakeHTNPlan::OnNodeFinishedMakingPlanExpansions(const UHTNStandaloneNode* Node)
{
	if (ensure(Node && Node == CurrentPlanningContext.AddingNode))
	{
		if (CurrentPlanningContext.NumAddedCandidatePlans <= 0)
		{
			SAVE_PLANNING_STEP_FAILURE(Node, NodePlanningFailureReason.IsEmpty() ? TEXT("Failed to plan") : NodePlanningFailureReason);
		}

		CurrentPlanningContext = {};
	}
}

bool UAITask_MakeHTNPlan::EnterDecorators(bool& bOutDecoratorsPassed, const FHTNPlan& Plan, const FHTNPlanStepID& StepID, const UHTNStandaloneNode& Node) const
{
	SET_NODE_FAILURE_REASON(TEXT(""));

	// If starting a plan level, enter root decorators of this level.
	if (StepID.StepIndex == INDEX_NONE)
	{
		const FHTNPlanLevel& Level = *Plan.Levels[StepID.LevelIndex];
		// Node.bAllowFailingDecoratorsOnNodeDuringPlanning doesn't influence root decorators because they aren't on the Node itself, but on the root node of the HTN.
		if (!EnterDecorators(bOutDecoratorsPassed, Level.GetRootDecoratorTemplates(), Plan, StepID, /*bMustPass=*/true))
		{
			return false;
		}
	}

	// Enter decorators of the node itself
	if (!EnterDecorators(bOutDecoratorsPassed, Node.Decorators, Plan, StepID, /*bMustPass=*/!Node.bAllowFailingDecoratorsOnNodeDuringPlanning))
	{
		return false;
	}

	return true;
}

bool UAITask_MakeHTNPlan::EnterDecorators(bool& bOutDecoratorsPassed, TArrayView<UHTNDecorator* const> Decorators, const FHTNPlan& Plan, const FHTNPlanStepID& StepID, bool bMustPass) const
{
	EHTNDecoratorTestResult TestResult = EHTNDecoratorTestResult::NotTested;
	for (UHTNDecorator* const Decorator : Decorators)
	{
		if (ensure(Decorator))
		{
			const EHTNDecoratorTestResult DecoratorTestResult = Decorator->WrappedTestCondition(*OwnerComponent, nullptr, EHTNDecoratorConditionCheckType::PlanEnter);
			TestResult = CombineHTNDecoratorTestResults(TestResult, DecoratorTestResult);
			if (bMustPass && !HTNDecoratorTestResultToBool(TestResult))
			{
				bOutDecoratorsPassed = false;
				SET_NODE_FAILURE_REASON(FString::Printf(TEXT("PlanEnter condition of decorator %s failed"), *Decorator->GetShortDescription()));
				return false;
			}

			Decorator->WrappedEnterPlan(*OwnerComponent, Plan, StepID);
		}
	}

	bOutDecoratorsPassed = HTNDecoratorTestResultToBool(TestResult);
	return true;
}

// Exits the decorators ending on the specified step.
// If the added task is the last one in a sublevel, assigns its worldstate to the compound step containing that sublevel. Recursively.
bool UAITask_MakeHTNPlan::ExitDecoratorsAndPropagateWorldState(FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{	
	const FHTNPlanLevel& Level = *Plan.Levels[StepID.LevelIndex];
	const FHTNPlanStep& Step = Level.Steps[StepID.StepIndex];

	const TSharedPtr<FBlackboardWorldState> WorldState = Step.WorldState;
	check(WorldState.IsValid());
	FGuardWorldStateProxy GuardProxy(*OwnerComponent->GetPlanningWorldStateProxy(), WorldState);
	
	SET_NODE_FAILURE_REASON(TEXT(""));

	if (!ExitDecorators(Step.Node->Decorators, Plan, StepID, !Step.bIsIfNodeFalseBranch))
	{
		return false;
	}

	if (Plan.IsLevelComplete(StepID.LevelIndex))
	{
		// Exit root decorators of level
		if (!ExitDecorators(Level.GetRootDecoratorTemplates(), Plan, { StepID.LevelIndex, -1 }))
		{
			return false;
		}
		
		// Exit decorators of the compound task containing the level of the added task
		if (Level.ParentStepID != FHTNPlanStepID::None)
		{
			// Duplicate the parent level in this plan, since we'll be making changes to it.
			TSharedPtr<FHTNPlanLevel>& ParentLevel = Plan.Levels[Level.ParentStepID.LevelIndex];
			ParentLevel = MakeShared<FHTNPlanLevel>(*ParentLevel);
			
			FHTNPlanStep& ParentStep = ParentLevel->Steps[Level.ParentStepID.StepIndex];
			check(!ParentStep.WorldState.IsValid());

			const bool bIsParentStepFinished = ParentStep.Node->OnSubLevelFinishedPlanning(Plan, Level.ParentStepID, StepID.LevelIndex, WorldState);
			ParentStep.Cost += Level.Cost;
			ParentLevel->Cost += Level.Cost;
			if (bIsParentStepFinished)
			{
				ParentStep.WorldState = WorldState;
				
				// Allow decorators on the parent node to modify cost of the sublevels.
				const int32 OldCost = ParentStep.Cost;
				ModifyStepCost(ParentStep, ParentStep.Node->Decorators);
				const int32 CostChange = ParentStep.Cost - OldCost;
				if (CostChange < 0)
				{
					UE_VLOG_UELOG(this, LogHTN, Error, TEXT("When modifying the cost of node %s with a decorator, cost was decreased. This is only allowed for primitive tasks. Otherwise the planner cannot guarantee finding the lowest-cost plan."),
						*ParentStep.Node->GetShortDescription());
					ParentStep.Cost = OldCost;
				}
				else
				{
					ParentLevel->Cost += CostChange;
					Plan.Cost += CostChange;
				}
					
				return ExitDecoratorsAndPropagateWorldState(Plan, Level.ParentStepID);
			}
		}
	}

	return true;
}

bool UAITask_MakeHTNPlan::ExitDecorators(TArrayView<UHTNDecorator* const> Decorators, const FHTNPlan& Plan, const FHTNPlanStepID& StepID, bool bShouldPass) const
{
	EHTNDecoratorTestResult TestResult = EHTNDecoratorTestResult::NotTested;
	for (int32 I = Decorators.Num() - 1; I >= 0; --I)
	{
		UHTNDecorator* const Decorator = Decorators[I];
		if (ensure(Decorator))
		{
			const EHTNDecoratorTestResult DecoratorTestResult = Decorator->WrappedTestCondition(*OwnerComponent, nullptr, EHTNDecoratorConditionCheckType::PlanExit);
			TestResult = CombineHTNDecoratorTestResults(TestResult, DecoratorTestResult, bShouldPass);
			if (!HTNDecoratorTestResultToBool(TestResult, bShouldPass))
			{
				if (bShouldPass)
				{
					SET_NODE_FAILURE_REASON(FString::Printf(
						TEXT("PlanExit condition of decorator %s failed"),
						*Decorator->GetShortDescription()));
				}
				else
				{
					SET_NODE_FAILURE_REASON(FString::Printf(
						TEXT("PlanExit condition of decorator %s passed when should've failed"),
						*Decorator->GetShortDescription()));
				}
				return false;
			}

			Decorator->WrappedExitPlan(*OwnerComponent, Plan, StepID);
		}
	}

	return true;
}

void UAITask_MakeHTNPlan::ModifyStepCost(FHTNPlanStep& Step, const TArray<UHTNDecorator*>& Decorators) const
{
	FGuardWorldStateProxy GuardProxy(*OwnerComponent->GetPlanningWorldStateProxy(), Step.WorldState);
	for (int32 I = Decorators.Num() - 1; I >= 0; --I)
	{
		UHTNDecorator* const Decorator = Decorators[I];
		if (ensure(Decorator))
		{
			Decorator->WrappedModifyStepCost(*OwnerComponent, Step);
		}
	}
}

void UAITask_MakeHTNPlan::SubmitCandidatePlan(const TSharedRef<FHTNPlan>& NewPlan, UHTNStandaloneNode* AddedNode, const FString& AddedStepDescription)
{
	if (WasCancelled())
	{
		return;
	}

	AddBlockingPriorityMarkersOf(*NewPlan);
	if (!IsBlockedByPriorityMarkers(*NewPlan))
	{
		Frontier.HeapPush(NewPlan, FCompareHTNPlanCosts());
	}
	else
	{
		BlockedPlans.Add(NewPlan);
	}

	SAVE_PLANNING_STEP_SUCCESS(AddedNode, NewPlan, AddedStepDescription);
}

void UAITask_MakeHTNPlan::ClearIntermediateState()
{
	CurrentPlanToExpand = nullptr;
	CurrentPlanStepID = FHTNPlanStepID::None;
	CurrentWorldState = nullptr;
	NextNodes.Empty();
	NextNodesIndex = 0;
	WorldStateAfterEnteredDecorators = nullptr;
	CurrentPlanningContext = {};
	bIsWaitingForNodeToMakePlanExpansions = false;
}

void UAITask_MakeHTNPlan::AddBlockingPriorityMarkersOf(const FHTNPlan& Plan)
{
	for (const FHTNPriorityMarker PriorityMarker : Plan.PriorityMarkers)
	{
		if (PriorityMarker > 0)
		{
			PriorityMarkerCounts.FindOrAdd(PriorityMarker) += 1;
		}
	}
}

void UAITask_MakeHTNPlan::RemoveBlockingPriorityMarkersOf(const FHTNPlan& Plan)
{
	for (const FHTNPriorityMarker PriorityMarker : Plan.PriorityMarkers)
	{
		if (PriorityMarker > 0)
		{
			PriorityMarkerCounts[PriorityMarker] -= 1;
		}
	}
}

bool UAITask_MakeHTNPlan::IsBlockedByPriorityMarkers(const FHTNPlan& Plan) const
{
	const auto IsBlocked = [&](FHTNPriorityMarker Marker) -> bool { return Marker < 0 && PriorityMarkerCounts.FindRef(-Marker) > 0; };
	return Algo::AnyOf(Plan.PriorityMarkers, IsBlocked);
}

void UAITask_MakeHTNPlan::AddUnblockedPlansToFrontier()
{
	bool bRemovedAny = false;
	for (auto It = PriorityMarkerCounts.CreateIterator(); It; ++It)
	{
		check(It->Value >= 0);
		if (It->Value == 0)
		{
			bRemovedAny = true;
			It.RemoveCurrent();
		}
	}
	
	if (bRemovedAny)
	{
		const int32 Index = Algo::Partition(BlockedPlans.GetData(), BlockedPlans.Num(), [this](const TSharedPtr<FHTNPlan>& Plan) 
		{ 
			return IsBlockedByPriorityMarkers(*Plan); 
		});
		for (int32 I = Index; I < BlockedPlans.Num(); ++I)
		{
			Frontier.HeapPush(BlockedPlans[I], FCompareHTNPlanCosts());
		}
		BlockedPlans.SetNum(Index);
	}
}

#undef SAVE_PLANNING_STEP_SUCCESS
#undef SAVE_PLANNING_STEP_FAILURE
#undef SET_NODE_FAILURE_REASON
