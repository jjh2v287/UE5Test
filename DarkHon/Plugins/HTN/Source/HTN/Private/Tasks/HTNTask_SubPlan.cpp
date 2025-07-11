// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_SubPlan.h"

#include "Misc/RuntimeErrors.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNTask_SubPlan::UHTNTask_SubPlan(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	bPlanDuringOuterPlanPlanning(false),
	bPlanDuringExecution(true),
	bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable(true),
	OnSubPlanSucceeded(EHTNPlanInstanceFinishReaction::Succeed),
	OnSubPlanFailed(EHTNPlanInstanceFinishReaction::Fail),
	OnThisNodeAborted(EHTNSubPlanNodeAbortedReaction::AbortSubPlanExecution)
{
	bShowTaskNameOnCurrentPlanVisualization = false;
	bPlanNextNodesAfterThis = false;

	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

FString UHTNTask_SubPlan::GetStaticDescription() const
{
	TStringBuilder<2048> SB;
	SB << Super::GetStaticDescription() << TEXT(":");
	
	if (bPlanDuringOuterPlanPlanning || bPlanDuringExecution)
	{
		SB << TEXT("\nPlan During Outer Plan Planning: ") << (bPlanDuringOuterPlanPlanning ? TEXT("True") : TEXT("False"));
		SB << TEXT("\nPlan During Execution: ");
		if (bPlanDuringExecution)
		{
			if (bPlanDuringOuterPlanPlanning && bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable)
			{
				SB << TEXT("Only if looped");
			}
			else
			{
				SB << TEXT("True");
			}
		}
		else
		{
			SB << TEXT("False");
		}

	}
	else
	{
		SB << TEXT("\nWARNING: neither 'Plan During Outer Plan Planning' nor 'Plan During Execution' are enabled.\nPlease enable at least one of them.");
	}

	SB << TEXT("\n");
	SB << TEXT("\nOn Sub Plan Succeeded: ") << *UEnum::GetDisplayValueAsText(OnSubPlanSucceeded).ToString();
	SB << TEXT("\nOn Sub Plan Failed: ") << *UEnum::GetDisplayValueAsText(OnSubPlanFailed).ToString();
	SB << TEXT("\nOn This Node Aborted: ") << *UEnum::GetDisplayValueAsText(OnThisNodeAborted).ToString();

	return SB.ToString();
}

#if WITH_EDITOR
void UHTNTask_SubPlan::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Make sure that at least one of bPlanDuringOuterPlanPlanning and bPlanDuringExecution is true.
	if (PropertyChangedEvent.MemberProperty && !bPlanDuringOuterPlanPlanning && !bPlanDuringExecution)
	{
		if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, bPlanDuringExecution))
		{
			bPlanDuringOuterPlanPlanning = true;
		}
		else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, bPlanDuringOuterPlanPlanning))
		{
			bPlanDuringExecution = true;
		}
	}
}
#endif

void UHTNTask_SubPlan::MakePlanExpansions(FHTNPlanningContext& Context)
{
	// Fail if we are nested in too many subplans to prevent infinite loop.
	if (UHTNComponent* const HTNComponent = Context.GetHTNComponent())
	{
		if (HTNComponent->MaxNestedSubPlanDepth >= 0)
		{
			if (const UHTNPlanInstance* const ParentPlanInstance = Context.GetPlanInstance())
			{
				const int32 ParentPlanInstanceDepth = ParentPlanInstance->GetDepth();
				if (ParentPlanInstanceDepth >= HTNComponent->MaxNestedSubPlanDepth)
				{
					UE_VLOG_UELOG(HTNComponent, LogHTN, Error,
						TEXT("Max subplan depth exceeded (%i), can't create subplan at depth (%i). Check if you have SubPlan nodes that form an infinite loop."),
						HTNComponent->MaxNestedSubPlanDepth,
						ParentPlanInstanceDepth + 1);
					Context.PlanningTask->SetNodePlanningFailureReason(TEXT("Max subplan depth exceeded"));
					return;
				}
			}
		}
	}

	if (bPlanDuringOuterPlanPlanning)
	{
		// If we want to plan the subplan as part of the superplan, then act like a HTNNode_Scope.
		// The produced sublevels will be ignored during execution but extracted into a separate plan.
		FHTNPlanStep* AddedStep = nullptr;
		FHTNPlanStepID AddedStepID;
		const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);

		AddedStep->SubLevelIndex = NextNodes.Num() ? Context.AddInlineLevel(*NewPlan, AddedStepID) : INDEX_NONE;
		Context.SubmitCandidatePlan(NewPlan);
	}
	else
	{
		Context.PlanningTask->SubmitPlanStep(this, Context.WorldStateAfterEnteringDecorators->MakeNext(), 0, TEXT("bPlanDuringOuterPlanPlanning=False"));
	}
}

uint16 UHTNTask_SubPlan::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}

void UHTNTask_SubPlan::InitializeSubPlanInstance(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, TSharedPtr<FHTNPlan> SubPlan) const
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	UHTNPlanInstance* const ParentPlanInstance = OwnerComp.FindPlanInstanceByNodeMemory(NodeMemory);
	if (!ensure(ParentPlanInstance))
	{
		return;
	}

	// Create a UHTNPlanInstance
	FHTNPlanInstanceConfig Config;
	Config.SucceededReaction = OnSubPlanSucceeded;
	Config.FailedReaction = OnSubPlanFailed;
	// If we planned together with the superplan, a subplan was extracted from the main plan.
	// In that case the new plan instance will execute this subplan.
	// This may be null, in which case the plan instance will need to plan first.
	// We cache the plan in case we'll need to make the plan instance execute it multiple times
	// (e.g., if bPlanDuringExecution is false).
	Config.PrePlannedPlan = SubPlan;
	Config.bPlanDuringExecution = bPlanDuringExecution;
	Config.bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable = bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable;

	Config.Depth = ParentPlanInstance->GetDepth() + 1;
	Config.RootNodeOverride = const_cast<ThisClass*>(this);
	Config.OuterPlanRecursionCounts = Plan.RecursionCounts;
	Config.CanPlanInstanceLoopDelegate.BindUObject(this, &ThisClass::CanPlanInstanceLoop, NodeMemory);

	UHTNPlanInstance& PlanInstance = OwnerComp.AddSubPlanInstance(Config);
	Memory.PlanInstanceID = PlanInstance.GetID();
	PlanInstance.PlanInstanceFinishedEvent.AddUObject(this, &ThisClass::OnSubPlanInstanceFinished, NodeMemory);

	ensure(PlanInstance.GetStatus() == EHTNPlanInstanceStatus::NotStarted);
}

void UHTNTask_SubPlan::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{	
	FNodeMemory& Memory = *(new(NodeMemory) FNodeMemory);
}

void UHTNTask_SubPlan::CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	// Unsubscribe from the event in case it's triggered as part of RemoveSubPlanInstance
	// This can happen if the plan instance is running a planning task and removing the plan instance cancels it,
	// triggering the plan instance to stop, firing the PlanInstanceFinishedEvent event.
	if (UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstance(Memory.PlanInstanceID))
	{
		PlanInstance->PlanInstanceFinishedEvent.RemoveAll(this);
	}

	ensure(OwnerComp.RemoveSubPlanInstance(Memory.PlanInstanceID));
}

bool UHTNTask_SubPlan::RecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FBlackboardWorldState& WorldState, const FHTNPlanStep& SubmittedPlanStep)
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstance(Memory.PlanInstanceID);
	if (ensure(PlanInstance))
	{
		return PlanInstance->RecheckCurrentPlan(&WorldState);
	}

	return false;
}

EHTNNodeResult UHTNTask_SubPlan::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);

	// In case we're in a looping branch of a Parallel node, meaning there are multiple calls to ExecuteTask.
	// We reset the value from the previous execution back to InProgress.
	Memory.TaskResult = EHTNNodeResult::InProgress;

	UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstance(Memory.PlanInstanceID);
	if (!ensure(PlanInstance))
	{
		return EHTNNodeResult::Failed;
	}

	StartPlanInstance(*PlanInstance, Memory);
	return Memory.TaskResult;
}

EHTNNodeResult UHTNTask_SubPlan::AbortTask(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	Memory.bAbortTaskCalled = true;
	Memory.bCanPlanLoop = false;
	if (UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstance(Memory.PlanInstanceID))
	{
		switch (OnThisNodeAborted)
		{
		case EHTNSubPlanNodeAbortedReaction::AbortSubPlanExecution:
		{
			if (Memory.bDeferredStartPlanInstance)
			{
				ensureAlways(PlanInstance->GetStatus() != EHTNPlanInstanceStatus::InProgress);
				return EHTNNodeResult::Aborted;
			}

			UE_VLOG(&OwnerComp, LogHTN, Verbose, TEXT("%s aborting sub plan due to being aborted."),
				*GetShortDescription());
			FGuardValue_Bitfield(Memory.bCanObserverPerformFinishLatentTask, false);
			PlanInstance->Stop();
			return Memory.TaskResult;
		}
		case EHTNSubPlanNodeAbortedReaction::WaitForSubPlanToFinish:
			UE_VLOG(&OwnerComp, LogHTN, Verbose, TEXT("%s will wait for subplan instance to finish execution (or fail to make a plan)."), 
				*GetShortDescription());
			return EHTNNodeResult::InProgress;
		default:
			checkNoEntry();
		}
	}

	return EHTNNodeResult::Aborted;
}

void UHTNTask_SubPlan::TickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime)
{
	// Tick Subplan
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	Memory.FrameCounter += 1;

	if (UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstance(Memory.PlanInstanceID))
	{
		if (Memory.bDeferredStartPlanInstance)
		{
			UE_VLOG(&OwnerComp, LogHTN, Verbose, TEXT("%s starting subplan whose start was deferred from last frame."),
				*GetShortDescription());
			StartPlanInstance(*PlanInstance, Memory);
			if (Memory.TaskResult != EHTNNodeResult::InProgress)
			{
				FinishLatentTask(OwnerComp, Memory.TaskResult, NodeMemory);
			}
		}

		if (!Memory.bDeferredStartPlanInstance && Memory.TaskResult == EHTNNodeResult::InProgress)
		{
			if (PlanInstance->GetStatus() == EHTNPlanInstanceStatus::InProgress)
			{
				PlanInstance->Tick(DeltaTime);
			}
			else
			{
				UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s failed because the subplan instance it created was not active during task tick"),
					*GetShortDescription());
				FinishLatentTask(OwnerComp, EHTNNodeResult::Failed, NodeMemory);
			}
		}
	}
	else
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Error, TEXT("%s failed because the subplan instance it created was missing during task tick"),
			*GetShortDescription())
		FinishLatentTask(OwnerComp, EHTNNodeResult::Failed, NodeMemory);
	}
}

void UHTNTask_SubPlan::OnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	Memory.bCanPlanLoop = false;
	if (UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstance(Memory.PlanInstanceID))
	{
		// Make sure we don't try to call FinishLatentTask upon aborting the plan instance.
		FGuardValue_Bitfield(Memory.bCanObserverPerformFinishLatentTask, false);

		// If we're being force-finished while the plan instance is still active, force-abort the instance.
		if (PlanInstance->GetStatus() == EHTNPlanInstanceStatus::InProgress)
		{
			PlanInstance->Stop(/*bDisregardLatentAbort=*/true);
		}
	}
}

UHTNTask_SubPlan::FNodeMemory::FNodeMemory() :
	TaskResult(EHTNNodeResult::InProgress),
	bAbortTaskCalled(false),
	bCanPlanLoop(true),
	bCanObserverPerformFinishLatentTask(true),
	bDeferredStartPlanInstance(false),
	FrameCounter(0)
{}

void UHTNTask_SubPlan::StartPlanInstance(UHTNPlanInstance& PlanInstance, FNodeMemory& Memory) const
{
	// This check prevents an infinite recursion if calling Start gets us here again within the same frame.
	// (e.g., if this task is in a looping parallel branch). 
	if (Memory.FrameOfLastStartOfPlanInstance.IsSet() && *Memory.FrameOfLastStartOfPlanInstance == Memory.FrameCounter)
	{
		UE_VLOG(PlanInstance.GetOwnerComponent(), LogHTN, Verbose, TEXT("%s deferring subplan start to next frame because it was already started this frame."),
			*GetShortDescription());
		Memory.bDeferredStartPlanInstance = true;
		return;
	}
	Memory.bDeferredStartPlanInstance = false;

	// Start the plan instance.
	// This may finish before we exit the function call, so we make sure FinishLatentTask isn't called from the event handler.
	// The event handler will however set Memory.TaskResult so we'll be able to return it here.
	FGuardValue_Bitfield(Memory.bCanObserverPerformFinishLatentTask, false);
	Memory.FrameOfLastStartOfPlanInstance = Memory.FrameCounter;
	PlanInstance.Start();
}

bool UHTNTask_SubPlan::CanPlanInstanceLoop(const UHTNPlanInstance* Sender, uint8* NodeMemory) const
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	return Memory.bCanPlanLoop;
}

void UHTNTask_SubPlan::OnSubPlanInstanceFinished(UHTNPlanInstance* Sender, uint8* NodeMemory) const
{
	FNodeMemory& Memory = *CastInstanceNodeMemory<FNodeMemory>(NodeMemory);
	Memory.TaskResult = [&]()
	{
		const EHTNPlanInstanceStatus PlanInstanceStatus = Sender->GetStatus();
		switch (PlanInstanceStatus)
		{
		case EHTNPlanInstanceStatus::NotStarted:
		case EHTNPlanInstanceStatus::InProgress:
			return EHTNNodeResult::InProgress;
		case EHTNPlanInstanceStatus::Succeeded:
			return Memory.bAbortTaskCalled ? EHTNNodeResult::Aborted : EHTNNodeResult::Succeeded;
		case EHTNPlanInstanceStatus::Failed:
			return Memory.bAbortTaskCalled ? EHTNNodeResult::Aborted : EHTNNodeResult::Failed;
		default:
			checkNoEntry();
			return EHTNNodeResult::Failed;
		}
	}();

	if (Memory.bCanObserverPerformFinishLatentTask && Memory.TaskResult != EHTNNodeResult::InProgress)
	{
		if (UHTNComponent* const OwnerComp = Sender->GetOwnerComponent())
		{
			FGuardValue_Bitfield(Memory.bCanObserverPerformFinishLatentTask, false);
			FinishLatentTask(*OwnerComp, Memory.TaskResult, NodeMemory);
		}
	}
}
