// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNPlanInstance.h"

#include "AITask_MakeHTNPlan.h"
#include "HTNComponent.h"
#include "HTNDelegates.h"
#include "HTNPlan.h"
#include "HTNTask.h"
#include "HTNDecorator.h"
#include "HTNService.h"
#include "Nodes/HTNNode_SubNetwork.h"
#include "Nodes/HTNNode_Parallel.h"
#include "Tasks/HTNTask_SubPlan.h"
#include "WorldStateProxy.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Algo/Count.h"
#include "Algo/Find.h"
#include "Algo/Transform.h"
#include "Misc/RuntimeErrors.h"
#include "Misc/ScopeExit.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	enum EHTNUpdateSubNodesFlags
	{
		Tick = 1 << 0,
		CheckConditionsExecution = 1 << 1,
		CheckConditionsRecheck = 1 << 2
	};

	using FHTNExtractedSubPlansMap = TMap<FHTNPlanStepID, TSharedPtr<FHTNPlan>, TInlineSetAllocator<32>>;

	// A helper for breaking down a plan into a main plan and subplans. 
	// Only does anything if there are HTNTask_SubPlan tasks in the plan that have bPlanDuringOuterPlanPlanning enabled.
	// If it extracted any subplans, returns a copy of the original plan with the extracted levels replaced with dummy levels.
	// Otherwise returns the original plan.
	struct FHTNSubPlanExtractor
	{
		FHTNSubPlanExtractor(TSharedRef<FHTNPlan> Plan, FHTNExtractedSubPlansMap& OutExtractedSubPlanMap) : 
			Plan(Plan), 
			ExtractedSubPlanMap(OutExtractedSubPlanMap)
		{
			Plan->CheckIntegrity();
		}

		TSharedRef<FHTNPlan> Extract()
		{
			Plan->CheckIntegrity();

			for (int32 LevelIndex = 1; LevelIndex < Plan->Levels.Num(); ++LevelIndex)
			{
				FHTNPlanLevel& PlanLevel = *Plan->Levels[LevelIndex];
				if (!PlanLevel.IsDummyLevel())
				{
					const FHTNPlanStepID ParentStepID = PlanLevel.ParentStepID;
					const FHTNPlanStep& ParentStep = Plan->GetStep(ParentStepID);
					if (UHTNTask_SubPlan* const SubPlanTask = Cast<UHTNTask_SubPlan>(ParentStep.Node))
					{
						// Make a copy of the plan if we want to mutate it.
						if (!bCopiedPlan)
						{
							Plan = MakeShared<FHTNPlan>(*Plan);
							bCopiedPlan = true;
						}

						ExtractedSubPlanMap.Add(ParentStepID, ExtractSubPlan(LevelIndex));
					}
				}
			}

			Plan->CheckIntegrity();
			return Plan;
		}

		// If the given level index is valid, will return a new plan that contains a copy of this sublevel and its sublevels (recursively).
		// Copied levels in the original plan will be replaced with a dummy level.
		// Level indices in the new plan will be fixed up.
		TSharedPtr<FHTNPlan> ExtractSubPlan(int32 PlanLevelIndex)
		{
			if (!Plan->Levels.IsValidIndex(PlanLevelIndex))
			{
				return nullptr;
			}

			const TSharedRef<FHTNPlan> SubPlan = MakeShared<FHTNPlan>();
			{
				SubPlan->RecursionCounts = Plan->RecursionCounts;

				const FHTNPlanLevel& SourceLevel = *Plan->Levels[PlanLevelIndex];
				SubPlan->Cost = SourceLevel.Cost;
				if (const FHTNPlanStep* const ParentStep = Plan->FindStep(SourceLevel.ParentStepID))
				{
					SubPlan->RootNodeOverride = ParentStep->Node;
				}
				// We don't copy over priority markers because they were only relevant when planning the outer plan.

				ExtractAndFixUpLevelIndices(*SubPlan, PlanLevelIndex);
			}

			SubPlan->CheckIntegrity();
			return SubPlan;
		}

		int32 ExtractAndFixUpLevelIndices(FHTNPlan& SubPlan, int32 SourceLevelIndex, FHTNPlanStepID NewParentStepID = FHTNPlanStepID::None)
		{
			if (!Plan->Levels.IsValidIndex(SourceLevelIndex))
			{
				return SourceLevelIndex;
			}

			// Extract a copy of the old level and replace the original with a dummy.
			// We copy the level instead of appropriating the existing one 
			// because the old one can be referenced by others and we need to modify it in the subplan.
			TSharedPtr<FHTNPlanLevel>& SourceLevel = Plan->Levels[SourceLevelIndex];
			const TSharedRef<FHTNPlanLevel> NewLevel = MakeShared<FHTNPlanLevel>(*SourceLevel);
			SourceLevel = FHTNPlanLevel::DummyPlanLevel;

			const int32 NewLevelIndex = SubPlan.Levels.Add(NewLevel);
			NewLevel->ParentStepID = NewParentStepID;

			for (int32 StepIndex = 0; StepIndex < NewLevel->Steps.Num(); ++StepIndex)
			{
				FHTNPlanStep& Step = NewLevel->Steps[StepIndex];
				const FHTNPlanStepID NewStepID = { NewLevelIndex, StepIndex };
				Step.SubLevelIndex = ExtractAndFixUpLevelIndices(SubPlan, Step.SubLevelIndex, NewStepID);
				Step.SecondarySubLevelIndex = ExtractAndFixUpLevelIndices(SubPlan, Step.SecondarySubLevelIndex, NewStepID);
			}

			return NewLevelIndex;
		}

		TSharedRef<FHTNPlan> Plan;
		FHTNExtractedSubPlansMap& ExtractedSubPlanMap;
		bool bCopiedPlan = false;
	};

	EHTNPlanInstanceStatus FinishReactionToPlanInstanceStatus(EHTNPlanInstanceFinishReaction Reaction)
	{
		switch (Reaction)
		{
		case EHTNPlanInstanceFinishReaction::Succeed:
			return EHTNPlanInstanceStatus::Succeeded;
		case EHTNPlanInstanceFinishReaction::Fail:
			return EHTNPlanInstanceStatus::Failed;
		case EHTNPlanInstanceFinishReaction::Loop:
			return EHTNPlanInstanceStatus::InProgress;
		default:
			checkNoEntry();
			return EHTNPlanInstanceStatus::Failed;
		}
	}

	bool IsInsideBlackboardNotifyObservers(const UBlackboardComponent* BlackboardComponent)
	{
		struct FBlackboardProtectedMemberAccessHelper : public UBlackboardComponent
		{
			using UBlackboardComponent::NotifyObserversRecursionCount;
		};

		if (BlackboardComponent)
		{
			return StaticCast<const FBlackboardProtectedMemberAccessHelper*>(BlackboardComponent)->NotifyObserversRecursionCount > 0;
		}

		return false;
	}
}

const FHTNPlanInstanceConfig FHTNPlanInstanceConfig::Default = {};

FHTNPlanInstanceConfig::FHTNPlanInstanceConfig() :
	SucceededReaction(EHTNPlanInstanceFinishReaction::Loop),
	FailedReaction(EHTNPlanInstanceFinishReaction::Loop),
	bPlanDuringExecution(true),
	bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable(true),
	Depth(0),
	RootNodeOverride(nullptr)
{}

bool FHTNPlanInstanceConfig::ShouldStopIfSucceeded() const
{
	return SucceededReaction != EHTNPlanInstanceFinishReaction::Loop;
}

bool FHTNPlanInstanceConfig::ShouldStopIfFailed() const
{
	return FailedReaction != EHTNPlanInstanceFinishReaction::Loop;
}

UHTNPlanInstance::UHTNPlanInstance() :
	Status(EHTNPlanInstanceStatus::NotStarted),
	OwnerComponent(nullptr),
	CurrentPlanningTask(nullptr),
	LockFlags(EHTNLockFlags::None),
	PlanExecutionCount(0),
	bCurrentPlanStartedExecution(false),
	bDeferredAbortPlan(false),
	bAbortingPlan(false),
	bDeferredStartPlanningTask(false),
	bBlockNotifyPlanInstanceFinished(false)
{}

void UHTNPlanInstance::Initialize(UHTNComponent& InOwnerComponent, FHTNPlanInstanceID InID, const FHTNPlanInstanceConfig& InConfig)
{
	OwnerComponent = &InOwnerComponent;
	ID = InID;
	Config = InConfig;

#if ENABLE_VISUAL_LOG
	if (OwnerComponent->GetWorld())
	{
		REDIRECT_TO_VLOG(OwnerComponent->GetOwner());
	}
#endif

	if (Config.PrePlannedPlan.IsValid())
	{
		// We initialize the subplan for execution (set up nodes, instances and call InitializeMemory on them)
		// This makes it possible for nodes in the subplan to run preparation logic even before their OnPlanExecutionStarted event.
		SetPlanPendingExecution(Config.PrePlannedPlan, /*bTryToInitialize=*/true);
	}
}

void UHTNPlanInstance::OnPreDestroy()
{
	DeleteAllWorldStates();
}

void UHTNPlanInstance::DeleteAllWorldStates()
{
	Reset();

	// We need to do this to get rid of any worldstates in the plan, 
	// so that the worldstates are guaranteed to be destroyed before the blackboard component they depend on.
	Config.PrePlannedPlan.Reset();
}

FHTNPlanInstanceID UHTNPlanInstance::GetID() const
{
	return ID;
}

int32 UHTNPlanInstance::GetDepth() const
{
	return Config.Depth;
}

EHTNPlanInstanceStatus UHTNPlanInstance::GetStatus() const
{
	return Status;
}

EHTNLockFlags UHTNPlanInstance::GetLockFlags() const
{
	return LockFlags;
}

bool UHTNPlanInstance::IsPlanning() const
{
	return IsValid(CurrentPlanningTask);
}

bool UHTNPlanInstance::HasPlan() const
{
	return CurrentPlan.IsValid() && bCurrentPlanStartedExecution;
}

bool UHTNPlanInstance::HasActiveTasks() const
{
	return CurrentlyExecutingStepIDs.Num() || PendingExecutionStepIDs.Num() || CurrentlyAbortingStepIDs.Num();
}

bool UHTNPlanInstance::HasActivePlan() const
{
	return HasPlan() && HasActiveTasks();
}

bool UHTNPlanInstance::IsAbortingPlan() const
{
	return bAbortingPlan;
}

bool UHTNPlanInstance::HasDeferredAbort() const
{
	// This intentionally doesn't check if a parent plan instance HasDeferredAbort:
	// Even if the plan instance containing this one is going to abort, we don't want to stop advancing *our* plan because
	// the subnode we're on might be configured to let the plan run its course during the latent abort of the SubPlan task.
	return bDeferredAbortPlan || (OwnerComponent && OwnerComponent->HasDeferredStop());
}

bool UHTNPlanInstance::IsWaitingForAbortingTasks() const
{
	return CurrentlyAbortingStepIDs.Num() > 0;
}

bool UHTNPlanInstance::CanStartExecutingNewPlan() const
{
	// !IsWaitingForAbortingTasks() is implied by !HasActivePlan() and so does not need to be checked separately.
	// !OwnerComponent->HasDeferredStop() is implied by !HasDeferredAbort().
	const bool bCanStartExecutingNewPlan = !HasActivePlan() && !IsAbortingPlan() && !HasDeferredAbort() && !OwnerComponent->IsPaused();

#if !HTN_ALLOW_START_PLAN_DURING_BLACKBOARD_NOTIFY_OBSERVERS
	if (bCanStartExecutingNewPlan && IsInsideBlackboardNotifyObservers(OwnerComponent->GetBlackboardComponent()))
	{
		UE_VLOG_UELOG(this, LogHTN, Log, 
			TEXT("Can't start a new plan from inside UBlackboardComponent::NotifyObservers, will be able to do so after the call to NotifyObservers is over"));
		return false;
	}
#endif

	return bCanStartExecutingNewPlan;
}

UHTNNode* UHTNPlanInstance::GetNodeInstanceInCurrentPlan(const UHTNNode& NodeTemplate, const uint8* NodeMemory) const
{
	if (OwnsNodeMemory(NodeMemory))
	{
		const FHTNNodeSpecialMemory* const SpecialMemory = NodeTemplate.GetSpecialNodeMemory<FHTNNodeSpecialMemory>(NodeMemory);
		if (ensure(SpecialMemory))
		{
			if (ensure(NodeInstances.IsValidIndex(SpecialMemory->NodeInstanceIndex)))
			{
				return NodeInstances[SpecialMemory->NodeInstanceIndex];
			}
		}
	}

	return nullptr;
}

bool UHTNPlanInstance::OwnsNodeMemory(const uint8* NodeMemory) const
{
	// The range intentionally uses <= instead of < at the end for when the (non-special) memory use of the last node is 0.
	return NodeMemory && PlanMemory.Num() > 0 && PlanMemory.GetData() <= NodeMemory && NodeMemory <= PlanMemory.GetData() + PlanMemory.Num();
}

FHTNNodeInPlanInfo UHTNPlanInstance::FindActiveTaskInfo(const UHTNTask* Task, const uint8* NodeMemory) const
{
	if (NodeMemory && !OwnsNodeMemory(NodeMemory))
	{
		return {};
	}

	if (!HasActivePlan())
	{
		return {};
	}
	
	const UHTNTask* const TaskTemplate = Task ? StaticCast<const UHTNTask*>(Task->GetTemplateNode()) : nullptr;
	if (!IsValid(TaskTemplate))
	{
		return {};
	}

	FHTNNodeInPlanInfo Result;
	Result.PlanInstance = const_cast<UHTNPlanInstance*>(this);

	const auto FindIn = [&](const TArray<FHTNPlanStepID>& StepIDs) -> bool
	{
		for (const FHTNPlanStepID& StepID : StepIDs)
		{
			uint8* TaskMemory = nullptr;
			if (TaskTemplate == &GetTaskInCurrentPlan(StepID, TaskMemory))
			{
				if (!NodeMemory || NodeMemory == TaskMemory)
				{
					Result.NodeMemory = TaskMemory;
					Result.PlanStepID = StepID;
					return true;
				}
			}
		}

		return false;
	};

	if (FindIn(CurrentlyExecutingStepIDs))
	{
		Result.Status = EHTNTaskStatus::Active;
	} 
	else if (FindIn(CurrentlyAbortingStepIDs))
	{
		Result.Status = EHTNTaskStatus::Aborting;
	}
	else
	{
		Result = {};
	}

	return Result;
}

FHTNNodeInPlanInfo UHTNPlanInstance::FindActiveDecoratorInfo(const UHTNDecorator* Decorator, const uint8* NodeMemory) const
{
	if (NodeMemory && !OwnsNodeMemory(NodeMemory))
	{
		return {};
	}

	if (!HasActivePlan())
	{
		return {};
	}

	const UHTNNode* const NodeTemplate = Decorator ? Decorator->GetTemplateNode() : nullptr;
	if (!IsValid(NodeTemplate))
	{
		return {};
	}

	FHTNNodeInPlanInfo Result;
	Result.PlanInstance = const_cast<UHTNPlanInstance*>(this);

	FHTNSubNodeGroups SubNodeGroups;
	const auto FindIn = [&](const TArray<FHTNPlanStepID>& StepIDs) -> bool
	{
		for (const FHTNPlanStepID& StepID : StepIDs)
		{
			SubNodeGroups.Reset();
			CurrentPlan->GetSubNodesAtPlanStep(StepID, SubNodeGroups);
			for (const FHTNSubNodeGroup& Group : SubNodeGroups)
			{
				for (const THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Group.SubNodesInfo->DecoratorInfos)
				{
					if (NodeTemplate == DecoratorInfo.TemplateNode)
					{
						uint8* const FoundMemory = GetNodeMemory(DecoratorInfo.NodeMemoryOffset);
						if (!NodeMemory || NodeMemory == FoundMemory)
						{
							Result.NodeMemory = FoundMemory;
							Result.PlanStepID = StepID;
							return true;
						}
					}
				}
			}
		}

		return false;
	};

	if (FindIn(CurrentlyExecutingStepIDs))
	{
		Result.Status = EHTNTaskStatus::Active;
	}
	else if (FindIn(CurrentlyAbortingStepIDs))
	{
		Result.Status = EHTNTaskStatus::Aborting;
	}
	else
	{
		Result = {};
	}

	return Result;
}

FHTNNodeInPlanInfo UHTNPlanInstance::FindActiveServiceInfo(const UHTNService* Service, const uint8* NodeMemory) const
{
	if (NodeMemory && !OwnsNodeMemory(NodeMemory))
	{
		return {};
	}

	if (!HasActivePlan())
	{
		return {};
	}

	const UHTNNode* const NodeTemplate = Service ? Service->GetTemplateNode() : nullptr;
	if (!IsValid(NodeTemplate))
	{
		return {};
	}

	FHTNNodeInPlanInfo Result;
	Result.PlanInstance = const_cast<UHTNPlanInstance*>(this);

	FHTNSubNodeGroups SubNodeGroups;
	const auto FindIn = [&](const TArray<FHTNPlanStepID>& StepIDs) -> bool
	{
		for (const FHTNPlanStepID& StepID : StepIDs)
		{
			SubNodeGroups.Reset();
			CurrentPlan->GetSubNodesAtPlanStep(StepID, SubNodeGroups);
			for (const FHTNSubNodeGroup& Group : SubNodeGroups)
			{
				for (const THTNNodeInfo<UHTNService>& ServiceInfo : Group.SubNodesInfo->ServiceInfos)
				{
					if (NodeTemplate == ServiceInfo.TemplateNode)
					{
						uint8* const FoundMemory = GetNodeMemory(ServiceInfo.NodeMemoryOffset);
						if (!NodeMemory || NodeMemory == FoundMemory)
						{
							Result.NodeMemory = FoundMemory;
							Result.PlanStepID = StepID;
							return true;
						}
					}
				}
			}
		}

		return false;
	};

	if (FindIn(CurrentlyExecutingStepIDs))
	{
		Result.Status = EHTNTaskStatus::Active;
	}
	else if (FindIn(CurrentlyAbortingStepIDs))
	{
		Result.Status = EHTNTaskStatus::Aborting;
	}
	else
	{
		Result = {};
	}

	return Result;
}

bool UHTNPlanInstance::IsRootInstance() const
{
	return OwnerComponent && this == &OwnerComponent->GetRootPlanInstance();
}

#if USE_HTN_DEBUGGER

FHTNDebugExecutionPerPlanInstanceInfo UHTNPlanInstance::GetDebugStepInfo() const
{
	FHTNDebugExecutionPerPlanInstanceInfo Info;
	Info.HTNPlan = CurrentPlan;
	Info.ActivePlanStepIDs = CurrentlyExecutingStepIDs;
	Info.ActivePlanStepIDs.Append(CurrentlyAbortingStepIDs);

	return Info;
}

#endif

#if ENABLE_VISUAL_LOG

void UHTNPlanInstance::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	// Describe plan instance
	{
		const FString PlanInstanceCategoryName = FString::Printf(TEXT("HTN Plan Instance %d (%s)"), 
			GetID().ID,
			Config.RootNodeOverride ? *Config.RootNodeOverride->GetShortDescription() : TEXT("root"));
		FVisualLogStatusCategory& PlanInstanceCategory = Snapshot->Status.Emplace_GetRef(PlanInstanceCategoryName);
		if (ActiveReplanParameters.IsSet())
		{
			PlanInstanceCategory.Add(TEXT("replanning: "), ActiveReplanParameters->ToCompactString());
		}
		if (IsPlanning())
		{
			PlanInstanceCategory.Add(TEXT("planning..."), CurrentPlanningTask->GetCurrentStateDescription());
		}
		PlanInstanceCategory.Add(DescribeCurrentPlan(), TEXT(""));
	}

	// Describe active steps
	{
		const auto FindOrAddCategory = [&](const FString& CategoryName) -> FVisualLogStatusCategory&
		{
			if (FVisualLogStatusCategory* const ExistingCategory = Algo::FindBy(Snapshot->Status, CategoryName, &FVisualLogStatusCategory::Category))
			{
				return *ExistingCategory;
			}

			return Snapshot->Status.Emplace_GetRef(CategoryName);
		};

		const auto DescribePlanSteps = [&](TArrayView<const FHTNPlanStepID> StepIDs, const FString& CategoryName)
		{
			FVisualLogStatusCategory& Category = FindOrAddCategory(CategoryName);
			for (const FHTNPlanStepID& StepID : StepIDs)
			{
				DescribeActivePlanStepToVisLog(StepID, Category);
			}
		};

		DescribePlanSteps(CurrentlyExecutingStepIDs, TEXT("HTN Executing Plan Steps"));
		DescribePlanSteps(CurrentlyAbortingStepIDs, TEXT("HTN Aborting Plan Steps"));
	}
}

#endif

FString UHTNPlanInstance::DescribeCurrentPlan() const
{
	TStringBuilder<32768> StringBuilder;

	struct Local
	{
		static void DescribeLevel(decltype(StringBuilder)& SB, const UHTNPlanInstance& PlanInstance, int32 LevelIndex = 0, int32 IndentationDepth = 0)
		{
			const FHTNPlanLevel& Level = *PlanInstance.GetCurrentPlan()->Levels[LevelIndex];
			for (int32 StepIndex = 0; StepIndex < Level.Steps.Num(); ++StepIndex)
			{
				for (int32 I = 0; I < IndentationDepth; ++I)
				{
					SB << TEXT("    ");
				}

				const FHTNPlanStep& Step = Level.Steps[StepIndex];
				const FHTNPlanStepID StepID { LevelIndex, StepIndex };
				
				const TCHAR* const Prefix =
					PlanInstance.CurrentlyExecutingStepIDs.Contains(StepID) ? TEXT("> ") :
					PlanInstance.CurrentlyAbortingStepIDs.Contains(StepID) ? TEXT("> (aborting) ") :
					PlanInstance.PendingExecutionStepIDs.Contains(StepID) ? TEXT("> (pending) ") :
					TEXT("");

				SB << Prefix << Step.Node->GetShortDescription() << TEXT("\n");

				if (Step.SubLevelIndex != INDEX_NONE)
				{
					DescribeLevel(SB, PlanInstance, Step.SubLevelIndex, IndentationDepth + 1);
				}
				if (Step.SecondarySubLevelIndex != INDEX_NONE)
				{
					DescribeLevel(SB, PlanInstance, Step.SecondarySubLevelIndex, IndentationDepth + 1);
				}
			}
		}
	};

	if (CurrentPlan.IsValid())
	{
		Local::DescribeLevel(StringBuilder, *this);
	}
	else
	{
		StringBuilder << TEXT("no plan");
	}

	return StringBuilder.ToString();
}

TSharedPtr<FHTNPlan> UHTNPlanInstance::GetPlanPendingExecution() const
{
	return 
		PlanPendingExecution.IsValid() ? PlanPendingExecution :
		!bCurrentPlanStartedExecution && CurrentPlan.IsValid() ? CurrentPlan :
		nullptr;
}

void UHTNPlanInstance::SetPlanPendingExecution(TSharedPtr<FHTNPlan> Plan, bool bTryToInitialize)
{
	const TSharedPtr<FHTNPlan> PendingPlan = GetPlanPendingExecution();
	if (Plan == PendingPlan)
	{
		return;
	}

	// If the current plan is pending execution, clear it.
	if (!bCurrentPlanStartedExecution && CurrentPlan.IsValid())
	{
		ClearCurrentPlan();
	}

	PlanPendingExecution = Plan;

	if (bTryToInitialize && PlanPendingExecution.IsValid() && !bCurrentPlanStartedExecution)
	{
		InitializeCurrentPlan(PlanPendingExecution.ToSharedRef());
		PlanPendingExecution.Reset();
	}
}

void UHTNPlanInstance::Start()
{
	if (Status == EHTNPlanInstanceStatus::InProgress)
	{
		UE_VLOG_UELOG(this, LogHTN, Warning,
			TEXT("%s trying to Start plan instance while its status is %s. Ignoring."),
			*GetLogPrefix(),
			*UEnum::GetDisplayValueAsText(Status).ToString());
		return;
	}

	UE_VLOG(this, LogHTN, Log, TEXT("%s start"), *GetLogPrefix());
	Status = EHTNPlanInstanceStatus::InProgress;

	if (!ShouldReusePrePlannedPlan())
	{
		ClearCurrentPlan();
	}

	{
		FHTNScopedLock ScopedLock(LockFlags, EHTNLockFlags::LockTick);
		UpdateCurrentExecutionAndPlanning();
	}

	ProcessDeferredActions();
	UE_VLOG(this, LogHTN, Log, TEXT("%s start finished"), *GetLogPrefix());
}

void UHTNPlanInstance::Tick(float DeltaTime)
{
	UE_VLOG(this, LogHTN, VeryVerbose, TEXT("%s tick"), *GetLogPrefix());

	{
		FHTNScopedLock ScopedLock(LockFlags, EHTNLockFlags::LockTick);
		UpdateCurrentExecutionAndPlanning();

		if (HasActivePlan())
		{
			TickCurrentPlan(DeltaTime);
		}
	}

	ProcessDeferredActions();
	UE_VLOG(this, LogHTN, VeryVerbose, TEXT("%s tick finished"), *GetLogPrefix());
}

void UHTNPlanInstance::Stop(bool bDisregardLatentAbort)
{
	if (Status != EHTNPlanInstanceStatus::InProgress)
	{
		UE_VLOG(this, LogHTN, Warning,
			TEXT("%s trying to Stop plan instance while its status is '%s'. Ignoring."),
			*GetLogPrefix(),
			*UEnum::GetDisplayValueAsText(Status).ToString());
		return;
	}

	UE_VLOG(this, LogHTN, Log, TEXT("%s stop (bDisregardLatentAbort=%s)"), *GetLogPrefix(), bDisregardLatentAbort ? TEXT("True") : TEXT("False"));

	UE_CVLOG(ActiveReplanParameters.IsSet(), this, LogHTN, Log, 
		TEXT("%s Unsetting active replan parameters %s because the plan instance is being stopped"), 
		*GetLogPrefix(), *ActiveReplanParameters->ToCompactString());
	ActiveReplanParameters.Reset();

	{
		// Prevent NotifyPlanInstanceFinished from being done from inside AbortCurrentPlan and the rest of this scope.
		FGuardValue_Bitfield(bBlockNotifyPlanInstanceFinished, true);

		CancelActivePlanning();
		AbortCurrentPlan();

		// Deal with tasks that are taking multiple frames to abort. Force-abort them if we need to bDisregardLatentAbort.
		// We do this here instead of inside AbortCurrentPlan in case some tasks were already in the process of being aborted before we started aborting the whole plan.
		if (IsWaitingForAbortingTasks())
		{
			if (!bDisregardLatentAbort)
			{
				UE_VLOG(this, LogHTN, Log, TEXT("%s Stop is waiting for aborting tasks to finish..."), *GetLogPrefix());
			}
			else
			{
				UE_VLOG_UELOG(this, LogHTN, Warning, TEXT("%s Stop was forced while waiting for tasks to finish aborting."), *GetLogPrefix());
				ForceFinishAbortingTasks();
			}
		}

		SetPlanPendingExecution(nullptr);
	}

	if (!HasPlan())
	{
		Status = GetStatusAfterFailed();
		if (Status != EHTNPlanInstanceStatus::InProgress)
		{
			NotifyPlanInstanceFinished();
		}
	}

	UE_VLOG(this, LogHTN, Log, TEXT("%s stop finished"), *GetLogPrefix());
}

void UHTNPlanInstance::Replan(const FHTNReplanParameters& Params)
{
	if (!ensure(IsValid(OwnerComponent)))
	{
		return;
	}

	if (Params.bReplanOutermostPlanInstance && !IsRootInstance())
	{
		UE_VLOG(this, LogHTN, Log,
			TEXT("%s Replan %s called with bReplanOutermostPlanInstance=true on a plan instance that is not root. Redirecting call to root instance of the HTNComponent."),
			*GetLogPrefix(), *Params.ToCompactString());
		OwnerComponent->GetRootPlanInstance().Replan(Params);
		return;
	}

	if (ActiveReplanParameters.IsSet())
	{
		// IF we have a replan for TryToAdjustCurrentPlan and we try to start a Normal replan, cancel the current replan. 
		// Otherwise let the orignal replan continue.
		if (ActiveReplanParameters->PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan && 
			Params.PlanningType != EHTNPlanningType::TryToAdjustCurrentPlan)
		{
			UE_VLOG(this, LogHTN, Log, 
				TEXT("%s cancelling current Replan %s because of new Replan %s"), 
				*GetLogPrefix(), *ActiveReplanParameters->ToCompactString(), *Params.ToCompactString());
			CancelActivePlanning();
		}
		else
		{
			UE_VLOG(this, LogHTN, Log,
				TEXT("%s ignoring Replan because already has active replan: %s"),
				*GetLogPrefix(), *ActiveReplanParameters->ToCompactString());
			return;
		}
	}

	UE_VLOG(this, LogHTN, Log, TEXT("%s Replan %s"), *GetLogPrefix(), *Params.ToCompactString());
	ActiveReplanParameters = Params;

	const EHTNPlanInstanceStatus ExpectedStatus = GetStatusAfterFailed();
	if (ExpectedStatus != EHTNPlanInstanceStatus::InProgress)
	{
		UE_VLOG(this, LogHTN, Log, 
			TEXT("%s Replan called on plan instance that isn't allowed to loop, stopping plan instance instead. This behavior can be overridden by enabling bForceReplan in the Replan parameters"), 
			*GetLogPrefix());
		Stop();
		return;
	}

	if (Params.bForceAbortPlan && CurrentPlan.IsValid())
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s Replan: aborting current plan"), *GetLogPrefix());
		AbortCurrentPlan(Params.bForceDeferToNextFrame);
	}

	if (Params.bForceRestartActivePlanning || !IsPlanning())
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s Replan: starting new planning"), *GetLogPrefix());
		StartPlanningTask(Params.bForceDeferToNextFrame);
	}
}

void UHTNPlanInstance::AbortCurrentPlan(bool bForceDeferToNextFrame)
{
	// Don't start aborting if already aborting.
	if (IsAbortingPlan())
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s ignoring AbortCurrentPlan because already aborting"), *GetLogPrefix());
		bDeferredAbortPlan = false;
		return;
	}

	if (bForceDeferToNextFrame)
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s deferring AbortCurrentPlan because bForceDeferToNextFrame=True"), *GetLogPrefix());
		bDeferredAbortPlan = true;
		return;
	}
	
	if (!OwnerComponent->IsStoppingHTN())
	{
		if (OwnerComponent->IsPaused())
		{
			UE_VLOG(this, LogHTN, Log, TEXT("%s deferring AbortCurrentPlan because the HTNComponent is Paused"), *GetLogPrefix());
			bDeferredAbortPlan = true;
			return;
		}

		if (LockFlags != EHTNLockFlags::None)
		{
			UE_VLOG(this, LogHTN, Log, TEXT("%s deferring AbortCurrentPlan because it was called while the plan instance has lock flags: %s"), 
				*GetLogPrefix(),
				*HTNLockFlagsToString(LockFlags));
			bDeferredAbortPlan = true;
			return;
		}
	}

	FHTNScopedLock ScopedLock(LockFlags, EHTNLockFlags::LockAbortPlan);
	ON_SCOPE_EXIT { bDeferredAbortPlan = false; };

	if (HasPlan())
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s started aborting plan"), *GetLogPrefix());
		bAbortingPlan = true;

		// The pending execution steps could be holding some subnodes active. Finish those to avoid scope leak.
		RemovePendingExecutionPlanSteps();

		if (!CurrentlyExecutingStepIDs.IsEmpty())
		{
			while (!CurrentlyExecutingStepIDs.IsEmpty())
			{
				// AbortExecutingPlanStep removes this step from CurrentlyExecutingStepIDs.
				// In rare cases this may cause another task to abort as well, so more than one step ID is removed.
				AbortExecutingPlanStep(CurrentlyExecutingStepIDs.Top());
			}
		}
		else
		{
			if (IsWaitingForAbortingTasks())
			{
				UE_VLOG(this, LogHTN, Log, TEXT("%s waiting for plan to finish aborting"), *GetLogPrefix());
			}
			else
			{
				OnPlanAbortFinished();
			}
		}
	}
}

void UHTNPlanInstance::ForceFinishAbortingTasks()
{
	for (int32 I = CurrentlyAbortingStepIDs.Num() - 1; I >= 0; I--)
	{
		const FHTNPlanStepID StepID = CurrentlyAbortingStepIDs[I];
		uint8* TaskMemory = nullptr;
		UHTNTask& Task = GetTaskInCurrentPlan(StepID, TaskMemory);
		OnTaskFinished(&Task, TaskMemory, StepID, EHTNNodeResult::Aborted);
	}
	ensure(!IsWaitingForAbortingTasks());
}

void UHTNPlanInstance::CancelActivePlanning()
{
	if (CurrentPlanningTask)
	{
		CurrentPlanningTask->ExternalCancel();
		if (CurrentPlanningTask)
		{
			UAITask_MakeHTNPlan* const PlanningTask = CurrentPlanningTask;
			CurrentPlanningTask = nullptr;
			PlanningTask->Clear();
			ensure(CurrentPlanningTask == nullptr);
		}
	}
}

void UHTNPlanInstance::Reset()
{
	if (Status == EHTNPlanInstanceStatus::InProgress)
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s force-stopping due to plan instance being reset"), *GetLogPrefix());
		Stop(/*bDisregardLatentAbort=*/true);
	}

	SetPlanPendingExecution(nullptr);
	ClearCurrentPlan();
	CancelActivePlanning();
	Status = EHTNPlanInstanceStatus::NotStarted;
	PlanExecutionCount = 0;

#if USE_HTN_DEBUGGER
	if (IsRootInstance())
	{
		OwnerComponent->DebuggerSteps.Reset();
	}
#endif
}

void UHTNPlanInstance::OnTaskFinished(const UHTNTask* Task, uint8* TaskMemory, const FHTNPlanStepID& FinishedStepID, EHTNNodeResult Result)
{
	if (!IsValid(Task))
	{
		UE_VLOG_UELOG(this, LogHTN, Warning, 
			TEXT("%s OnTaskFinished was called with an invalid task. Ignoring."), 
			*GetLogPrefix());
		return;
	}

	UE_VLOG(this, LogHTN, Verbose, TEXT("%s OnTaskFinished: task %s, result: %s"),
		*GetLogPrefix(),
		*Task->GetShortDescription(),
		*UEnum::GetValueAsString(Result));

	if (!HasPlan())
	{
		UE_VLOG_UELOG(this, LogHTN, Warning, 
			TEXT("%s OnTaskFinished was called while the plan instance has no current plan. Ignoring."), 
			*GetLogPrefix());
		return;
	}

	// Validate the Result. Warn and force-change it if it's not valid for the current context.
	if (Result == EHTNNodeResult::InProgress)
	{
		Result = CurrentlyAbortingStepIDs.Contains(FinishedStepID) ? EHTNNodeResult::Aborted : EHTNNodeResult::Failed;
		UE_VLOG_UELOG(this, LogHTN, Warning,
			TEXT("%s %s OnTaskFinished was called with EHTNNodeResult::InProgress. Force-changing Result to %s"),
			*GetLogPrefix(),
			*Task->GetShortDescription(),
			*UEnum::GetValueAsString(Result));
	}
	else if (Result == EHTNNodeResult::Aborted)
	{
		if (!CurrentlyAbortingStepIDs.Contains(FinishedStepID))
		{
			UE_VLOG_UELOG(this, LogHTN, Warning,
				TEXT("%s %s was not aborting but OnTaskFinished was called with EHTNNodeResult::Aborted. Expected Succeeded or Failed. Force-changing Result to Failed."),
				*GetLogPrefix(),
				*Task->GetShortDescription());
			Result = EHTNNodeResult::Failed;
		}
	}
	else if (ensure(Result == EHTNNodeResult::Succeeded || Result == EHTNNodeResult::Failed))
	{
		if (CurrentlyAbortingStepIDs.Contains(FinishedStepID))
		{
			UE_VLOG_UELOG(this, LogHTN, Warning,
				TEXT("%s %s was aborting but OnTaskFinished was called with %s. Expected Aborted. Force-changing Result to Aborted."),
				*GetLogPrefix(),
				*Task->GetShortDescription(),
				*UEnum::GetValueAsString(Result));
			Result = EHTNNodeResult::Aborted;
		}
		else if (!CurrentlyExecutingStepIDs.Contains(FinishedStepID))
		{
			UE_VLOG_UELOG(this, LogHTN, Warning,
				TEXT("%s %s was not executing but OnTaskFinished was called with %s. Ignoring."),
				*GetLogPrefix(),
				*Task->GetShortDescription(),
				*UEnum::GetValueAsString(Result));
			return;
		}
	}

	// If a call to OnTaskFinished caused another call to OnTaskFinished, 
	// we only want do things like OnPlanAbortFinished/OnPlanExecutionSuccessfullyFinished from the outermost call.
	const bool bReentrantCallToOnTaskFinished = EnumHasAnyFlags(LockFlags, EHTNLockFlags::LockOnTaskFinished);
	FHTNScopedLock ScopedLock(LockFlags, EHTNLockFlags::LockOnTaskFinished);

	const UHTNTask* const TaskTemplate = StaticCast<const UHTNTask*>(Task->GetTemplateNode());
	TaskTemplate->WrappedOnTaskFinished(*OwnerComponent, TaskMemory, Result);
	if (Result == EHTNNodeResult::Succeeded)
	{
		UE_VLOG(this, LogHTN, Verbose, TEXT("%s finished task %s"),
			*GetLogPrefix(),
			*Task->GetShortDescription());

		// Apply worldstate changes of the finished step.
		// This may affect event-based Blackboard decorators (e.g., a SetValue, ClearValue task), 
		// so we do this before we exit the decorators or remove the finished step id from CurrentlyExecutingStepIDs.
		const FHTNPlanStep& FinishedStep = CurrentPlan->GetStep(FinishedStepID);
		FinishedStep.WorldState->ApplyChangedValues(*OwnerComponent->GetBlackboardComponent());

		NotifySublevelFinishedIfNeeded(FinishedStepID);

		if (!IsAbortingPlan() && !HasDeferredAbort())
		{
			// Pick next tasks to execute.
			// This has to be done before we FinishSubNodesAtPlanStep so that it doesn't accidentally finish subnodes that should still be active during the new steps`.
			GetNextPrimitiveStepsInCurrentPlan(PendingExecutionStepIDs, FinishedStepID);
		}
	}
	FinishSubNodesAtPlanStep(FinishedStepID, Result);
	if (Result == EHTNNodeResult::Succeeded)
	{
		AbortSecondaryParallelBranchesIfNeeded(FinishedStepID);
		check(HasPlan());
	}
	CurrentlyExecutingStepIDs.RemoveSingle(FinishedStepID);

	check(OwnerComponent->GetBlackboardComponent());
	check(HasPlan());
	CurrentPlan->CheckIntegrity();

	if (Result == EHTNNodeResult::Succeeded)
	{
		if (!IsAbortingPlan() && !HasDeferredAbort())
		{
			if (!bReentrantCallToOnTaskFinished && !HasActiveTasks())
			{
				OnPlanExecutionSuccessfullyFinished();
			}
		}
		else
		{
			// If we won't continue, stop subnodes on nodes containing this one. 
			// Normally this would be done when finishing the next tasks, 
			// but we decided not to start those because we're aborting.
			FinishSubNodesAtPlanStep(FinishedStepID, EHTNNodeResult::Aborted);
		}
	}
	else if (Result == EHTNNodeResult::Aborted)
	{
		UE_VLOG(this, LogHTN, Verbose, TEXT("%s finished aborting task %s"),
			*GetLogPrefix(),
			*Task->GetShortDescription());

		CurrentlyAbortingStepIDs.RemoveSingle(FinishedStepID);
		if (!bReentrantCallToOnTaskFinished && !HasActiveTasks())
		{
			if (IsAbortingPlan())
			{
				OnPlanAbortFinished();
			}
			else
			{
				OnPlanExecutionSuccessfullyFinished();
			}
		}
	}
	else
	{
		UE_VLOG(this, LogHTN, Verbose, TEXT("%s failed task %s"),
			*GetLogPrefix(),
			*Task->GetShortDescription());
		AbortCurrentPlan();
	}
}

bool UHTNPlanInstance::NotifyEventBasedDecoratorCondition(const UHTNDecorator* Decorator, const uint8* NodeMemory, bool bFinalConditionValue, bool bCanAbortPlanInstantly)
{
	const UHTNDecorator* const DecoratorTemplate = StaticCast<const UHTNDecorator*>(Decorator->GetTemplateNode());
	if (!IsValid(DecoratorTemplate) || !NodeMemory || !ensure(OwnsNodeMemory(NodeMemory)))
	{
		return false;
	}

	if (!HasActivePlan() || IsAbortingPlan())
	{
		return false;
	}

	const uint16 ExpectedMemoryOffset = NodeMemory - PlanMemory.GetData();
	FHTNSubNodeGroup Group;
	if (!CurrentPlan->FindExecutingSubNodeGroupWithDecoratorWithMemoryOffset(Group, ExpectedMemoryOffset))
	{
		UE_VLOG_UELOG(this, LogHTN, Warning, TEXT("%s NotifyEventBasedDecoratorCondition could not find the group of subnodes to which decorator %s belongs"),
			*GetLogPrefix(),
			*Decorator->GetShortDescription());
		return false;
	}

	check(Group && Algo::FindBy(Group.SubNodesInfo->DecoratorInfos, ExpectedMemoryOffset, &THTNNodeInfo<UHTNDecorator>::NodeMemoryOffset) != nullptr);

	bool bCanReplan = !Group.bIsIfNodeFalseBranch ?
		Group.bCanConditionsInterruptTrueBranch && !bFinalConditionValue :
		// To interrupt a false branch, all decorators must be true.
		Group.bCanConditionsInterruptFalseBranch && bFinalConditionValue && Algo::AllOf(Group.SubNodesInfo->DecoratorInfos, [&](const THTNNodeInfo<UHTNDecorator>& OtherInfo)
	{
		uint8* const RawMemory = GetNodeMemory(OtherInfo.NodeMemoryOffset);
		const EHTNDecoratorTestResult Result = OtherInfo.TemplateNode->GetLastEffectiveConditionValue(RawMemory);
		return Result == EHTNDecoratorTestResult::Passed;
	});

	if (bCanReplan)
	{
		UE_VLOG(this, LogHTN, Log, TEXT("Decorator '%s' of node '%s' notified the plan instance of its condition, which forced a replan (bCanAbortPlanInstantly=%s)"),
			*Decorator->GetShortDescription(),
			GetCurrentPlan()->HasStep(Group.PlanStepID) ? *GetCurrentPlan()->GetStep(Group.PlanStepID).Node->GetShortDescription() : TEXT("root"),
			bCanAbortPlanInstantly ? TEXT("True") : TEXT("False"));

		FHTNReplanParameters Params;
		UE_IFVLOG(Params.DebugReason = FString::Printf(TEXT("NotifyEventBasedDecoratorCondition of decorator '%s'"), *Decorator->GetShortDescription()));
		Params.bForceAbortPlan = bCanAbortPlanInstantly;
		Params.bForceRestartActivePlanning = true;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 26
		// In versions prior to 4.26, there was a bug in Blackboard:
		// calling RemoveObserver while inside NotifyObservers would cause a crash (modifying a multimap while iterating over it).
		// Postponing the abort of the current plan and the start of a new planning until next frame prevents this from happening.
		Params.bForceDeferToNextFrame = true;
#endif
		Replan(Params);

		return true;
	}

	return false;
}

void UHTNPlanInstance::NotifySublevelFinishedIfNeeded(const FHTNPlanStepID& FinishedStepID)
{
	if (!CurrentlyExecutingStepIDs.Num())
	{
		return;
	}

	if (!IsLevelFinishedWhenStepFinished(FinishedStepID))
	{
		return;
	}

	const FHTNPlanLevel& FinishedStepLevel = *CurrentPlan->Levels[FinishedStepID.LevelIndex];
	const FHTNPlanStepID ParentStepID = FinishedStepLevel.ParentStepID;
	if (ParentStepID != FHTNPlanStepID::None)
	{
		const FHTNPlanStep& ParentStep = CurrentPlan->GetStep(ParentStepID);
		if (ensure(ParentStep.Node.IsValid()))
		{
			if (ParentStep.Node->OnSubLevelFinished(*this, ParentStepID, FinishedStepID.LevelIndex))
			{
				NotifySublevelFinishedIfNeeded(ParentStepID);
			}
		}
	}
}

void UHTNPlanInstance::AbortSecondaryParallelBranchesIfNeeded(const FHTNPlanStepID& FinishedStepID)
{
	if (!IsLevelFinishedWhenStepFinished(FinishedStepID))
	{
		return;
	}

	const FHTNPlanLevel& FinishedStepLevel = *CurrentPlan->Levels[FinishedStepID.LevelIndex];
	const FHTNPlanStepID ParentStepID = FinishedStepLevel.ParentStepID;
	if (ParentStepID != FHTNPlanStepID::None)
	{
		const FHTNPlanStep& ParentStep = CurrentPlan->GetStep(ParentStepID);
		if (UHTNNode_Parallel* const ParallelNode = Cast<UHTNNode_Parallel>(ParentStep.Node))
		{
			uint8* const NodeRawMemory = GetNodeMemory(ParentStep.NodeMemoryOffset);
			const UHTNNode_Parallel::FNodeMemory* const NodeMemory = ParallelNode->CastInstanceNodeMemory<UHTNNode_Parallel::FNodeMemory>(NodeRawMemory);
			if (FinishedStepID.LevelIndex == ParentStep.SubLevelIndex && NodeMemory->bIsExecutionComplete)
			{
				const auto IsStepUnderAbortedLevel = [&, SecondaryLevelIndex = ParentStep.SecondarySubLevelIndex]
				(const FHTNPlanStepID& StepID) -> bool
				{
					return CurrentPlan->HasStep(StepID, SecondaryLevelIndex);
				};

				RemovePendingExecutionPlanSteps(IsStepUnderAbortedLevel);

				for (int32 I = CurrentlyExecutingStepIDs.Num() - 1; I >= 0; --I)
				{
					if (IsStepUnderAbortedLevel(CurrentlyExecutingStepIDs[I]))
					{
						AbortExecutingPlanStep(CurrentlyExecutingStepIDs[I]);
					}
				}
			}
		}

		AbortSecondaryParallelBranchesIfNeeded(ParentStepID);
	}
}

bool UHTNPlanInstance::IsLevelFinishedWhenStepFinished(const FHTNPlanStepID& FinishedStepID) const
{
	const FHTNPlanLevel& FinishedStepLevel = *CurrentPlan->Levels[FinishedStepID.LevelIndex];

	// We have finished the current level of the plan if we're the last node...
	if (FinishedStepID.StepIndex == FinishedStepLevel.Steps.Num() - 1)
	{
		return true;
	}

	// ...or the one before the last and the last is an empty Optional or Prefer node.
	if (FinishedStepID.StepIndex == FinishedStepLevel.Steps.Num() - 2)
	{
		const FHTNPlanStep& LastStep = FinishedStepLevel.Steps.Last();
		if (!Cast<UHTNTask>(LastStep.Node) && 
			LastStep.SubLevelIndex == INDEX_NONE && 
			LastStep.SecondarySubLevelIndex == INDEX_NONE)
		{
			return true;
		}
	}

	return false;
}

uint8* UHTNPlanInstance::GetNodeMemory(uint16 MemoryOffset) const
{
	// The range intentionally includes PlanMemory.Num() for when the (non-special) memory use of the last node is 0.
	check(MemoryOffset >= 0 && MemoryOffset <= PlanMemory.Num());
	return const_cast<uint8*>(PlanMemory.GetData() + MemoryOffset);
}

int32 UHTNPlanInstance::GetNextPrimitiveStepsInCurrentPlan(TArray<FHTNPlanStepID>& OutStepIDs, const FHTNPlanStepID& StepID, bool bIsExecutingPlan) const
{
	if (!ensure(OwnerComponent) || !CurrentPlan.IsValid())
	{
		return 0;
	}

	FHTNGetNextStepsContext Context(*this, *CurrentPlan, bIsExecutingPlan, OutStepIDs);
	Context.AddNextPrimitiveStepsAfter(StepID);
	return Context.GetNumSubmittedSteps();
}

void UHTNPlanInstance::GetSubNodesInCurrentPlanToStart(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const
{
	if (!CurrentPlan.IsValid())
	{
		return;
	}

	if (!ensure(CurrentPlan->HasStep(StepID)))
	{
		return;
	}

	FHTNPlanStepID CurrentStepID = StepID;
	while (true)
	{
		FHTNPlanLevel& Level = *CurrentPlan->Levels[CurrentStepID.LevelIndex];
		FHTNPlanStep& Step = Level.Steps[CurrentStepID.StepIndex];
		if (!Step.SubNodesInfo.bSubNodesExecuting)
		{
			// Subnodes at the CurrentStep itself
			OutSubNodeGroups.Add(FHTNSubNodeGroup(Step.SubNodesInfo, CurrentStepID,
				Step.bIsIfNodeFalseBranch, Step.bCanConditionsInterruptTrueBranch, Step.bCanConditionsInterruptFalseBranch));

			if (!Level.RootSubNodesInfo.bSubNodesExecuting)
			{
				// Subnodes of the root of the level the CurrentStep is at
				OutSubNodeGroups.Emplace(Level.RootSubNodesInfo, Level.ParentStepID);

				if (CurrentStepID.LevelIndex > 0)
				{
					CurrentStepID = Level.ParentStepID;
					continue;
				}
			}
		}

		break;
	}
}

void UHTNPlanInstance::GetSubNodesInCurrentPlanToTick(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const
{
	if (!CurrentPlan.IsValid())
	{
		return;
	}

	if (!ensure(CurrentPlan->HasStep(StepID)))
	{
		return;
	}

	FHTNPlanStepID CurrentStepID = StepID;
	while (true)
	{
		FHTNPlanLevel& Level = *CurrentPlan->Levels[CurrentStepID.LevelIndex];
		FHTNPlanStep& Step = Level.Steps[CurrentStepID.StepIndex];
		if (Step.SubNodesInfo.LastFrameSubNodesTicked != GFrameCounter)
		{
			// Subnodes at the CurrentStep itself
			OutSubNodeGroups.Add(FHTNSubNodeGroup(Step.SubNodesInfo, CurrentStepID,
				Step.bIsIfNodeFalseBranch, Step.bCanConditionsInterruptTrueBranch, Step.bCanConditionsInterruptFalseBranch));

			if (Level.RootSubNodesInfo.LastFrameSubNodesTicked != GFrameCounter)
			{
				// Subnodes of the root of the level the CurrentStep is at
				OutSubNodeGroups.Emplace(Level.RootSubNodesInfo, Level.ParentStepID);

				if (CurrentStepID.LevelIndex > 0)
				{
					CurrentStepID = Level.ParentStepID;
					continue;
				}
			}
		}

		break;
	}
}

// This is expected to be called after PendingExecutionStepIDs is populated. 
// That is needed to determine which subnodes need to be stopped.
void UHTNPlanInstance::GetSubNodesInCurrentPlanToFinish(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const
{
	if (!CurrentPlan.IsValid())
	{
		return;
	}

	if (!ensure(CurrentPlan->HasStep(StepID)))
	{
		return;
	}

	FHTNPlanStepID CurrentStepID = StepID;
	while (true)
	{
		FHTNPlanLevel& Level = *CurrentPlan->Levels[CurrentStepID.LevelIndex];
		FHTNPlanStep& Step = Level.Steps[CurrentStepID.StepIndex];
		if (Step.SubNodesInfo.bSubNodesExecuting)
		{
			// Subnodes at the CurrentStep itself
			OutSubNodeGroups.Add(FHTNSubNodeGroup(Step.SubNodesInfo, CurrentStepID,
				Step.bIsIfNodeFalseBranch, Step.bCanConditionsInterruptTrueBranch, Step.bCanConditionsInterruptFalseBranch));

			if (ensure(Level.RootSubNodesInfo.bSubNodesExecuting))
			{
				// Check if there will not be any more steps in this level or its sublevels (recursively).
				const bool bLastStepInLevel = Algo::NoneOf(PendingExecutionStepIDs, [&](const FHTNPlanStepID& FutureStepID)
				{
					// Disregard steps pending execution that are before the current one (or are the current one).
					// That can happen in a looping secondary branch of a Parallel: 
					// in that case we should still finish the subnodes of that branch and then start them again.
					if (FutureStepID.LevelIndex == CurrentStepID.LevelIndex && FutureStepID.StepIndex <= CurrentStepID.StepIndex)
					{
						return false;
					}

					return CurrentPlan->HasStep(FutureStepID, CurrentStepID.LevelIndex);
				});
				if (bLastStepInLevel)
				{
					// Subnodes of the root of the level the CurrentStep is at
					OutSubNodeGroups.Emplace(Level.RootSubNodesInfo, Level.ParentStepID);

					if (CurrentStepID.LevelIndex > 0 && !AnyActiveSubLevelsExcept(CurrentStepID.LevelIndex))
					{
						CurrentStepID = Level.ParentStepID;
						continue;
					}
				}
			}
		}

		break;
	}
}

void UHTNPlanInstance::GetSubNodesInCurrentPlanToAbort(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const
{
	if (!CurrentPlan.IsValid())
	{
		return;
	}

	if (!ensure(CurrentPlan->HasStep(StepID)))
	{
		return;
	}

	FHTNPlanStepID CurrentStepID = StepID;
	while (true)
	{
		FHTNPlanLevel& Level = *CurrentPlan->Levels[CurrentStepID.LevelIndex];
		FHTNPlanStep& Step = Level.Steps[CurrentStepID.StepIndex];
		if (Step.SubNodesInfo.bSubNodesExecuting)
		{
			// Subnodes at the CurrentStep itself
			OutSubNodeGroups.Add(FHTNSubNodeGroup(Step.SubNodesInfo, CurrentStepID,
				Step.bIsIfNodeFalseBranch, Step.bCanConditionsInterruptTrueBranch, Step.bCanConditionsInterruptFalseBranch));
		}

		if (Level.RootSubNodesInfo.bSubNodesExecuting)
		{
			// Subnodes of the root of the level the CurrentStep is at
			OutSubNodeGroups.Emplace(Level.RootSubNodesInfo, Level.ParentStepID);
		}

		if (CurrentStepID.LevelIndex > 0 && !AnyActiveSubLevelsExcept(CurrentStepID.LevelIndex))
		{
			CurrentStepID = Level.ParentStepID;
			continue;
		}

		break;
	}
}

void UHTNPlanInstance::GetSubNodesInCurrentPlanAll(FHTNSubNodeGroups& OutSubNodeGroups, const FHTNPlanStepID& StepID) const
{
	if (!CurrentPlan.IsValid())
	{
		return;
	}

	if (!ensure(CurrentPlan->HasStep(StepID)))
	{
		return;
	}

	FHTNPlanStepID CurrentStepID = StepID;
	while (true)
	{
		FHTNPlanLevel& Level = *CurrentPlan->Levels[CurrentStepID.LevelIndex];
		FHTNPlanStep& Step = Level.Steps[CurrentStepID.StepIndex];

		// Subnodes at the CurrentStep itself
		OutSubNodeGroups.Add(FHTNSubNodeGroup(Step.SubNodesInfo, CurrentStepID,
			Step.bIsIfNodeFalseBranch, Step.bCanConditionsInterruptTrueBranch, Step.bCanConditionsInterruptFalseBranch));

		// Subnodes of the root of the level the CurrentStep is at
		OutSubNodeGroups.Emplace(Level.RootSubNodesInfo, Level.ParentStepID);

		if (CurrentStepID.LevelIndex > 0)
		{
			CurrentStepID = Level.ParentStepID;
			continue;
		}

		break;
	}
}

bool UHTNPlanInstance::AnyActiveSubLevelsExcept(int32 SubLevelIndex) const
{
	if (CurrentPlan.IsValid() && CurrentPlan->HasLevel(SubLevelIndex))
	{
		if (const FHTNPlanStep* const ParentStep = CurrentPlan->FindStep(CurrentPlan->Levels[SubLevelIndex]->ParentStepID))
		{
			if (SubLevelIndex != ParentStep->SubLevelIndex && IsLevelActive(ParentStep->SubLevelIndex))
			{
				return true;
			}

			if (SubLevelIndex != ParentStep->SecondarySubLevelIndex && IsLevelActive(ParentStep->SecondarySubLevelIndex))
			{
				return true;
			}
		}
	}

	return false;
}

bool UHTNPlanInstance::IsLevelActive(int32 LevelIndex) const
{
	if (CurrentPlan.IsValid() && CurrentPlan->HasLevel(LevelIndex))
	{
		const bool bExecuting = CurrentPlan->Levels[LevelIndex]->RootSubNodesInfo.bSubNodesExecuting;
		if (bExecuting)
		{
			return true;
		}

		const bool bWillExecute = Algo::AnyOf(PendingExecutionStepIDs, [&](const FHTNPlanStepID& StepID) { return CurrentPlan->HasStep(StepID, LevelIndex); });
		if (bWillExecute)
		{
			return true;
		}
	}

	return false;
}

FString UHTNPlanInstance::GetLogPrefix() const
{
	return FString::Printf(TEXT("(PI %d)"), ID.ID);
}

void UHTNPlanInstance::UpdateCurrentExecutionAndPlanning()
{
	if (Status != EHTNPlanInstanceStatus::InProgress)
	{
		UE_CVLOG_UELOG(HasPlan(), OwnerComponent, LogHTN, Error, 
			TEXT("%s Status is %s but HasPlan() is true!"), 
			*GetLogPrefix(), *UEnum::GetDisplayValueAsText(Status).ToString());
		UE_CVLOG_UELOG(IsPlanning(), OwnerComponent, LogHTN, Error, 
			TEXT("%s Status is %s but IsPlanning() is true!"), 
			*GetLogPrefix(), *UEnum::GetDisplayValueAsText(Status).ToString());
		UE_CVLOG_UELOG(GetPlanPendingExecution().IsValid(), OwnerComponent, LogHTN, Error,
			TEXT("%s Status is %s but there is a PlanPendingExecution!"), 
			*GetLogPrefix(), *UEnum::GetDisplayValueAsText(Status).ToString());
		return;
	}

	// Start pending execution if needed
	if (GetPlanPendingExecution().IsValid() && CanStartExecutingNewPlan())
	{
		const bool bManagedToStartExecuting = StartPendingPlanExecution();
		if (!bManagedToStartExecuting && Config.ShouldStopIfFailed())
		{
			Stop();
			return;
		}
	}

	// Replan if current plan fails recheck
	if (Status == EHTNPlanInstanceStatus::InProgress && HasActivePlan() && !IsPlanning() && !IsAbortingPlan() && !HasDeferredAbort())
	{
		if (!RecheckCurrentPlan())
		{
			UE_VLOG(this, LogHTN, Log, TEXT("%s plan recheck failed -> forcing replan."), *GetLogPrefix());
			Replan();
		}
	}
	
	if (Status == EHTNPlanInstanceStatus::InProgress)
	{
		// Start planning if needed
		if (bDeferredStartPlanningTask)
		{
			StartPlanningTask();
		}
		else if (!GetPlanPendingExecution().IsValid() && !HasActivePlan() && !IsAbortingPlan() && !HasDeferredAbort())
		{
			if (!IsPlanning())
			{
				StartPlanningTask();
			}
			// If the planning task didn't actually start when we called ReadyForActivation before, start it here.
			// This is possible when there are many nested planning tasks starting synchronously inside each-others "planning finished" handlers.
			// This can occur in a deeply nested stack of SubPlan nodes that each plan instantly. 
			// If we don't do this, the plan instance will be stuck forever waiting for the planning task.
			else if (CurrentPlanningTask->GetState() == EGameplayTaskState::AwaitingActivation)
			{
				CurrentPlanningTask->ReadyForActivation();
			}
		}
	}
}

void UHTNPlanInstance::ProcessDeferredActions()
{
	// Process deferred aborts etc.
	if (bDeferredAbortPlan)
	{
		AbortCurrentPlan();
	}

	// If the current plan was aborted in a deferred way and there's a new plan available, we should immediately begin executing the new plan.
	// This can happen (e.g.) if a replan was requested inside TickCurrentPlan and finished synchronously, and current plan had to be aborted in a deferred way as well.
	if (GetPlanPendingExecution().IsValid() && CanStartExecutingNewPlan())
	{
		const bool bManagedToStartExecuting = StartPendingPlanExecution();
		if (!bManagedToStartExecuting && Config.ShouldStopIfFailed())
		{
			Stop();
		}
	}
}

void UHTNPlanInstance::StartPlanningTask(bool bDeferToNextFrame)
{
	if (bDeferToNextFrame || (IsRootInstance() && OwnerComponent->IsStoppingHTN()))
	{
		bDeferredStartPlanningTask = true;
		return;
	}
	ON_SCOPE_EXIT { bDeferredStartPlanningTask = false; };

	CancelActivePlanning();

	if (ShouldReusePrePlannedPlan())
	{
		UE_VLOG(this, LogHTN, Verbose, TEXT("%s reusing pre-planned plan instead of planning a new one"), *GetLogPrefix());
		OnNewPlanProduced(Config.PrePlannedPlan);
	}
	else if (OwnerComponent && OwnerComponent->GetCurrentHTN() && OwnerComponent->GetAIOwner() && OwnerComponent->GetBlackboardComponent())
	{
		OwnerComponent->UpdateBlackboardState();
		check(OwnerComponent && OwnerComponent->GetCurrentHTN() && OwnerComponent->GetAIOwner() && OwnerComponent->GetBlackboardComponent());

		// Set up the planning task with an empty initial plan to start planning from.
		CurrentPlanningTask = OwnerComponent->MakePlanningTask();
		{
			UBlackboardComponent& BlackboardComponent = *OwnerComponent->GetBlackboardComponent();
			const TSharedRef<FBlackboardWorldState> WorldStateAtPlanStart = MakeShared<FBlackboardWorldState>(BlackboardComponent);
			UHTN* const HTNAsset = Config.RootNodeOverride ? Config.RootNodeOverride->GetSourceHTNAsset() : OwnerComponent->GetCurrentHTN();
			const TSharedRef<FHTNPlan> InitialPlan = MakeShared<FHTNPlan>(HTNAsset, WorldStateAtPlanStart, Config.RootNodeOverride);
			InitialPlan->RecursionCounts = Config.OuterPlanRecursionCounts;
			
			const EHTNPlanningType PlanningType = ActiveReplanParameters.IsSet() ? ActiveReplanParameters->PlanningType : EHTNPlanningType::Normal;

			CurrentPlanningTask->SetUp(*this, InitialPlan, PlanningType);
			CurrentPlanningTask->OnPlanningFinished.AddUObject(this, &ThisClass::OnPlanningTaskFinished);
		}
		UE_VLOG(this, LogHTN, Verbose, TEXT("%s starting planning task %s"), *GetLogPrefix(), *CurrentPlanningTask->GetName());

		CurrentPlanningTask->ReadyForActivation();
	}
}

void UHTNPlanInstance::OnPlanningTaskFinished(UAITask_MakeHTNPlan& Sender, TSharedPtr<FHTNPlan> ProducedPlan)
{
	if (!ensure(IsValid(OwnerComponent)))
	{
		return;
	}

	if (!ensure(&Sender == CurrentPlanningTask))
	{
		return;
	}

	const EHTNPlanningType PlanningType = Sender.GetPlanningType();
	const bool bWasCancelled = Sender.WasCancelled();
	Sender.Clear();
	CurrentPlanningTask = nullptr;

	// If we were merely trying to adjust the current plan, we should not stop the current plan (or really do anything else) if we failed to adjust it.
	if (PlanningType == EHTNPlanningType::TryToAdjustCurrentPlan && !ProducedPlan.IsValid())
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s could not adjust current plan, continuing with current plan"), *GetLogPrefix());
		ActiveReplanParameters.Reset();
		return;
	}

	if (bWasCancelled)
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s planning task was cancelled"), *GetLogPrefix());
		// Adjustments of the current plan that failed are always ignored.
		if (PlanningType != EHTNPlanningType::TryToAdjustCurrentPlan && Config.ShouldStopIfFailed())
		{
			Stop();
		}

		return;
	}

	if (Status != EHTNPlanInstanceStatus::InProgress)
	{
		UE_VLOG_UELOG(this, LogHTN, Log,
			TEXT("%s planning of plan instance finished while the plan instance status is %s. Ignoring."),
			*GetLogPrefix(),
			*UEnum::GetDisplayValueAsText(Status).ToString());
		return;
	}

	OnNewPlanProduced(ProducedPlan);
}

void UHTNPlanInstance::OnNewPlanProduced(TSharedPtr<FHTNPlan> ProducedPlan)
{
	if (HasPlan())
	{
		AbortCurrentPlan();

		// In case the AbortCurrentPlan caused the HTN component to be destroyed.
		// If that happens, we'll no longer receive Tick events, so we need to clean up now.
		// Only do this from the top-level plan instance to prevent duplicate calls to Cleanup.
		if (OwnerComponent->bDeferredCleanup && IsRootInstance())
		{
			OwnerComponent->Cleanup();
			return;
		}
	}

	if (ProducedPlan.IsValid())
	{
		INC_DWORD_STAT(STAT_AI_HTN_NumProducedPlans);
		PlanPendingExecution = ProducedPlan;

		if (CanStartExecutingNewPlan())
		{
			const bool bManagedToStartExecuting = StartPendingPlanExecution();
			if (!bManagedToStartExecuting && Config.ShouldStopIfFailed())
			{
				Stop();
			}
			else
			{
				ProcessDeferredActions();
			}
		}
	}
	else
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s failed to produce a new plan"), 
			*GetLogPrefix());
		if (Config.ShouldStopIfFailed())
		{
			Stop();
		}
	}
}

bool UHTNPlanInstance::StartPendingPlanExecution()
{
	++PlanExecutionCount;
	ActiveReplanParameters.Reset();

	// Prevent doing things like aborts, replans etc. from happening synchronously inside NotifyOnPlanExecutionStarted.
	FHTNScopedLock ScopedLock(LockFlags, EHTNLockFlags::LockTick);

	const TSharedPtr<FHTNPlan> PendingPlan = GetPlanPendingExecution();
	if (!ensure(PendingPlan.IsValid()))
	{
		return false;
	}

	if (!ensure(OwnerComponent->CurrentHTNAsset))
	{
		SetPlanPendingExecution(nullptr);
		return false;
	}

	check(!bCurrentPlanStartedExecution);
	check(!HasActiveTasks());
	check(CanStartExecutingNewPlan());

	UE_VLOG(this, LogHTN, Log, TEXT("%s attempting to start new plan with cost %d"), *GetLogPrefix(), PendingPlan->Cost);
	InitializeCurrentPlan(PendingPlan.ToSharedRef());
	check(CurrentPlan.IsValid());
	PlanPendingExecution.Reset();

	check(PendingExecutionStepIDs.Num() == 0);
	const bool bPlanHasTasks = GetNextPrimitiveStepsInCurrentPlan(PendingExecutionStepIDs, { 0, INDEX_NONE }) > 0;

	if (!RecheckCurrentPlan())
	{
		UE_VLOG_UELOG(this, LogHTN, Warning,
			TEXT("%s new plan failed plan recheck, so it can't be started."),
			*GetLogPrefix());
		ClearCurrentPlan();
		return false;
	}

	UE_VLOG(this, LogHTN, Log, TEXT("%s started executing plan"), *GetLogPrefix());
	bCurrentPlanStartedExecution = true;
	PlanExecutionStartedEvent.Broadcast(this);
	NotifyNodesOnPlanExecutionStarted();

	if (bPlanHasTasks)
	{
		// Start executing tasks (the way it does in TickCurrentPlan) immediately 
		// to avoid a one-frame gap between a plan being "started" and its tasks actually beginning execution.
		if (HasPlan() && !IsAbortingPlan() && !HasDeferredAbort())
		{
			StartTasksPendingExecution();
		}
	}
	else
	{
		UE_VLOG(this, LogHTN, Log,
			TEXT("%s new plan was degenerate, having no primitive tasks. This is possible when there are empty branches in the HTN or plans composed of an Optional node. Treating the plan as instantly succeeded."),
			*GetLogPrefix());
		OnPlanExecutionSuccessfullyFinished();
	}

	return true;
}

void UHTNPlanInstance::InitializeCurrentPlan(TSharedRef<FHTNPlan> NewPlan)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UHTNPlanInstance::InitializeCurrentPlan"), STAT_AI_HTN_InitializeCurrentPlan, STATGROUP_AI_HTN);

	if (NewPlan == CurrentPlan)
	{
		return;
	}

	if (CurrentPlan.IsValid())
	{
		ClearCurrentPlan();
	}
	check(!CurrentPlan.IsValid());
	check(PlanMemory.Num() == 0);
	check(NodeInstances.Num() == 0);

	// Extract subplans from under SubPlan nodes
	// that were configured to plan together with the plan that contains them.
	FHTNExtractedSubPlansMap ExtractedSubPlanMap;
	CurrentPlan = FHTNSubPlanExtractor(NewPlan, ExtractedSubPlanMap).Extract();
#if DO_CHECK
	CurrentPlan->VerifySubNodesAreInactive(OwnerComponent);
	for (const TPair<FHTNPlanStepID, TSharedPtr<FHTNPlan>>& Pair : ExtractedSubPlanMap)
	{
		if (const FHTNPlan* const ExtractedSubPlan = Pair.Value.Get())
		{
			ExtractedSubPlan->VerifySubNodesAreInactive(OwnerComponent);
		}
	}
#endif

	struct Local
	{
		// Round to 4 bytes
		static int32 GetAlignedDataSize(int32 Size) { return (Size + 3) & ~3; };
	};

	struct FNodeInitInfo
	{
		UHTNNode* NodeTemplate = nullptr;
		FHTNPlanStepID StepID;
		uint16 MemoryOffset = 0;
	};

	uint16 TotalNumBytesNeeded = 0;
	TArray<FNodeInitInfo, TInlineAllocator<256>> InitList;
	const auto RecordNode = [&](UHTNNode& NodeTemplate, uint16& OutMemoryOffset, const FHTNPlanStepID& StepID)
	{
		const uint16 SpecialDataSize = Local::GetAlignedDataSize(NodeTemplate.GetSpecialMemorySize());
		OutMemoryOffset = TotalNumBytesNeeded + SpecialDataSize;

		const uint16 TotalDataSize = Local::GetAlignedDataSize(SpecialDataSize + NodeTemplate.GetInstanceMemorySize());
		TotalNumBytesNeeded += TotalDataSize;

		InitList.Add({ &NodeTemplate, StepID, OutMemoryOffset });
	};

	// Decide where to put each node in the plan
	for (int32 LevelIndex = 0; LevelIndex < CurrentPlan->Levels.Num(); ++LevelIndex)
	{
		FHTNPlanLevel& Level = *CurrentPlan->Levels[LevelIndex];
		const FHTNPlanStepID RootNodesStepID { LevelIndex, INDEX_NONE };

		// Do root decorators
		const TArrayView<TObjectPtr<UHTNDecorator>> DecoratorTemplates = Level.GetRootDecoratorTemplates();
		Level.RootSubNodesInfo.DecoratorInfos.Reset(DecoratorTemplates.Num());
		for (UHTNDecorator* const Decorator : DecoratorTemplates)
		{
			if (Decorator)
			{
				RecordNode(*Decorator, Level.RootSubNodesInfo.DecoratorInfos.Add_GetRef({ Decorator, 0 }).NodeMemoryOffset, RootNodesStepID);
			}
		}

		// Do root services
		const TArrayView<TObjectPtr<UHTNService>> ServiceTemplates = Level.GetRootServiceTemplates();
		Level.RootSubNodesInfo.ServiceInfos.Reset(ServiceTemplates.Num());
		for (UHTNService* const Service : ServiceTemplates)
		{
			if (Service)
			{
				RecordNode(*Service, Level.RootSubNodesInfo.ServiceInfos.Add_GetRef({ Service, 0 }).NodeMemoryOffset, RootNodesStepID);
			}
		}

		// Do steps
		for (int32 StepIndex = 0; StepIndex < Level.Steps.Num(); ++StepIndex)
		{
			const FHTNPlanStepID StepID { LevelIndex, StepIndex };
			FHTNPlanStep& Step = Level.Steps[StepIndex];

			if (Step.Node.IsValid())
			{
				RecordNode(*Step.Node, Step.NodeMemoryOffset, StepID);
			}

			Step.SubNodesInfo.DecoratorInfos.Reset(Step.Node->Decorators.Num());
			for (UHTNDecorator* const Decorator : Step.Node->Decorators)
			{
				RecordNode(*Decorator, Step.SubNodesInfo.DecoratorInfos.Add_GetRef({ Decorator, 0 }).NodeMemoryOffset, StepID);
			}

			Step.SubNodesInfo.ServiceInfos.Reset(Step.Node->Services.Num());
			for (UHTNService* const Service : Step.Node->Services)
			{
				RecordNode(*Service, Step.SubNodesInfo.ServiceInfos.Add_GetRef({ Service, 0 }).NodeMemoryOffset, StepID);
			}
		}
	}

	UE_VLOG(this, LogHTN, Verbose, TEXT("%s initializing plan %p: %d nodes (%d instanced), %d bytes of memory"), 
		*GetLogPrefix(),
		CurrentPlan.Get(),
		InitList.Num(), 
		Algo::CountIf(InitList, [](const FNodeInitInfo& Info) { return Info.NodeTemplate->HasInstance(); }),
		TotalNumBytesNeeded);

	// Actually initialize the memory and create node instances where needed.
	PlanMemory.SetNumZeroed(TotalNumBytesNeeded);
	for (const FNodeInitInfo& NodeInitInfo : InitList)
	{
		uint8* const RawNodeMemory = PlanMemory.GetData() + NodeInitInfo.MemoryOffset;
		NodeInitInfo.NodeTemplate->InitializeFromAsset(*OwnerComponent->GetCurrentHTN());
		NodeInitInfo.NodeTemplate->InitializeInPlan(*OwnerComponent, RawNodeMemory, *CurrentPlan, NodeInitInfo.StepID, NodeInstances);

		// Initialize SubPlan tasks with subplans that were extracted for them (if any).
		if (UHTNTask_SubPlan* const SubPlanTask = Cast<UHTNTask_SubPlan>(NodeInitInfo.NodeTemplate))
		{
			const TSharedPtr<FHTNPlan> SubPlan = ExtractedSubPlanMap.FindRef(NodeInitInfo.StepID);
			SubPlanTask->InitializeSubPlanInstance(*OwnerComponent, RawNodeMemory, *CurrentPlan, SubPlan);
		}
	}
}

void UHTNPlanInstance::ClearCurrentPlan()
{
	ensureMsgf(!IsWaitingForAbortingTasks(), TEXT("UHTNPlanInstance::ClearCurrentPlan called when tasks are still aborting."));

	if (CurrentPlan.IsValid())
	{
		UE_VLOG(this, LogHTN, Verbose, TEXT("%s cleaning up plan %p"), 
			*GetLogPrefix(), 
			CurrentPlan.Get());

		CurrentPlan->VerifySubNodesAreInactive(OwnerComponent);

		const auto CleanupNode = [&](UHTNNode* NodeTemplate, uint16 MemoryOffset)
		{
			if (ensure(NodeTemplate))
			{
				NodeTemplate->CleanupInPlan(*OwnerComponent, GetNodeMemory(MemoryOffset));
			}
		};

		for (int32 LevelIndex = 0; LevelIndex < CurrentPlan->Levels.Num(); ++LevelIndex)
		{
			FHTNPlanLevel& Level = *CurrentPlan->Levels[LevelIndex];

			// Do root decorators
			for (THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Level.RootSubNodesInfo.DecoratorInfos)
			{
				CleanupNode(DecoratorInfo.TemplateNode, DecoratorInfo.NodeMemoryOffset);
			}

			// Do root services
			for (THTNNodeInfo<UHTNService>& ServiceInfo : Level.RootSubNodesInfo.ServiceInfos)
			{
				CleanupNode(ServiceInfo.TemplateNode, ServiceInfo.NodeMemoryOffset);
			}

			// Do steps
			for (FHTNPlanStep& Step : Level.Steps)
			{
				if (Step.Node.IsValid())
				{
					CleanupNode(Step.Node.Get(), Step.NodeMemoryOffset);
				}

				for (THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Step.SubNodesInfo.DecoratorInfos)
				{
					CleanupNode(DecoratorInfo.TemplateNode, DecoratorInfo.NodeMemoryOffset);
				}

				for (THTNNodeInfo<UHTNService>& ServiceInfo : Step.SubNodesInfo.ServiceInfos)
				{
					CleanupNode(ServiceInfo.TemplateNode, ServiceInfo.NodeMemoryOffset);
				}
			}
		}

		CurrentPlan.Reset();
		NodeInstances.Reset();
		PlanMemory.Reset(); 
	}

	CurrentlyExecutingStepIDs.Reset();
	PendingExecutionStepIDs.Reset();
	CurrentlyAbortingStepIDs.Reset();
	bCurrentPlanStartedExecution = false;

#if USE_HTN_DEBUGGER
	FHTNDebugExecutionStep& DebugStep = OwnerComponent->StoreDebugStep();
	check(DebugStep.PerPlanInstanceInfo.FindChecked(ID).ActivePlanStepIDs.Num() == 0);
	check(DebugStep.PerPlanInstanceInfo.FindChecked(ID).HTNPlan == nullptr);
#endif
}

bool UHTNPlanInstance::RecheckCurrentPlan(const FBlackboardWorldState* WorldStateOverride)
{
	if (!CurrentPlan.IsValid())
	{
		return true;
	}

	// Ensure that the worldstate proxy is restored to its current state at the end of this.
	FGuardWorldStateProxy GuardProxy(*OwnerComponent->GetPlanningWorldStateProxy());

	struct FRecheckContext
	{
		TSharedRef<FBlackboardWorldState> WorldState;
		FHTNPlanStepID StepID;
	};

	TArray<FRecheckContext> RecheckStack;

	// Set up the RecheckStack.
	TArray<FHTNPlanStepID, TInlineAllocator<8>> ActiveStepIDs;
	if (bCurrentPlanStartedExecution)
	{
		ActiveStepIDs.Append(CurrentlyExecutingStepIDs);
		ActiveStepIDs.Append(PendingExecutionStepIDs);
	}
	else
	{
		// We use a temporary buffer here to avoid having to do heap allocations in both branches of the if.
		// This is preferrable because this branch is rare.
		TArray<FHTNPlanStepID> Buffer;
		GetNextPrimitiveStepsInCurrentPlan(Buffer, { 0, INDEX_NONE }, /*bExecutingPlan=*/false);
		ActiveStepIDs = Buffer;
	}
	Algo::Transform(ActiveStepIDs, RecheckStack, [&](const FHTNPlanStepID& StepID) -> FRecheckContext
	{
		const TSharedRef<FBlackboardWorldState> WorldState = WorldStateOverride ? 
			WorldStateOverride->MakeNext() : 
			MakeShared<FBlackboardWorldState>(*OwnerComponent->GetBlackboardComponent());
		return { WorldState, StepID };
	});

	// Make sure that the step on the most primary branch is first, i.e. on the bottom of the stack.
	Algo::SortBy(RecheckStack, [&](const FRecheckContext& RecheckContext)
	{
		return CurrentPlan->IsSecondaryParallelStep(RecheckContext.StepID) ? 1 : 0;
	});

	TArray<FHTNPlanStepID> NextStepsBuffer;
	while (RecheckStack.Num())
	{
		const FRecheckContext CurrentContext = RecheckStack.Pop();
		OwnerComponent->SetPlanningWorldState(CurrentContext.WorldState, /*bIsEditable=*/false);

		// Apply changes caused by decorators entering
		TArray<FHTNPlanStepID, TInlineAllocator<16>> EnteredSteps{ CurrentContext.StepID };
		while (EnteredSteps.Top().StepIndex == 0 && EnteredSteps.Top().LevelIndex > 0)
		{
			EnteredSteps.Add(CurrentPlan->Levels[EnteredSteps.Top().LevelIndex]->ParentStepID);
		}
		for (int32 StepIndex = EnteredSteps.Num() - 1; StepIndex >= 0; --StepIndex)
		{
			const FHTNPlanStep& EnteredStep = CurrentPlan->GetStep(EnteredSteps[StepIndex]);
			EnteredStep.WorldStateAfterEnteringDecorators->ApplyChangedValues(*CurrentContext.WorldState);
		}

		const FHTNPlanStep& CurrentStep = CurrentPlan->GetStep(CurrentContext.StepID);
		UHTNTask* const Task = CastChecked<UHTNTask>(CurrentStep.Node);

		const bool bExecuting = CurrentlyExecutingStepIDs.Contains(CurrentContext.StepID);

		// Recheck the task itself
		if (!bExecuting && !Task->WrappedRecheckPlan(*OwnerComponent, GetNodeMemory(CurrentStep.NodeMemoryOffset), *CurrentContext.WorldState, CurrentStep))
		{
			UE_VLOG(this, LogHTN, Log,
				TEXT("%s plan recheck failed on task %s"),
				*GetLogPrefix(),
				*Task->GetShortDescription());
			return false;
		}

		// Apply changes caused by the task and by decorators exiting
		CurrentStep.WorldState->ApplyChangedValues(*CurrentContext.WorldState);

		// Recheck decorators
		if (!bExecuting && !UpdateSubNodes(CurrentContext.StepID, EHTNUpdateSubNodesFlags::CheckConditionsRecheck))
		{
			UE_VLOG(this, LogHTN, Log,
				TEXT("%s plan recheck failed because of subnodes active at task %s"),
				*GetLogPrefix(),
				*Task->GetShortDescription());
			return false;
		}

		NextStepsBuffer.Reset();
		if (GetNextPrimitiveStepsInCurrentPlan(NextStepsBuffer, CurrentContext.StepID, /*bIsExecutingPlan=*/false))
		{
			// We don't copy the first worldstate as an optimization, since the original won't be used after this anyway.
			RecheckStack.Push({ CurrentContext.WorldState, NextStepsBuffer[0] });
			for (int32 I = 1; I < NextStepsBuffer.Num(); ++I)
			{
				RecheckStack.Push({ CurrentContext.WorldState->MakeNext(), NextStepsBuffer[I] });
			}
		}
	}

	return true;
}

void UHTNPlanInstance::TickCurrentPlan(float DeltaTime)
{
	check(HasActivePlan());

	StartTasksPendingExecution();
	if (HasDeferredAbort())
	{
		return;
	}

	const auto UpdateAtPlanStep = [&](const FHTNPlanStepID& PlanStepID, uint8 UpdateFlags) -> bool
	{
		if (!UpdateSubNodes(PlanStepID, UpdateFlags, DeltaTime))
		{
			// if wasn't already aborted from within the decorators.
			if (HasActivePlan())
			{
				AbortCurrentPlan();
				return false;
			}
		}
		else
		{
			uint8* TaskMemory = nullptr;
			const UHTNTask& ExecutingTask = GetTaskInCurrentPlan(PlanStepID, TaskMemory);
			UE_VLOG(this, LogHTN, VeryVerbose, TEXT("%s ticking task %s."), *GetLogPrefix(), *ExecutingTask.GetShortDescription());
			ExecutingTask.WrappedTickTask(*OwnerComponent, TaskMemory, DeltaTime);
		}

		return !HasDeferredAbort() && HasActivePlan();
	};

	// Make a copy since items might get removed from the arrays during this.
	TArray<FHTNPlanStepID, TInlineAllocator<8>> ExecutingStepIDs(CurrentlyExecutingStepIDs);
	TArray<FHTNPlanStepID, TInlineAllocator<8>> AbortingStepIDs(CurrentlyAbortingStepIDs);
	for (const FHTNPlanStepID& ExecutingStepID : ExecutingStepIDs)
	{
		if (CurrentlyExecutingStepIDs.Contains(ExecutingStepID))
		{
			if (!UpdateAtPlanStep(ExecutingStepID, EHTNUpdateSubNodesFlags::Tick | EHTNUpdateSubNodesFlags::CheckConditionsExecution))
			{
				return;
			}
		}
	}

	for (const FHTNPlanStepID& AbortingStepID : AbortingStepIDs)
	{
		if (CurrentlyAbortingStepIDs.Contains(AbortingStepID))
		{
			if (!UpdateAtPlanStep(AbortingStepID, EHTNUpdateSubNodesFlags::Tick))
			{
				return;
			}
		}
	}
}

void UHTNPlanInstance::StartTasksPendingExecution()
{
	// Note that step ids might get added to PendingExecutionStepIDs during the loop 
	// if a task completes instantly (e.g.. Success, SetValue)
	TArray<FHTNPlanStepID, TInlineAllocator<8>> AlreadyStartedSteps;
	while (PendingExecutionStepIDs.Num() && !OwnerComponent->IsPaused() && !IsWaitingForAbortingTasks())
	{
		const FHTNPlanStepID AddedStepID = PendingExecutionStepIDs[0];

		// Stop if have to start the same step again
		// (such as when the secondary branch of a Parallel node is looping and completed instantly)
		if (AlreadyStartedSteps.Contains(AddedStepID))
		{
			break;
		}

		// PendingExecutionPlanStepIDs may be arbitrarily changed within this loop (i.e., in AbortSecondaryParallelBranchesIfNeeded).
		// To avoid errors, items are popped from the beginning of the array during iteration.
		PendingExecutionStepIDs.RemoveAt(0, 1, HTN_DISALLOW_SHRINKING);

		TArray<FHTNPlanStepID, TInlineAllocator<8>> EnteringStepIDs{ AddedStepID };
		while (EnteringStepIDs.Top().StepIndex == 0 && EnteringStepIDs.Top().LevelIndex > 0)
		{
			EnteringStepIDs.Add(CurrentPlan->Levels[EnteringStepIDs.Top().LevelIndex]->ParentStepID);
		}

#if USE_HTN_DEBUGGER
		// Store debug steps of entering the plan steps that contain this level
		// This makes it possible to trigger breakpoints on structural nodes when the first task in them begins
		// execution. For example, if there are a few nested scope nodes and a Task after them, then this allows 
		// breakpoints on those scope nodes to be triggered.
		for (int32 StepIndex = EnteringStepIDs.Num() - 1; StepIndex >= 0; --StepIndex)
		{
			FHTNDebugExecutionStep& DebugStep = OwnerComponent->StoreDebugStep();
			FHTNDebugExecutionPerPlanInstanceInfo& DebugInfoOfThisInstance = DebugStep.PerPlanInstanceInfo.FindChecked(ID);
			DebugInfoOfThisInstance.ActivePlanStepIDs.Add(EnteringStepIDs[StepIndex]);
		}
#endif
		const EHTNNodeResult Result = StartExecuteTask(AddedStepID);
		AlreadyStartedSteps.Add(AddedStepID);
	}
}

EHTNNodeResult UHTNPlanInstance::StartExecuteTask(const FHTNPlanStepID& PlanStepID)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_Execution);

	if (!ensure(HasPlan()))
	{
		return EHTNNodeResult::Failed;
	}

	const FHTNPlanStep& PlanStep = CurrentPlan->GetStep(PlanStepID);
	UHTNTask& Task = *CastChecked<UHTNTask>(PlanStep.Node);

	UE_VLOG(this, LogHTN, Verbose, TEXT("%s starting task %s"),
		*GetLogPrefix(),
		*Task.GetShortDescription());

	FHTNSubNodeGroups SubNodeGroups;
	GetSubNodesInCurrentPlanToStart(SubNodeGroups, PlanStepID);

	const bool bShouldApplyWorldstateChangesFromDecorators = !CurrentPlan->IsSecondaryParallelStep(PlanStepID);

	// In order of outermost to innermost subnodes:
	// check decorator condition, execution start of decorators, execution start of services.
	// if something fails along the way (condition check failure, ForceAbort etc.), call execution finish in reverse order on everything we started so far and abort the plan.
	// We do this group-by-group instead of subnode-by-subnode because there is only one FHTNPlanStep::WorldStateAfterEnteringDecorators, not an array of worldstates for each entered decorator.
	for (int32 GroupIndex = SubNodeGroups.Num() - 1; GroupIndex >= 0; --GroupIndex)
	{
		const FHTNSubNodeGroup& SubNodeGroup = SubNodeGroups[GroupIndex];
		if (CheckConditionsOfDecoratorsInGroup(SubNodeGroup, EHTNDecoratorConditionCheckType::Execution, PlanStepID))
		{
			// Apply changes to worldstate made during planning by decorators we're entering now.
			if (bShouldApplyWorldstateChangesFromDecorators)
			{
				if (const FHTNPlanStep* const EnteredStep = CurrentPlan->FindStep(SubNodeGroup.PlanStepID))
				{
					EnteredStep->WorldStateAfterEnteringDecorators->ApplyChangedValues(*OwnerComponent->GetBlackboardComponent());
				}
			}

			StartSubNodesInSubNodeGroup(SubNodeGroup);
		}
		else
		{
			UE_VLOG(this, LogHTN, Verbose, TEXT("%s could not start task %s due to decorator condition"),
				*GetLogPrefix(),
				*Task.GetShortDescription());

			if (HasPlan())
			{
				// If we can't start executing this task, finish the subnodes which we just started and then the
				// already active subnodes on nodes above this one (e.g., decorators on a Scope node right before this one).
				// Normally this would be done when finishing this task or one of its successors, but we failed to start the task, so we have to do that now.
				FinishSubNodesAtPlanStep(PlanStepID, EHTNNodeResult::Aborted);
				AbortCurrentPlan();
			}

			return EHTNNodeResult::Failed;
		}
	}

	check(!CurrentlyExecutingStepIDs.Contains(PlanStepID));
	CurrentlyExecutingStepIDs.Add(PlanStepID);
#if USE_HTN_DEBUGGER
	OwnerComponent->StoreDebugStep();
#endif

	// Start the task itself.
	uint8* const TaskMemory = GetNodeMemory(PlanStep.NodeMemoryOffset);
	const EHTNNodeResult Result = Task.WrappedExecuteTask(*OwnerComponent, TaskMemory, PlanStepID);
	if (Result != EHTNNodeResult::InProgress && CurrentlyExecutingStepIDs.Contains(PlanStepID))
	{
		OnTaskFinished(&Task, TaskMemory, PlanStepID, Result);
	}
	return Result;
}

bool UHTNPlanInstance::UpdateSubNodes(const FHTNPlanStepID& PlanStepID, uint8 UpdateFlags, float DeltaTime)
{
	const bool bTick = UpdateFlags & EHTNUpdateSubNodesFlags::Tick;
	const bool bCheckConditionsExecution = UpdateFlags & EHTNUpdateSubNodesFlags::CheckConditionsExecution;
	const bool bCheckConditionsRecheck = UpdateFlags & EHTNUpdateSubNodesFlags::CheckConditionsRecheck;
	const bool bCheckConditions = bCheckConditionsExecution || bCheckConditionsRecheck;

	const auto TickSubNode = [&](const auto& SubNodeInfo) -> bool
	{
		const auto* const TemplateNode = SubNodeInfo.TemplateNode;
		if (ensure(TemplateNode))
		{
			TemplateNode->WrappedTickNode(*OwnerComponent, GetNodeMemory(SubNodeInfo.NodeMemoryOffset), DeltaTime);
			if (HasDeferredAbort())
			{
				return false;
			}

			if (!HasActivePlan())
			{
				UE_VLOG_UELOG(this, LogHTN, Log, 
					TEXT("%s ticking subnode %s removed the current plan, skipping remaining node ticks."), 
					*GetLogPrefix(), IsValid(TemplateNode) ? *TemplateNode->GetShortDescription() : TEXT("[Missing]"));
				return false;
			}
		}

		return true;
	};

	// Update the subnode groups in order or execution (outermost first, from root decorators of the top level down to the subnodes of the current task).
	FHTNSubNodeGroups SubNodeGroups;
	GetSubNodesInCurrentPlanToTick(SubNodeGroups, PlanStepID);

	for (int32 GroupIndex = SubNodeGroups.Num() - 1; GroupIndex >= 0; --GroupIndex)
	{
		const FHTNSubNodeGroup& Group = SubNodeGroups[GroupIndex];

		// When we want to tick subnodes and call GetSubNodesInCurrentPlanToTick on a plan step that's currently executing,
		// the resulting groups all have bSubNodesExecuting == true. If a group  had bSubNodesExecuting disabled 
		// during this loop, that means plan execution has advanced from the current task and so the subnodes in this group 
		// and all groups below are no longer active, so we can stop here. This can happen in the following scenario (for example):
		// 
		// A decorator's Tick moved the destination of a MoveTo task closer, 
		// making it instantly succeed and so the plan advances to the next task inside the call to TickSubNode, 
		// which changes which subnodes should be ticked.
		if (bTick && !Group.SubNodesInfo->bSubNodesExecuting)
		{
			return true;
		}
		const bool bTickGroup = bTick && ensure(Group.SubNodesInfo->LastFrameSubNodesTicked != GFrameCounter);

		if (bTickGroup)
		{
			Group.SubNodesInfo->LastFrameSubNodesTicked = GFrameCounter;

			for (const THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Group.SubNodesInfo->DecoratorInfos)
			{
				if (!TickSubNode(DecoratorInfo))
				{
					return false;
				}
				
				// If TickSubNode made the plan advance to the next task and this group is no longer active, 
				// we can stop here and not tick or check conditions on the remaining subnodes in this group
				// or any group deeper in the plan.
				if (!Group.SubNodesInfo->bSubNodesExecuting)
				{
					return true;
				}
			}
		}

		if (bCheckConditions)
		{
			const EHTNDecoratorConditionCheckType CheckType = bCheckConditionsRecheck ?
				EHTNDecoratorConditionCheckType::PlanRecheck : EHTNDecoratorConditionCheckType::Execution;
			if (!CheckConditionsOfDecoratorsInGroup(Group, CheckType, PlanStepID))
			{
				return false;
			}
		}

		if (bTickGroup)
		{
			for (const THTNNodeInfo<UHTNService>& ServiceInfo : Group.SubNodesInfo->ServiceInfos)
			{
				if (!TickSubNode(ServiceInfo))
				{
					return false;
				}

				// If TickSubNode made the plan advance to the next task and this group is no longer active, 
				// we can stop here and not tick any remaining services in this group or remaining subnodes 
				// in any group deeper in the plan.
				if (!Group.SubNodesInfo->bSubNodesExecuting)
				{
					return true;
				}
			}
		}
	}

	return true;
}

bool UHTNPlanInstance::CheckConditionsOfDecoratorsInGroup(const FHTNSubNodeGroup& Group, EHTNDecoratorConditionCheckType CheckType, const FHTNPlanStepID& CheckedPlanStepID)
{
	if (!ensure(CurrentPlan.IsValid()))
	{
		return false;
	}

	if (!Group.CanDecoratorConditionsInterruptPlan())
	{
		return true;
	}

	bool bSkippedAtLeastOneDecorator = false;
	for (const THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Group.SubNodesInfo->DecoratorInfos)
	{
		UHTNDecorator* const DecoratorTemplate = DecoratorInfo.TemplateNode;
		if (!ensure(DecoratorTemplate))
		{
			continue;
		}

		uint8* const NodeMemory = GetNodeMemory(DecoratorInfo.NodeMemoryOffset);
		const EHTNDecoratorTestResult Result = DecoratorTemplate->WrappedTestCondition(*OwnerComponent, NodeMemory, CheckType);
		bSkippedAtLeastOneDecorator |= Result == EHTNDecoratorTestResult::NotTested;
		if (HasDeferredAbort())
		{
			return false;
		}

		if (Result == EHTNDecoratorTestResult::Failed)
		{
			if (!Group.bIsIfNodeFalseBranch && Group.bCanConditionsInterruptTrueBranch)
			{
				UE_VLOG_UELOG(this, LogHTN, Log, TEXT("%s %s test of node '%s' failed when checking decorator '%s' of node '%s'"),
					*GetLogPrefix(),
					*UEnum::GetDisplayValueAsText(CheckType).ToString(),
					*CurrentPlan->GetStep(CheckedPlanStepID).Node->GetShortDescription(),
					*DecoratorTemplate->GetShortDescription(),
					CurrentPlan->HasStep(Group.PlanStepID) ? *CurrentPlan->GetStep(Group.PlanStepID).Node->GetShortDescription() : TEXT("root"));

				return false;
			}
			else if (Group.bIsIfNodeFalseBranch)
			{
				return true;
			}
		}
	}

	if (!bSkippedAtLeastOneDecorator && Group.bIsIfNodeFalseBranch && Group.bCanConditionsInterruptFalseBranch)
	{
		UE_VLOG_UELOG(this, LogHTN, Log, TEXT("%s %s test of node '%s' failed because all of the decorators of node '%s' succeeded while at least one of them should've failed."),
			*GetLogPrefix(),
			*UEnum::GetDisplayValueAsText(CheckType).ToString(),
			*CurrentPlan->GetStep(CheckedPlanStepID).Node->GetShortDescription(),
			CurrentPlan->HasStep(Group.PlanStepID) ? *CurrentPlan->GetStep(Group.PlanStepID).Node->GetShortDescription() : TEXT("root"));
		return false;
	}

	return true;
}

void UHTNPlanInstance::StartSubNodesInSubNodeGroup(const FHTNSubNodeGroup& SubNodeGroup)
{
	const auto StartExecution = [this](const auto& SubNodeInfo)
	{
		if (ensure(SubNodeInfo.TemplateNode))
		{
			SubNodeInfo.TemplateNode->WrappedExecutionStart(*OwnerComponent, GetNodeMemory(SubNodeInfo.NodeMemoryOffset));
		}
	};

	FHTNRuntimeSubNodesInfo& SubNodesInfo = *SubNodeGroup.SubNodesInfo;

	if (!ensure(!SubNodesInfo.bSubNodesExecuting))
	{
		return;
	}
	SubNodesInfo.bSubNodesExecuting = true;

	// Start decorators in the group
	for (const THTNNodeInfo<UHTNDecorator>& DecoratorInfo : SubNodesInfo.DecoratorInfos)
	{
		StartExecution(DecoratorInfo);
	}

	// Start services in the group.
	for (const THTNNodeInfo<UHTNService>& ServiceInfo : SubNodesInfo.ServiceInfos)
	{
		StartExecution(ServiceInfo);
	}
}

void UHTNPlanInstance::FinishSubNodesAtPlanStep(const FHTNPlanStepID& PlanStepID, EHTNNodeResult Result)
{
	FHTNSubNodeGroups SubNodeGroups;
	switch (Result)
	{
		case EHTNNodeResult::Succeeded:
			GetSubNodesInCurrentPlanToFinish(SubNodeGroups, PlanStepID);
			break;
		case EHTNNodeResult::Failed:
		case EHTNNodeResult::Aborted:
			GetSubNodesInCurrentPlanToAbort(SubNodeGroups, PlanStepID);
			break;
		case EHTNNodeResult::InProgress:
			checkNoEntry();
			break;
		default:
			checkNoEntry();
			break;
	}

	// Innermost to outermost subnodes
	for (const FHTNSubNodeGroup& SubNodeGroup : SubNodeGroups)
	{
		FinishSubNodesInSubNodeGroup(SubNodeGroup, Result);
	}
}

void UHTNPlanInstance::FinishSubNodesInSubNodeGroup(const FHTNSubNodeGroup& SubNodeGroup, EHTNNodeResult Result)
{
	const auto FinishExecution = [&](const auto& SubNodeInfo)
	{
		if (ensure(SubNodeInfo.TemplateNode))
		{
			SubNodeInfo.TemplateNode->WrappedExecutionFinish(*OwnerComponent, GetNodeMemory(SubNodeInfo.NodeMemoryOffset), Result);
		}
	};

	FHTNRuntimeSubNodesInfo& SubNodesInfo = *SubNodeGroup.SubNodesInfo;

	if (!ensure(SubNodesInfo.bSubNodesExecuting))
	{
		return;
	}
	SubNodesInfo.bSubNodesExecuting = false;
	SubNodesInfo.LastFrameSubNodesTicked.Reset();

	const TArray<THTNNodeInfo<UHTNService>>& ServiceGroup = SubNodesInfo.ServiceInfos;
	for (int32 I = ServiceGroup.Num() - 1; I >= 0; --I)
	{
		FinishExecution(ServiceGroup[I]);
	}

	const TArray<THTNNodeInfo<UHTNDecorator>>& DecoratorGroup = SubNodesInfo.DecoratorInfos;
	for (int32 I = DecoratorGroup.Num() - 1; I >= 0; --I)
	{
		FinishExecution(DecoratorGroup[I]);
	}
}

UHTNTask& UHTNPlanInstance::GetTaskInCurrentPlan(const FHTNPlanStepID& ExecutingStepID) const
{
	check(HasActivePlan());

	const FHTNPlanStep& ExecutingStep = CurrentPlan->GetStep(ExecutingStepID);
	UHTNTask* const ExecutingTask = CastChecked<UHTNTask>(ExecutingStep.Node);

	check(ExecutingTask);
	return *ExecutingTask;
}

UHTNTask& UHTNPlanInstance::GetTaskInCurrentPlan(const FHTNPlanStepID& ExecutingStepID, uint8*& OutTaskMemory) const
{
	check(HasActivePlan());

	const FHTNPlanStep& ExecutingStep = CurrentPlan->GetStep(ExecutingStepID);
	UHTNTask* const ExecutingTask = CastChecked<UHTNTask>(ExecutingStep.Node);

	OutTaskMemory = GetNodeMemory(ExecutingStep.NodeMemoryOffset);

	check(ExecutingTask);
	return *ExecutingTask;
}

void UHTNPlanInstance::RemovePendingExecutionPlanSteps()
{
	for (int32 I = PendingExecutionStepIDs.Num() - 1; I >= 0; --I)
	{
		const FHTNPlanStepID StepID = PendingExecutionStepIDs[I];
		// We need to remove as we iterate to that steps that we already handles 
		// don't prevent FinishSubNodesAtPlanStep from finishing the subnodes they might be holding active.
		PendingExecutionStepIDs.RemoveAt(I, 1, HTN_DISALLOW_SHRINKING);
		FinishSubNodesAtPlanStep(StepID, EHTNNodeResult::Aborted);
	}
}

void UHTNPlanInstance::RemovePendingExecutionPlanSteps(TFunctionRef<bool(const FHTNPlanStepID&)> Predicate)
{
	for (int32 I = PendingExecutionStepIDs.Num() - 1; I >= 0; --I)
	{
		const FHTNPlanStepID StepID = PendingExecutionStepIDs[I];
		if (Predicate(StepID))
		{
			PendingExecutionStepIDs.RemoveAt(I, 1, HTN_DISALLOW_SHRINKING);
			FinishSubNodesAtPlanStep(StepID, EHTNNodeResult::Aborted);
		}
	}
}

void UHTNPlanInstance::AbortExecutingPlanStep(FHTNPlanStepID PlanStepID)
{
	ensure(!CurrentlyAbortingStepIDs.Contains(PlanStepID));

	uint8* TaskMemory = nullptr;
	UHTNTask& Task = GetTaskInCurrentPlan(PlanStepID, TaskMemory);

	UE_VLOG(this, LogHTN, Log, TEXT("%s aborting task %s"),
		*GetLogPrefix(),
		*Task.GetShortDescription());

	CurrentlyExecutingStepIDs.RemoveSingle(PlanStepID);
	CurrentlyAbortingStepIDs.Add(PlanStepID);

	const EHTNNodeResult Result = Task.WrappedAbortTask(*OwnerComponent, TaskMemory);
	UE_CVLOG_UELOG(Result != EHTNNodeResult::Aborted && Result != EHTNNodeResult::InProgress, 
		OwnerComponent, LogHTN, Error,
		TEXT("%s unexpected EHTNNodeResult returned from AbortTask of %s. Expected Aborted or InProgress, instead got %s"),
		*GetLogPrefix(),
		*Task.GetShortDescription(), 
		*UEnum::GetValueAsString(Result));

	if (Result == EHTNNodeResult::Aborted)
	{
		OnTaskFinished(&Task, TaskMemory, PlanStepID, Result);
	}
	else
	{
		UE_VLOG(this, LogHTN, Verbose, 
			TEXT("%s task %s indicated latent abort, will wait until abort is complete"), 
			*GetLogPrefix(),
			*Task.GetShortDescription());
	}
}

void UHTNPlanInstance::OnPlanAbortFinished()
{
	check(IsAbortingPlan());
	check(!IsWaitingForAbortingTasks());

	Status = GetStatusAfterFailed();
	const bool bInstanceFinished = Status != EHTNPlanInstanceStatus::InProgress;

	UE_VLOG(this, LogHTN, Log, TEXT("%s finished aborting plan"), *GetLogPrefix());

	{
		// Prevent NotifyPlanInstanceFinished from being done from inside here.
		FGuardValue_Bitfield(bBlockNotifyPlanInstanceFinished, true);

		NotifyNodesOnPlanExecutionFinished(EHTNPlanExecutionFinishedResult::FailedOrAborted);
		ClearCurrentPlan();

		bAbortingPlan = false;
		if (bInstanceFinished)
		{
			SetPlanPendingExecution(nullptr);
		}

		PlanExecutionFinishedEvent.Broadcast(this, EHTNPlanExecutionFinishedResult::FailedOrAborted);
	}

	if (bInstanceFinished)
	{
		NotifyPlanInstanceFinished();
	}
}

void UHTNPlanInstance::OnPlanExecutionSuccessfullyFinished()
{
	check(!HasActiveTasks());

	Status = GetStatusAfterSucceeded();
	const bool bInstanceFinished = Status != EHTNPlanInstanceStatus::InProgress;

	UE_VLOG(this, LogHTN, Log, TEXT("%s finished executing plan successfully"), *GetLogPrefix());

	{
		// Prevent NotifyPlanInstanceFinished from being done from inside here.
		FGuardValue_Bitfield(bBlockNotifyPlanInstanceFinished, true);

		NotifyNodesOnPlanExecutionFinished(EHTNPlanExecutionFinishedResult::Succeeded);
		ClearCurrentPlan();

		if (bInstanceFinished)
		{
			SetPlanPendingExecution(nullptr);
		}

		PlanExecutionFinishedEvent.Broadcast(this, EHTNPlanExecutionFinishedResult::Succeeded);
	}

	if (bInstanceFinished)
	{
		NotifyPlanInstanceFinished();
	}
}

void UHTNPlanInstance::NotifyPlanInstanceFinished()
{
	if (!bBlockNotifyPlanInstanceFinished)
	{
		UE_VLOG(this, LogHTN, Log, TEXT("%s plan instance finished, status: %s"), 
			*GetLogPrefix(), *UEnum::GetDisplayValueAsText(Status).ToString());
		PlanInstanceFinishedEvent.Broadcast(this);
	}
}

void UHTNPlanInstance::NotifyNodesOnPlanExecutionStarted()
{
	NotifyNodesOnPlanExecutionHelper([this](UHTNNode* TemplateNode, uint16 NodeMemoryOffset)
	{
		TemplateNode->WrappedOnPlanExecutionStarted(*OwnerComponent, GetNodeMemory(NodeMemoryOffset));
	});
}

void UHTNPlanInstance::NotifyNodesOnPlanExecutionFinished(EHTNPlanExecutionFinishedResult Result)
{
	NotifyNodesOnPlanExecutionHelper([this, Result](UHTNNode* TemplateNode, uint16 NodeMemoryOffset)
	{
		TemplateNode->WrappedOnPlanExecutionFinished(*OwnerComponent, GetNodeMemory(NodeMemoryOffset), Result);
	});
}

void UHTNPlanInstance::NotifyNodesOnPlanExecutionHelper(TFunctionRef<void(UHTNNode* /*TemplateNode*/, uint16 /*NodeMemoryOffset*/)> Callable)
{
	FGuardWorldStateProxy GuardProxy(*OwnerComponent->GetPlanningWorldStateProxy());

	for (int32 LevelIndex = 0; LevelIndex < CurrentPlan->Levels.Num(); ++LevelIndex)
	{
		FHTNPlanLevel& Level = *CurrentPlan->Levels[LevelIndex];

		// Root subnodes
		{
			OwnerComponent->SetPlanningWorldState(Level.WorldStateAtLevelStart, /*bIsEditable=*/false);

			// Do root decorators
			for (THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Level.RootSubNodesInfo.DecoratorInfos)
			{
				Callable(DecoratorInfo.TemplateNode, DecoratorInfo.NodeMemoryOffset);
			}

			// Do root services
			for (THTNNodeInfo<UHTNService>& ServiceInfo : Level.RootSubNodesInfo.ServiceInfos)
			{
				Callable(ServiceInfo.TemplateNode, ServiceInfo.NodeMemoryOffset);
			}
		}

		// Do steps
		for (const FHTNPlanStep& Step : Level.Steps)
		{
			OwnerComponent->SetPlanningWorldState(Step.WorldState, /*bIsEditable=*/false);

			if (Step.Node.IsValid())
			{
				Callable(Step.Node.Get(), Step.NodeMemoryOffset);
			}

			for (const THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Step.SubNodesInfo.DecoratorInfos)
			{
				Callable(DecoratorInfo.TemplateNode, DecoratorInfo.NodeMemoryOffset);
			}

			for (const THTNNodeInfo<UHTNService>& ServiceInfo : Step.SubNodesInfo.ServiceInfos)
			{
				Callable(ServiceInfo.TemplateNode, ServiceInfo.NodeMemoryOffset);
			}
		}
	}
}

#if ENABLE_VISUAL_LOG

// VLOG the current plan, showing where the character will be at each step of the plan.
void UHTNPlanInstance::VisualizeCurrentPlan() const
{
	if (!FVisualLogger::IsRecording())
	{
		return;
	}

	if (!CurrentPlan.IsValid())
	{
		return;
	}

	if (!ensure(OwnerComponent->GetBlackboardComponent()))
	{
		return;
	}

	CurrentPlan->CheckIntegrity();

	// Make sure that the worldstate proxy is restored to its current state at the end of this.
	FGuardWorldStateProxy GuardProxy(*OwnerComponent->GetPlanningWorldStateProxy());

	FVector Location = bCurrentPlanStartedExecution ?
		OwnerComponent->GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(FBlackboard::KeySelfLocation) :
		CurrentPlan->Levels[0]->WorldStateAtLevelStart->GetValue<UBlackboardKeyType_Vector>(FBlackboard::KeySelfLocation);
	FString LocationDescription;
	const auto LogLocation = [&]()
	{
		UE_VLOG_LOCATION(this, LogHTNCurrentPlan, Verbose, Location, 10.0f, FColor::Blue, TEXT("%s"), *LocationDescription);
		LocationDescription.Reset();
	};

	TArray<FHTNPlanStepID, TInlineAllocator<8>> ActiveStepIDs;
	if (bCurrentPlanStartedExecution)
	{
		ActiveStepIDs = CurrentlyExecutingStepIDs;
	}
	else
	{
		// We use a temporary buffer here to avoid having to do heap allocations in both branches of the if.
		// This is preferrable because this branch is rare.
		TArray<FHTNPlanStepID> Buffer;
		GetNextPrimitiveStepsInCurrentPlan(Buffer, { 0, INDEX_NONE }, /*bExecutingPlan=*/false);
		ActiveStepIDs = Buffer;
	}
	const FHTNPlanStepID* const PrimaryStepID = ActiveStepIDs.FindByPredicate([this](const FHTNPlanStepID& StepID) -> bool
	{
		return !CurrentPlan->IsSecondaryParallelStep(StepID);
	});
	if (!PrimaryStepID)
	{
		return;
	}

	FHTNPlanStepID PlanStepID = *PrimaryStepID;
	TArray<FHTNPlanStepID> NextStepsBuffer;
	while (true)
	{
		const FHTNPlanStep& PlanStep = CurrentPlan->GetStep(PlanStepID);
		UHTNTask* const Task = Cast<UHTNTask>(PlanStep.Node);
		if (Task)
		{
			OwnerComponent->SetPlanningWorldState(PlanStep.WorldState, /*bIsEditable=*/false);
			Task->WrappedLogToVisualLog(*OwnerComponent, GetNodeMemory(PlanStep.NodeMemoryOffset), PlanStep);
		}
		const bool bShowTaskName = Task && Task->bShowTaskNameOnCurrentPlanVisualization;

		const FVector NextLocation = PlanStep.WorldState->GetValue<UBlackboardKeyType_Vector>(FBlackboard::KeySelfLocation);
		// We skip drawing the line from here to there if the task node itself has sublevels
		// This can happen if the task is an HTNTask_SubNode whose subplan was made as part of the outer plan and then extracted.
		// This prevents drawing an extra line from where the character will be before the subplan to where it will be after.
		// That is not needed because the changes to the worldstate made by the subplan will be drawn by the UHTNPlanInstance of that subplan.
		if (PlanStep.SubLevelIndex == INDEX_NONE && PlanStep.SecondarySubLevelIndex == INDEX_NONE)
		{
			if (FAISystem::IsValidLocation(Location) && FAISystem::IsValidLocation(NextLocation) && !FVector::PointsAreNear(Location, NextLocation, KINDA_SMALL_NUMBER))
			{
				LogLocation();
				UE_VLOG_SEGMENT_THICK(this, LogHTNCurrentPlan, Verbose, Location, NextLocation, FColor::Blue, 5.0f, TEXT("%s"), 
					bShowTaskName ? *PlanStep.Node->GetNodeName() : TEXT(""));
			}
			else if (bShowTaskName)
			{
				LocationDescription += PlanStep.Node->GetNodeName();
				LocationDescription += TEXT("\n");
			}
		}
		Location = NextLocation;

		NextStepsBuffer.Reset();
		if (GetNextPrimitiveStepsInCurrentPlan(NextStepsBuffer, PlanStepID, /*bIsExecutingPlan=*/false))
		{
			// Only logging the primary branch, no secondaries.
			PlanStepID = NextStepsBuffer[0];
		}
		else
		{
			break;
		}
	}

	if (LocationDescription.Len() > 0)
	{
		LogLocation();
	}
}

void UHTNPlanInstance::DescribeActivePlanStepToVisLog(const FHTNPlanStepID& StepID, FVisualLogStatusCategory& LogCategory) const
{
	if (!HasPlan())
	{
		return;
	}

	const FHTNPlanStep* const Step = CurrentPlan->FindStep(StepID);
	if (!Step || !Step->Node.IsValid())
	{
		return;
	}

	const FString StepCategoryName = FString::Printf(TEXT("%s%s"),
		IsRootInstance() ? TEXT("") : *(GetLogPrefix() + TEXT(" ")),
		Step->Node.IsValid() ? *Step->Node->GetShortDescription() : TEXT("invalid"));
	FVisualLogStatusCategory& StepCategory = LogCategory.Children.Emplace_GetRef(StepCategoryName);

	// Describe all active subnodes at this step, from the outside in. Include which node they're on.
	FHTNSubNodeGroups SubNodeGroups;
	GetSubNodesInCurrentPlanAll(SubNodeGroups, StepID);
	for (int32 GroupIndex = SubNodeGroups.Num() - 1; GroupIndex >= 0; --GroupIndex)
	{
		const FHTNSubNodeGroup& Group = SubNodeGroups[GroupIndex];

		FString SubNodeStepName(TEXT("root"));
		if (const FHTNPlanStep* const SubNodeStep = CurrentPlan->FindStep(Group.PlanStepID))
		{
			SubNodeStepName = SubNodeStep->Node.IsValid() ? SubNodeStep->Node->GetShortDescription() : TEXT("invalid");
		}

		for (const THTNNodeInfo<UHTNDecorator>& DecoratorInfo : Group.SubNodesInfo->DecoratorInfos)
		{
			StepCategory.Add(DecoratorInfo.TemplateNode ? DecoratorInfo.TemplateNode->GetNodeName() : TEXT("invalid"), SubNodeStepName);
		}

		for (const THTNNodeInfo<UHTNService>& ServiceInfo : Group.SubNodesInfo->ServiceInfos)
		{
			StepCategory.Add(ServiceInfo.TemplateNode ? ServiceInfo.TemplateNode->GetNodeName() : TEXT("invalid"), SubNodeStepName);
		}
	}
}

#endif

EHTNPlanInstanceStatus UHTNPlanInstance::GetStatusAfterSucceeded() const
{
	EHTNPlanInstanceStatus NewStatus = FinishReactionToPlanInstanceStatus(Config.SucceededReaction);
	if (!CanLoop())
	{
		if (NewStatus == EHTNPlanInstanceStatus::InProgress)
		{
			NewStatus = EHTNPlanInstanceStatus::Succeeded;
		}
	}
	else if (NewStatus != EHTNPlanInstanceStatus::InProgress && ActiveReplanParameters.IsSet() && ActiveReplanParameters->bForceReplan)
	{
		NewStatus = EHTNPlanInstanceStatus::InProgress;
	}

	return NewStatus;
}

EHTNPlanInstanceStatus UHTNPlanInstance::GetStatusAfterFailed() const
{
	EHTNPlanInstanceStatus NewStatus = FinishReactionToPlanInstanceStatus(Config.FailedReaction);
	if (!CanLoop())
	{
		if (NewStatus == EHTNPlanInstanceStatus::InProgress)
		{
			NewStatus = EHTNPlanInstanceStatus::Failed;
		}
	}
	else if (NewStatus != EHTNPlanInstanceStatus::InProgress && ActiveReplanParameters.IsSet() && ActiveReplanParameters->bForceReplan)
	{
		NewStatus = EHTNPlanInstanceStatus::InProgress;
	}

	return NewStatus;
}

bool UHTNPlanInstance::CanLoop() const
{
	if (IsRootInstance() && OwnerComponent->IsStoppingHTN())
	{
		return false;
	}

	if (Config.CanPlanInstanceLoopDelegate.IsBound() && !Config.CanPlanInstanceLoopDelegate.Execute(this))
	{
		return false;
	}

	return true;
}

// Decide if we're reusing an existing plan (e.g. the one made during planning) or make a new one.
bool UHTNPlanInstance::ShouldReusePrePlannedPlan() const
{
	const bool bAllowedToReusePlan = !ActiveReplanParameters.IsSet() || !ActiveReplanParameters->bMakeNewPlanRegardlessOfSubPlanSettings;
	const bool bSkipPlanningDuringThisExecution = !Config.bPlanDuringExecution || (PlanExecutionCount == 0 && Config.bSkipPlanningOnFirstExecutionIfPlanAlreadyAvailable);
	const bool bReusePlan = bAllowedToReusePlan && bSkipPlanningDuringThisExecution && Config.PrePlannedPlan.IsValid();
	UE_CVLOG_UELOG(bAllowedToReusePlan && !Config.bPlanDuringExecution && !Config.PrePlannedPlan.IsValid(), this, LogHTN, Error,
		TEXT("%s: bPlanDuringExecution is false but no plan was pre-planned. Check that at least one of bPlanDuringExecution and bPlanDuringOuterPlanPlanning on the SubPlan task is enabled."),
		*GetLogPrefix());

	return bReusePlan;
}
