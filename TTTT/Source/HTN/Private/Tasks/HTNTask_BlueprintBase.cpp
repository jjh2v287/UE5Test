// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_BlueprintBase.h"

#include "HTNBlueprintLibrary.h"
#include "Utility/HTNHelpers.h"
#include "WorldStateProxy.h"

#include "BlueprintNodeHelpers.h"
#include "Misc/ScopeExit.h"
#include "VisualLogger/VisualLogger.h"

UHTNTask_BlueprintBase::UHTNTask_BlueprintBase(const FObjectInitializer& Initializer) : Super(Initializer),
	CachedNodeMemory(nullptr),
	CurrentlyExecutedFunction(EHTNTaskFunction::None),
	CurrentCallResult(EHTNNodeResult::Failed),
	bShowPropertyDetails(true),
	bIsAborting(false)
{
#define IS_IMPLEMENTED(FunctionName) \
	(BlueprintNodeHelpers::HasBlueprintFunction(GET_FUNCTION_NAME_CHECKED(ThisClass, FunctionName), *this, *StaticClass()))
	bImplementsCreatePlanSteps = IS_IMPLEMENTED(ReceiveCreatePlanSteps);
	bImplementsRecheckPlan = IS_IMPLEMENTED(ReceiveRecheckPlan);
	bImplementsExecute = IS_IMPLEMENTED(ReceiveExecute);
	bImplementsAbort = IS_IMPLEMENTED(ReceiveAbort);
	bImplementsTick = IS_IMPLEMENTED(ReceiveTick);
	bImplementsOnFinished = IS_IMPLEMENTED(ReceiveOnFinished);
	bImplementsOnPlanExecutionStarted = IS_IMPLEMENTED(ReceiveOnPlanExecutionStarted);
	bImplementsOnPlanExecutionFinished = IS_IMPLEMENTED(ReceiveOnPlanExecutionFinished);
	bImplementsLogToVisualLog = IS_IMPLEMENTED(ReceiveDescribePlanStepToVisualLog);
#undef IS_IMPLEMENTED

	bNotifyTick = bImplementsTick;
	// We need OnExecutionFinish to be called even if ReceiveExecutionFinish is not implemented in Blueprints,
	// so that we can abort latent actions (e.g. Delay nodes) and timers.
	bNotifyTaskFinished = true;
	bNotifyOnPlanExecutionStarted = bImplementsOnPlanExecutionStarted;
	// Likewise for OnPlanExecutionFinished.
	bNotifyOnPlanExecutionFinished = true;

	// All blueprint-based nodes must create instances
	bCreateNodeInstance = true;
	// Since tasks created from blueprints won't set this to true on their own.
	bOwnsGameplayTasks = true;
	
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		BlueprintNodeHelpers::CollectPropertyData(this, StaticClass(), PropertyData);
	}
}

void UHTNTask_BlueprintBase::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	FHTNHelpers::ResolveBlackboardKeySelectors(*this, *StaticClass(), Asset.BlackboardAsset);
}

FString UHTNTask_BlueprintBase::GetStaticDescription() const
{
	FString Description = Super::GetStaticDescription();

	if (bShowPropertyDetails)
	{
		if (UHTNTask_BlueprintBase* const CDO = GetClass()->GetDefaultObject<UHTNTask_BlueprintBase>())
		{
			const FString PropertyDescription = FHTNHelpers::CollectPropertyDescription(this, StaticClass(), CDO->PropertyData);
			if (PropertyDescription.Len())
			{
				Description += TEXT(":\n\n");
				Description += PropertyDescription;
			}
		}
	}

	return Description;
}

bool UHTNTask_BlueprintBase::IsTaskExecuting() const
{
	if (UHTNComponent* const OwnerComp = GetTypedOuter<UHTNComponent>())
	{
		if (const FHTNNodeInPlanInfo Info = OwnerComp->FindActiveTaskInfo(this, CachedNodeMemory))
		{
			return Info.Status == EHTNTaskStatus::Active;
		}
	}

	return false;
}

bool UHTNTask_BlueprintBase::IsTaskAborting() const
{
	return bIsAborting;
}

void UHTNTask_BlueprintBase::InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	CachedNodeMemory = NodeMemory;
}

void UHTNTask_BlueprintBase::CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	CachedNodeMemory = nullptr;
}

void UHTNTask_BlueprintBase::CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const
{
	if (!bImplementsCreatePlanSteps)
	{
		Super::CreatePlanSteps(OwnerComp, PlanningTask, WorldState);
		return;
	}

	const FGuardOwnerComponent OwnerGuard(*this, &OwnerComp);
	CurrentlyExecutedFunction = EHTNTaskFunction::CreatePlanSteps;
	OldWorldState = WorldState;
	NextWorldState = WorldState->MakeNext();
	OutPlanningTask = &PlanningTask;
	
	FGuardWorldStateProxy GuardProxy(*OwnerComp.GetPlanningWorldStateProxy(), NextWorldState);
	ON_SCOPE_EXIT
	{
		CurrentlyExecutedFunction = EHTNTaskFunction::None;
		OldWorldState.Reset();
		NextWorldState.Reset();
		OutPlanningTask = nullptr;
	};

	ReceiveCreatePlanSteps(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
}

bool UHTNTask_BlueprintBase::RecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FBlackboardWorldState& WorldState, const FHTNPlanStep& SubmittedPlanStep)
{
	if (!bImplementsRecheckPlan)
	{
		return Super::RecheckPlan(OwnerComp, NodeMemory, WorldState, SubmittedPlanStep);
	}

	TGuardValue<EHTNTaskFunction> Guard(CurrentlyExecutedFunction, EHTNTaskFunction::RecheckPlan);
	FGuardValue_Bitfield(bForceUsingPlanningWorldState, true);

	check(GetOwnerComponent() == &OwnerComp);
	check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
	check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsWorldState());
	return ReceiveRecheckPlan(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
}

EHTNNodeResult UHTNTask_BlueprintBase::ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID)
{
	bIsAborting = false;
	
	CurrentCallResult = bImplementsExecute || bImplementsTick ? EHTNNodeResult::InProgress : EHTNNodeResult::Succeeded;

	if (bImplementsExecute)
	{
		TGuardValue<EHTNTaskFunction> Guard(CurrentlyExecutedFunction, EHTNTaskFunction::Execute);

		check(GetOwnerComponent() == &OwnerComp);
		check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetBlackboardProxy());
		check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
		ReceiveExecute(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
	}
		
	return CurrentCallResult;
}

EHTNNodeResult UHTNTask_BlueprintBase::AbortTask(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	bIsAborting = true;
	BlueprintNodeHelpers::AbortLatentActions(OwnerComp, *this);
	
	CurrentCallResult = bImplementsAbort ? EHTNNodeResult::InProgress : EHTNNodeResult::Aborted;

	if (bImplementsAbort)
	{
		TGuardValue<EHTNTaskFunction> Guard(CurrentlyExecutedFunction, EHTNTaskFunction::Abort);

		check(GetOwnerComponent() == &OwnerComp);
		check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetBlackboardProxy());
		check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
		ReceiveAbort(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
	}

	return CurrentCallResult;
}

void UHTNTask_BlueprintBase::TickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (TickInterval.Tick(DeltaSeconds))
	{
		DeltaSeconds = TickInterval.GetElapsedTimeWithFallback(DeltaSeconds);

		if (bImplementsTick)
		{
			check(GetOwnerComponent() == &OwnerComp);
			check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetBlackboardProxy());
			check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
			ReceiveTick(
				OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr,
				DeltaSeconds);
		}

		TickInterval.Reset();
	}
}

void UHTNTask_BlueprintBase::OnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result)
{	
	Super::OnTaskFinished(OwnerComp, NodeMemory, Result);

	// So that we tick immediately if (somehow) enabled back.
	TickInterval.Set(0); 

	BlueprintNodeHelpers::AbortLatentActions(OwnerComp, *this);

	if (bImplementsOnFinished)
	{
		check(GetOwnerComponent() == &OwnerComp);
		check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetBlackboardProxy());
		check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
		check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
		ReceiveOnFinished(
			OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr, 
			Result);
	}
}

void UHTNTask_BlueprintBase::OnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory)
{
	if (bImplementsOnPlanExecutionStarted)
	{
		FGuardValue_Bitfield(bForceUsingPlanningWorldState, true);
		check(GetOwnerComponent() == &OwnerComp);
		check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
		check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
		check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
		ReceiveOnPlanExecutionStarted(OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
	}
}

void UHTNTask_BlueprintBase::OnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result)
{
	// Ths handles the case where we started a latent action (e.g., a delay node) in OnPlanExecutionStarted, 
	// but then the plan got aborted before the node even activated, so OnExecutionStart never got called and
	// therefore OnExecutionFinish didn't either.
	BlueprintNodeHelpers::AbortLatentActions(OwnerComp, *this);

	if (bImplementsOnPlanExecutionFinished)
	{
		FGuardValue_Bitfield(bForceUsingPlanningWorldState, true);
		check(GetOwnerComponent() == &OwnerComp);
		check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
		check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsBlackboard());
		check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
		ReceiveOnPlanExecutionFinished(
			OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr, 
			Result);
	}
}

void UHTNTask_BlueprintBase::LogToVisualLog(UHTNComponent& OwnerComp, const uint8* NodeMemory, const FHTNPlanStep& SubmittedPlanStep) const
{
#if ENABLE_VISUAL_LOG
	if (bImplementsLogToVisualLog)
	{
		FGuardValue_Bitfield(bForceUsingPlanningWorldState, true);
		
		check(GetOwnerComponent() == &OwnerComp);
		check(UHTNNodeLibrary::GetOwnersWorldState(this) == OwnerComp.GetPlanningWorldStateProxy());
		check(UHTNNodeLibrary::GetOwnersWorldState(this)->IsWorldState());
		check(!UHTNNodeLibrary::GetOwnersWorldState(this)->IsEditable());
		ReceiveDescribePlanStepToVisualLog(
			OwnerComp.GetOwner(), OwnerComp.GetAIOwner(), OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr, 
			LogHTNCurrentPlan.GetCategoryName());
	}
#endif
}

void UHTNTask_BlueprintBase::BP_Replan(const FHTNReplanParameters& Params) const
{
	if (UHTNComponent* const HTNComponent = GetTypedOuter<UHTNComponent>())
	{
		if (CachedNodeMemory)
		{
			return Replan(*HTNComponent, CachedNodeMemory, Params);
		}
		else
		{
			UE_VLOG_UELOG(HTNComponent, LogHTN, Error,
				TEXT("Task '%s' called Replan while not initialized for execution."),
				*GetShortDescription());
		}
	}
	else
	{
		UE_VLOG_UELOG(this, LogHTN, Warning,
			TEXT("Task '%s': Replan could not find the owning HTNComponent. Was it called during planning?"),
			*GetShortDescription());
	}
}

void UHTNTask_BlueprintBase::SubmitPlanStep(int32 Cost, const FString& Description) const
{
	if (ensureMsgf(CurrentlyExecutedFunction == EHTNTaskFunction::CreatePlanSteps, TEXT("SubmitPlanStep can only be called from CreatePlanSteps!")))
	{
		check(OutPlanningTask);
		OutPlanningTask->SubmitPlanStep(this, NextWorldState, Cost, Description);
		
		NextWorldState = OldWorldState->MakeNext();
		UHTNComponent* const OwnerComp = GetOwnerComponent();
		if (ensure(OwnerComp))
		{
			OwnerComp->SetPlanningWorldState(NextWorldState);
		}
	}
}

void UHTNTask_BlueprintBase::SetPlanningFailureReason(const FString& FailureReason) const
{
	if (ensureMsgf(CurrentlyExecutedFunction == EHTNTaskFunction::CreatePlanSteps, TEXT("SetPlanningFailureReason can only be called from CreatePlanSteps!")))
	{
		check(OutPlanningTask);
		OutPlanningTask->SetNodePlanningFailureReason(FailureReason);
	}
}

void UHTNTask_BlueprintBase::FinishExecute(bool bSuccess)
{	
	const EHTNNodeResult Result = bSuccess ? EHTNNodeResult::Succeeded : EHTNNodeResult::Failed;
	if (CurrentlyExecutedFunction == EHTNTaskFunction::Execute)
	{
		CurrentCallResult = Result;
	}
	else
	{
		UHTNComponent* const OwnerComp = GetTypedOuter<UHTNComponent>();
		if (ensure(OwnerComp))
		{
			if (!bIsAborting)
			{
				FinishLatentTask(*OwnerComp, Result, CachedNodeMemory);
			}
			else
			{
				UE_VLOG_UELOG(OwnerComp, LogHTN, Error, TEXT("%s: Called FinishExecute while aborting. Please call FinishAbort instead."), *GetShortDescription());
			}
		}
	}
}

void UHTNTask_BlueprintBase::FinishAbort()
{
	const EHTNNodeResult Result = EHTNNodeResult::Aborted;
	if (CurrentlyExecutedFunction == EHTNTaskFunction::Abort)
	{
		CurrentCallResult = Result;
	}
	else
	{
		UHTNComponent* const OwnerComp = GetTypedOuter<UHTNComponent>();
		if (ensure(OwnerComp))
		{
			if (bIsAborting)
			{
				FinishLatentTask(*OwnerComp, Result, CachedNodeMemory);
			}
			else
			{
				UE_VLOG_UELOG(OwnerComp, LogHTN, Error, TEXT("%s: Called FinishAbort while not aborting. Please call FinishExecute instead."), *GetShortDescription());
			}
		}
	}
}
