// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNComponent.h"

#include "AITask_MakeHTNPlan.h"
#include "Decorators/HTNDecorator_Cooldown.h"
#include "HTN.h"
#include "HTNExtension.h"
#include "HTNPlan.h"
#include "HTNTask.h"
#include "HTNTypes.h"
#include "HTNDecorator.h"
#include "HTNDelegates.h"
#include "HTNService.h"
#include "Nodes/HTNNode_Parallel.h"
#include "Nodes/HTNNode_SubNetworkDynamic.h"
#include "WorldStateProxy.h"

#include "AIController.h"
#include "Algo/AllOf.h"
#include "Algo/Find.h"
#include "Algo/Transform.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameplayTasksComponent.h"
#include "Misc/ScopeExit.h"
#include "Misc/RuntimeErrors.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "VisualLogger/VisualLoggerTypes.h"
#include "VisualLogger/VisualLogger.h"

DECLARE_CYCLE_STAT(TEXT("Sub-plan Instance Creation Time"), STAT_AI_HTN_SubPlanInstanceCreation, STATGROUP_AI_HTN);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sub-plan Instances Created"), STAT_AI_HTN_SubPlanInstancesCreated, STATGROUP_AI_HTN);

#if USE_HTN_DEBUGGER
TArray<TWeakObjectPtr<UHTNComponent>> UHTNComponent::PlayingComponents;
#endif

FHTNDebugExecutionStep& FHTNDebugSteps::Add_GetRef()
{
	if (Steps.Num() >= 100)
	{
		Steps.RemoveAt(0, 1, HTN_DISALLOW_SHRINKING);
	}

	const int32 Index = GetLastIndex() + 1;
	FHTNDebugExecutionStep& DebugStep = Steps.AddDefaulted_GetRef();
	DebugStep.DebugStepIndex = Index;
	return DebugStep;
}

void FHTNDebugSteps::Reset()
{
	Steps.Reset();
}

const FHTNDebugExecutionStep* FHTNDebugSteps::GetByIndex(int32 Index) const
{
	if (Steps.Num())
	{
		const int32 FirstIndex = Steps[0].DebugStepIndex;
		const int32 ArrayIndex = Index - FirstIndex;
		if (Steps.IsValidIndex(ArrayIndex))
		{
			return &Steps[ArrayIndex];
		}
	}
	
	return nullptr;
}

FHTNDebugExecutionStep* FHTNDebugSteps::GetByIndex(int32 Index)
{
	if (Steps.Num())
	{
		const int32 FirstIndex = Steps[0].DebugStepIndex;
		const int32 ArrayIndex = Index - FirstIndex;
		if (Steps.IsValidIndex(ArrayIndex))
		{
			return &Steps[ArrayIndex];
		}
	}

	return nullptr;
}

int32 FHTNDebugSteps::GetLastIndex() const
{
	return Steps.Num() ? Steps.Last().DebugStepIndex : INDEX_NONE;
}

UHTNComponent::UHTNComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	MaxPlanLength(100),
	MaxNestedSubPlanDepth(100),
	DefaultHTNAsset(nullptr),
	bIsPaused(false),
	bDeferredCleanup(false),
	bStoppingHTN(false),
	CurrentHTNAsset(nullptr),
	PendingHTNAsset(nullptr),
	RootPlanInstance(CreateDefaultSubobject<UHTNPlanInstance>(TEXT("RootPlanInstance"))),
	PlanningWorldStateProxy(CreateDefaultSubobject<UWorldStateProxy>(TEXT("WorldStateProxy"))),
	BlackboardProxy(CreateDefaultSubobject<UWorldStateProxy>(TEXT("BlackboardProxy")))
{
	bAutoActivate = true;
	bWantsInitializeComponent = true;
}

void UHTNComponent::PostInitProperties()
{
	Super::PostInitProperties();

	RootPlanInstance->Initialize(*this, FHTNPlanInstanceID::GenerateNewID);
	RootPlanInstance->PlanExecutionStartedEvent.AddUObject(this, &ThisClass::OnPlanInstanceExecutionStarted);
	RootPlanInstance->PlanExecutionFinishedEvent.AddUObject(this, &ThisClass::OnPlanInstanceExecutionFinished);
	RootPlanInstance->PlanInstanceFinishedEvent.AddUObject(this, &ThisClass::OnPlanInstanceFinished);
}

void UHTNComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SCOPE_CYCLE_COUNTER(STAT_AI_Overall);
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_Tick);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(HTNTick);
	
	UpdateBlackboardState();
	
	if (bIsPaused)
	{
		return;
	}

	if (IsValid(PendingHTNAsset) && RootPlanInstance->CanStartExecutingNewPlan())
	{
		StartPendingHTN();
	}

	ForEachExtensionSafe([DeltaTime](UHTNExtension& Extension)
	{
		if (Extension.WantsNotifyTick())
		{
			Extension.Tick(DeltaTime);
		}
	});
	RootPlanInstance->Tick(DeltaTime);

	ProcessDeferredActions();

	UE_IFVLOG(ForEachPlanInstance(&UHTNPlanInstance::VisualizeCurrentPlan));
}

void UHTNComponent::OnRegister()
{
	Super::OnRegister();

	REDIRECT_TO_VLOG(GetOwner());
}

void UHTNComponent::BeginPlay()
{
	Super::BeginPlay();

#if USE_HTN_DEBUGGER
	PlayingComponents.AddUnique(this);
#endif
}

void UHTNComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{	
	// Cleanup and remove worldstates before the blackboard component they reference gets uninitialized
	Cleanup();

#if USE_HTN_DEBUGGER
	PlayingComponents.Remove(this);
#endif

	Super::EndPlay(EndPlayReason);
}

void UHTNComponent::StartLogic()
{
	UE_VLOG(this, LogHTN, Log, TEXT("UHTNComponent::StartLogic"));

	if (RootPlanInstance->GetStatus() == EHTNPlanInstanceStatus::InProgress)
	{
		UE_VLOG(this, LogHTN, Log, TEXT("UHTNComponent::StartLogic: Skipping, logic already started."));
		return;
	}

	if (!IsValid(PendingHTNAsset))
	{
		PendingHTNAsset = DefaultHTNAsset;
	}

	if (IsValid(PendingHTNAsset))
	{
		StartPendingHTN();
	}
	else
	{
		UE_VLOG(this, LogHTN, Log, TEXT("UHTNComponent::StartLogic: Could not find HTN asset to run."));
	}
}

void UHTNComponent::RestartLogic()
{
	UE_VLOG(this, LogHTN, Log, TEXT("UHTNComponent::RestartLogic"));

	CancelActivePlanning();
	if (HasActivePlan()) 
	{
		RootPlanInstance->AbortCurrentPlan();
	}
}

void UHTNComponent::StopLogic(const FString& Reason)
{
	UE_VLOG(this, LogHTN, Log, TEXT("UHTNComponent::StopLogic: \'%s\'"), *Reason);
	StopHTN();
}

void UHTNComponent::Cleanup()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_Cleanup);

	if (bStoppingHTN)
	{
		UE_CVLOG(GetWorld(), this, LogHTN, Log, TEXT("deferring Cleanup because the HTNComponent is stopping HTN"));
		bDeferredCleanup = true;
		return;
	}

	const EHTNLockFlags LockFlags = GetLockFlags();
	if (LockFlags != EHTNLockFlags::None)
	{
		UE_CVLOG(GetWorld(), this, LogHTN, Log, TEXT("deferring Cleanup due to lock flags: %s"), *HTNLockFlagsToString(LockFlags));
		bDeferredCleanup = true;
		return;
	}

	UE_CVLOG(GetWorld(), this, LogHTN, Log, TEXT("Cleanup"));

	ON_SCOPE_EXIT { bDeferredCleanup = false; };
	
	// Ensure the worldstates in the plan are deallocated before their linked BlackboardComponent,
	// since they need info from there to properly deallocate their values.
	StopHTN(/*bDisregardLatentAbort*/true);
	RootPlanInstance->Reset();
	DestroyNodeInstancePools();
	PendingHTNAsset = nullptr;

	SetPlanningWorldState(nullptr);
	
	// End gameplay tasks
	if (AIOwner)
	{
		if (UGameplayTasksComponent* const GTComp = AIOwner->GetGameplayTasksComponent())
		{
			GTComp->EndAllResourceConsumingTasksOwnedBy(*this);
		}
	}

#if USE_HTN_DEBUGGER
	DebuggerSteps.Reset();
#endif

	ForEachExtensionSafe([](UHTNExtension& Extension)
	{
		Extension.Cleanup();
#if ENGINE_MAJOR_VERSION < 5
		Extension.MarkPendingKill();
#else
		Extension.MarkAsGarbage();
#endif
	});
	Extensions.Reset();
}

void UHTNComponent::PauseLogic(const FString& Reason)
{
	UE_VLOG(this, LogHTN, Log, TEXT("Execution updates: PAUSED (%s)"), *Reason);
	bIsPaused = true;

	if (BlackboardComp)
	{
		BlackboardComp->PauseObserverNotifications();
	}
}

EAILogicResuming::Type UHTNComponent::ResumeLogic(const FString& Reason)
{
	const EAILogicResuming::Type SuperResumeResult = Super::ResumeLogic(Reason);
	if (bIsPaused)
	{
		bIsPaused = false;

		if (SuperResumeResult == EAILogicResuming::Continue)
		{
			if (BlackboardComp)
			{
				// Resume the blackboard's observer notifications and send any queued notifications
				BlackboardComp->ResumeObserverNotifications(true);
			}
		}
		else if (SuperResumeResult == EAILogicResuming::RestartedInstead)
		{
			if (BlackboardComp)
			{
				// Resume the blackboard's observer notifications but do not send any queued notifications
				BlackboardComp->ResumeObserverNotifications(false);
			}
		}
	}

	return SuperResumeResult;
}

bool UHTNComponent::IsRunning() const
{
	return !IsPaused() && (IsValid(GetCurrentHTN()) || IsValid(PendingHTNAsset));
}

bool UHTNComponent::IsPaused() const
{
	return bIsPaused;
}

UGameplayTasksComponent* UHTNComponent::GetGameplayTasksComponent(const UGameplayTask& Task) const
{
	// Helpers for making sure the AIController has a gameplay tasks component.
	// AIControllers possessing a pawn make sure there is one on their own, but standalone ones don't.
	struct Local
	{
		struct AAIControllerHelper : public AAIController
		{
			static void SetCachedGameplayComponent(AAIController& InAIController, UGameplayTasksComponent* TasksComponent)
			{
				StaticCast<AAIControllerHelper&>(InAIController).CachedGameplayTasksComponent = TasksComponent;
			}
		};
		
		static UGameplayTasksComponent* EnsureGameplayTasksComponent(AAIController& InAIController, const UGameplayTask& InTask)
		{
			if (UGameplayTasksComponent* const ExistingTasksComponent = InAIController.GetGameplayTasksComponent(InTask))
			{
				return ExistingTasksComponent;
			}

			if (UGameplayTasksComponent* const FoundTasksComponent = InAIController.FindComponentByClass<UGameplayTasksComponent>())
			{
				AAIControllerHelper::SetCachedGameplayComponent(InAIController, FoundTasksComponent);
				return FoundTasksComponent;
			}

			UGameplayTasksComponent* const NewTasksComponent = NewObject<UGameplayTasksComponent>(&InAIController, TEXT("GameplayTasksComponent"));
			NewTasksComponent->RegisterComponent();
			AAIControllerHelper::SetCachedGameplayComponent(InAIController, NewTasksComponent);
			return NewTasksComponent;
		}
	};
	
	if (const UAITask* const AITask = Cast<UAITask>(&Task))
	{
		if (AAIController* const AIController = AITask->GetAIController())
		{
			return Local::EnsureGameplayTasksComponent(*AIController, Task);
		}
	}

	if (AIOwner)
	{
		return Local::EnsureGameplayTasksComponent(*AIOwner, Task);
	}

	return Task.GetGameplayTasksComponent();
}

AActor* UHTNComponent::GetGameplayTaskOwner(const UGameplayTask* Task) const
{
	if (!Task)
	{
		return AIOwner;
	}

	if (const UAITask* AITask = Cast<const UAITask>(Task))
	{
		return AITask->GetAIController();
	}

	if (const UGameplayTasksComponent* const TasksComponent = Task->GetGameplayTasksComponent())
	{
		return TasksComponent->GetGameplayTaskOwner(Task);
	}

	return nullptr;
}

AActor* UHTNComponent::GetGameplayTaskAvatar(const UGameplayTask* Task) const
{
	if (!Task)
	{
		return AIOwner ? AIOwner->GetPawn() : nullptr;
	}

	if (const UAITask* AITask = Cast<const UAITask>(Task))
	{
		return AITask->GetAIController() ? AITask->GetAIController()->GetPawn() : nullptr;
	}

	if (const UGameplayTasksComponent* const TasksComponent = Task->GetGameplayTasksComponent())
	{
		return TasksComponent->GetGameplayTaskAvatar(Task);
	}

	return nullptr;
}

uint8 UHTNComponent::GetGameplayTaskDefaultPriority() const { return StaticCast<uint8>(EAITaskPriority::AutonomousAI); }

void UHTNComponent::OnGameplayTaskInitialized(UGameplayTask& Task)
{
	if (const UAITask* const AITask = Cast<const UAITask>(&Task))
	{
		if (!AITask->GetAIController())
		{
			// this means that the AI task was either
			// created without specifying UAITask::OwnerController's value (like via BP's Construct Object node)
			// or it was created in C++ with an inappropriate function.
			UE_VLOG_UELOG(this, LogHTN, Error, TEXT("Missing AIController in AITask %s"), *AITask->GetName());
		}
	}
}

void UHTNComponent::StartHTN(UHTN* Asset)
{
	if (CurrentHTNAsset == Asset)
	{
		UE_VLOG(this, LogHTN, Log, TEXT("Skipping HTN start request - given asset (%s) same as current."), *GetNameSafe(Asset));
		return;
	}

	StopHTN();

	PendingHTNAsset = Asset;
	if (!RootPlanInstance->HasActivePlan())
	{
		StartPendingHTN();
	}
}

void UHTNComponent::StopHTN(bool bDisregardLatentAbort)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_StopHTN);

	if (bStoppingHTN)
	{
		UE_CVLOG(GetWorld(), this, LogHTN, Log, TEXT("ignoring StopHTN because the HTNComponent is already stopping HTN"));
		return;
	}

	const EHTNLockFlags LockFlags = GetLockFlags();
	if (LockFlags != EHTNLockFlags::None)
	{
		UE_CVLOG(GetWorld(), this, LogHTN, Log, TEXT("deferring StopHTN due to lock flags: %s"), *HTNLockFlagsToString(LockFlags));
		DeferredStopHTNInfo = { bDisregardLatentAbort };
		return;
	}

	UE_CVLOG(GetWorld(), this, LogHTN, Log, TEXT("StopHTN"));

	ON_SCOPE_EXIT { DeferredStopHTNInfo.Reset(); };
	bStoppingHTN = true;
	if (RootPlanInstance->GetStatus() == EHTNPlanInstanceStatus::InProgress)
	{
		RootPlanInstance->Stop(bDisregardLatentAbort);
	}
	else
	{
		OnStopHTNFinished();
	}
}

void UHTNComponent::CancelActivePlanning()
{
	ForEachPlanInstance(&UHTNPlanInstance::CancelActivePlanning);
}

void UHTNComponent::BP_Replan(const FHTNReplanParameters& Params)
{
	Replan(Params);
}

void UHTNComponent::Replan(const FHTNReplanParameters& Params)
{
	RootPlanInstance->Replan(Params);
}

void UHTNComponent::SetPlanningWorldState(TSharedPtr<FBlackboardWorldState> WorldState, bool bIsEditable)
{
	check(PlanningWorldStateProxy);
	PlanningWorldStateProxy->WorldState = WorldState;
	PlanningWorldStateProxy->bIsEditable = bIsEditable;
}

UHTNExtension* UHTNComponent::FindExtensionByClass(TSubclassOf<UHTNExtension> ExtensionClass) const
{
	UHTNExtension* const Extension = Extensions.FindRef(ExtensionClass);
	return IsValid(Extension) ? Extension : nullptr;
}

UHTNExtension* UHTNComponent::BP_FindExtensionByClass(EHTNFindExtensionResult& OutResult, TSubclassOf<UHTNExtension> ExtensionClass) const
{
	UHTNExtension* const Extension = FindExtensionByClass(ExtensionClass);
	OutResult = IsValid(Extension) ? EHTNFindExtensionResult::Found : EHTNFindExtensionResult::NotFound;
	return Extension;
}

UHTNExtension* UHTNComponent::FindOrAddExtensionByClass(TSubclassOf<UHTNExtension> ExtensionClass)
{
	UHTNExtension* Extension = FindExtensionByClass(ExtensionClass);
	
	if (!IsValid(Extension) && IsValid(ExtensionClass) && !ExtensionClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_VLOG(this, LogHTN, Log, TEXT("Creating an HTN extension: %s"), *GetNameSafe(ExtensionClass));
		Extension = Extensions.Add(ExtensionClass, NewObject<UHTNExtension>(this, ExtensionClass));
		if (IsValid(Extension))
		{
			Extension->Initialize();
		}
	}

	return Extension;
}

bool UHTNComponent::RemoveExtensionByClass(TSubclassOf<UHTNExtension> ExtensionClass)
{
	if (UHTNExtension* const Extension = FindExtensionByClass(ExtensionClass))
	{
		Extension->Cleanup();
#if ENGINE_MAJOR_VERSION < 5
		Extension->MarkPendingKill();
#else
		Extension->MarkAsGarbage();
#endif
		Extensions.Remove(ExtensionClass);
		return true;
	}

	return false;
}

float UHTNComponent::GetCooldownEndTime(const UObject* CooldownOwner) const
{
	const UHTNExtension_Cooldown* const Extension = FindExtension<UHTNExtension_Cooldown>();
	if (IsValid(Extension))
	{
		return Extension->GetCooldownEndTime(CooldownOwner);
	}

	return -FLT_MAX;
}

void UHTNComponent::AddCooldownDuration(const UObject* CooldownOwner, float CooldownDuration, bool bAddToExistingDuration)
{
	UHTNExtension_Cooldown& Extension = FindOrAddExtension<UHTNExtension_Cooldown>();
	Extension.AddCooldownDuration(CooldownOwner, CooldownDuration, bAddToExistingDuration);
}

UHTN* UHTNComponent::GetDynamicHTN(FGameplayTag InjectTag) const
{
	UHTNExtension_SubNetworkDynamic* const Extension = FindExtension<UHTNExtension_SubNetworkDynamic>();
	return Extension ? Extension->GetDynamicHTN(InjectTag) : nullptr;
}

bool UHTNComponent::SetDynamicHTN(FGameplayTag InjectTag, UHTN* HTN, 
	bool bForceReplanIfChangedNodesInCurrentPlan, bool bForceReplanIfChangedDuringPlanning, bool bForceAbortCurrentPlanIfForcingReplan)
{
	UHTNExtension_SubNetworkDynamic& Extension = FindOrAddExtension<UHTNExtension_SubNetworkDynamic>();
	return Extension.SetDynamicHTN(InjectTag, HTN, 
		bForceReplanIfChangedNodesInCurrentPlan, bForceReplanIfChangedDuringPlanning, bForceAbortCurrentPlanIfForcingReplan);
}

UAITask_MakeHTNPlan* UHTNComponent::MakePlanningTask()
{
	while (!PlanningTasksPool.IsEmpty())
	{
		UAITask_MakeHTNPlan* const ExistingTask = PlanningTasksPool.Pop(HTN_DISALLOW_SHRINKING);
		if (IsValid(ExistingTask))
		{
			UE_VLOG(this, LogHTN, Log, TEXT("%s reused from the pool"), *GetNameSafe(ExistingTask));
			return ExistingTask;
		}
		else
		{
			UE_VLOG(this, LogHTN, Warning, TEXT("found an invalid planning task in the pool!"));
		}
	}

	UAITask_MakeHTNPlan* const FreshTask = UAITask::NewAITask<UAITask_MakeHTNPlan>(*GetAIOwner(), *this, TEXT("Make HTN Plan"));
	INC_DWORD_STAT(STAT_AI_HTN_NumAITask_MakeHTNPlan);
	UE_VLOG(this, LogHTN, Log, TEXT("%s created via NewAITask"), *GetNameSafe(FreshTask));
	return FreshTask;
}

void UHTNComponent::ReturnPlanningTaskToPool(UAITask_MakeHTNPlan& Task)
{
	UE_VLOG(this, LogHTN, Log, TEXT("%s returned to pool"), *GetNameSafe(&Task));
	check(!PlanningTasksPool.Contains(&Task));
	PlanningTasksPool.Push(&Task);
}

UHTNNode* UHTNComponent::MakeNodeInstance(const UHTNNode& TemplateNode)
{
	if (!ensure(TemplateNode.HasInstance()))
	{
		return nullptr;
	}

	if (!ensure(!TemplateNode.IsInstance()))
	{
		return nullptr;
	}

	if (TemplateNode.ShouldPoolNodeInstances())
	{
		if (FHTNNodeInstancePool* const Pool = NodeInstancePools.Find(&TemplateNode))
		{
			while (!Pool->Instances.IsEmpty())
			{
				UHTNNode* const ExistingInstance = Pool->Instances.Pop(HTN_DISALLOW_SHRINKING);
				if (IsValid(ExistingInstance))
				{
					UE_VLOG(this, LogHTN, VeryVerbose, TEXT("Reusing '%s' from the node instance pool"),
						*ExistingInstance->GetShortDescription());
					return ExistingInstance;
				}
				else
				{
					UE_VLOG(this, LogHTN, Warning, TEXT("Found an invalid node instance of '%s' in the node instance pool!"),
						*TemplateNode.GetShortDescription());
				}
			}
		}

		UE_VLOG(this, LogHTN, VeryVerbose, TEXT("Creating a new node instance from '%s' because its node instance pool is empty"), 
			*TemplateNode.GetShortDescription());
	}

	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_NodeInstantiation);
	UHTNNode* const NodeInstance = DuplicateObject(&TemplateNode, this);
	INC_DWORD_STAT(STAT_AI_HTN_NumNodeInstancesCreated);
	return NodeInstance;
}

void UHTNComponent::RecycleNodeInstance(UHTNNode& NodeInstance)
{
	if (!ensure(NodeInstance.IsInstance()))
	{
		return;
	}

	if (NodeInstance.ShouldPoolNodeInstances())
	{
		UE_VLOG(this, LogHTN, VeryVerbose, TEXT("Returning '%s' to node instance pool"), *NodeInstance.GetShortDescription());
		NodeInstance.OnInstanceReturnedToPool(*this);
		check(IsValid(&NodeInstance));
		FHTNNodeInstancePool& Pool = NodeInstancePools.FindOrAdd(NodeInstance.GetTemplateNode());
		check(!Pool.Instances.Contains(&NodeInstance));
		Pool.Instances.Add(&NodeInstance);
	}
	else
	{
		// If the Node is an instance created just for this plan, explicitly mark it as pending kill. 
		// This makes IsValid() checks fail for the Node. 
		// 
		// This is necessary because at this point there may be outstanding latent actions for this node.
		// For example, if the node started a latent action OnPlanExecutionStart but the plan got aborted before the node begins execution,
		// so OnTaskFinished/OnExecutionFinish never got called to cancel the latent actions. 
		// Or the node did try to cancel its latent actions but that didn't work because 
		// FLatentActionManager::RemoveActionsForObject doesn't guarantee removal of the actions 
		// if we're in the middle of processing latent actions (e.g., the Tick of the HTNComponent).
		// 
		// Killing the node instance completely is a more reliable way of ensuring that 
		// FLatentActionManager won't execute any of that instance's pending latent actions, 
		// compared to FLatentActionManager::RemoveActionsForObject.
		// 
		// Nodes that get pooled work around this issue by resetting the node instances' serial number, 
		// thus invalidating the CallbackTarget WeakObjectPtr inside latent actions. 
		// For more details see the comments inside UHTNNode::OnInstanceReturnedToPool.
#if ENGINE_MAJOR_VERSION < 5
		NodeInstance.MarkPendingKill();
#else
		NodeInstance.MarkAsGarbage();
#endif		
	}
}

#if ENABLE_VISUAL_LOG

void UHTNComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	if (!IsValid(this))
	{
		return;
	}

	Super::DescribeSelfToVisLog(Snapshot);

	{
		FVisualLogStatusCategory& HTNCategory = Snapshot->Status.Emplace_GetRef(TEXT("HTN"));
		HTNCategory.Add(TEXT("HTN Asset"), *GetNameSafe(CurrentHTNAsset));
		HTNCategory.Add(TEXT("Pending HTN Asset"), *GetNameSafe(PendingHTNAsset));
		HTNCategory.Add(TEXT("Is Paused"), TEXT_CONDITION(IsPaused()));
		HTNCategory.Add(TEXT("Is Stopping HTN"), TEXT_CONDITION(IsStoppingHTN()));
		HTNCategory.Add(TEXT("Has Deferred Cleanup"), TEXT_CONDITION(bDeferredCleanup));
	}

	ForEachPlanInstance([Snapshot](const UHTNPlanInstance& Instance) { Instance.DescribeSelfToVisLog(Snapshot); });
	ForEachExtensionSafe([&](const UHTNExtension& Extension) { Extension.DescribeSelfToVisLog(Snapshot); });
}

#endif

EHTNLockFlags UHTNComponent::GetLockFlags() const
{
	EHTNLockFlags Flags = EHTNLockFlags::None;
	ForEachPlanInstance([&](const UHTNPlanInstance& Instance) { Flags |= Instance.GetLockFlags(); });

	return Flags;
}

void UHTNComponent::StartPendingHTN()
{	
	CurrentHTNAsset = PendingHTNAsset;
	PendingHTNAsset = nullptr;

	if (CurrentHTNAsset)
	{
		if (EnsureCompatibleBlackboardAsset(CurrentHTNAsset->BlackboardAsset))
		{
			check(!HasPlan());
			ForEachExtensionSafe(&UHTNExtension::HTNStarted);
			RootPlanInstance->Start();
		}
		else
		{
			StopHTN();
		}
	}
}

bool UHTNComponent::EnsureCompatibleBlackboardAsset(UBlackboardData* DesiredBlackboardAsset)
{
	if (!DesiredBlackboardAsset)
	{
		UE_VLOG_UELOG(this, LogHTN, Error, TEXT("HTN trying to assign null blackboard asset."));
		return false;
	}
	
	UBlackboardComponent* BlackboardComponent = AIOwner->GetBlackboardComponent();
	if (!BlackboardComponent || !BlackboardComponent->IsCompatibleWith(DesiredBlackboardAsset))
	{
		// If changing to a different blackboard asset, remove all worldstates first.
		// This is necessary because worldstates rely on the blackboard component to manage their data.
		// If the blackboard component's asset changes, the worldstates wouldn't be able to deallocate correctly.
		// To prevent that, we destroy them first.
		DeleteAllWorldStates();
		if (!AIOwner->UseBlackboard(DesiredBlackboardAsset, BlackboardComponent))
		{
			UE_VLOG_UELOG(this, LogHTN, Error, TEXT("Could not use blackboard asset %s required by HTN %s. Previous blackboard asset is %s."),
				*GetNameSafe(DesiredBlackboardAsset),
				*GetNameSafe(CurrentHTNAsset),
				*GetNameSafe(BlackboardComponent ? BlackboardComponent->GetBlackboardAsset() : nullptr));
			return false;
		}
	}

	if (!BlackboardComp)
	{
		CacheBlackboardComponent(BlackboardComponent);
	}

	return true;
}

void UHTNComponent::DeleteAllWorldStates()
{
	ForEachPlanInstance(&UHTNPlanInstance::DeleteAllWorldStates);

	PendingHTNAsset = nullptr;

	CancelActivePlanning();
	SetPlanningWorldState(nullptr);
	
#if USE_HTN_DEBUGGER
	DebuggerSteps.Reset();
#endif
}

void UHTNComponent::ProcessDeferredActions()
{
	if (DeferredStopHTNInfo.IsSet())
	{
		StopHTN(DeferredStopHTNInfo->bDisregardLatentAbort);
	}

	if (bDeferredCleanup)
	{
		Cleanup();
	}
}

bool UHTNComponent::HasActivePlan() const
{
	return HasPlan() && HasActiveTasks();
}

bool UHTNComponent::HasPlan() const
{
	return RootPlanInstance->HasPlan();
}

bool UHTNComponent::HasActiveTasks() const
{
	return RootPlanInstance->HasActiveTasks();
}

bool UHTNComponent::IsWaitingForAbortingTasks() const
{
	return RootPlanInstance->IsWaitingForAbortingTasks();
}

bool UHTNComponent::IsPlanning() const
{
	return RootPlanInstance->IsPlanning();
}

bool UHTNComponent::IsAbortingPlan() const
{
	return RootPlanInstance->IsAbortingPlan();
}

bool UHTNComponent::HasDeferredAbort() const
{
	return RootPlanInstance->HasDeferredAbort();
}

bool UHTNComponent::HasDeferredStop() const
{
	return DeferredStopHTNInfo.IsSet() || bDeferredCleanup;
}

bool UHTNComponent::IsStoppingHTN() const
{
	return bStoppingHTN;
}

const UHTNPlanInstance& UHTNComponent::GetRootPlanInstance() const
{
	return *RootPlanInstance;
}

UHTNPlanInstance& UHTNComponent::GetRootPlanInstance()
{
	return *RootPlanInstance;
}

const UHTNPlanInstance* UHTNComponent::FindPlanInstanceByNodeMemory(const uint8* NodeMemory) const
{
	return FindPlanInstanceBy([NodeMemory](const UHTNPlanInstance& Instance) { return Instance.OwnsNodeMemory(NodeMemory); });
}

UHTNPlanInstance* UHTNComponent::FindPlanInstanceByNodeMemory(const uint8* NodeMemory)
{
	return const_cast<UHTNPlanInstance*>(const_cast<const ThisClass*>(this)->FindPlanInstanceByNodeMemory(NodeMemory));
}

UHTNPlanInstance& UHTNComponent::AddSubPlanInstance(const FHTNPlanInstanceConfig& Config)
{
	INC_DWORD_STAT(STAT_AI_HTN_SubPlanInstancesCreated);
	SCOPE_CYCLE_COUNTER(STAT_AI_HTN_SubPlanInstanceCreation);

	const FHTNPlanInstanceID PlanInstanceID(FHTNPlanInstanceID::GenerateNewID);
	UHTNPlanInstance* const NewInstance = SubPlanInstances.Add(PlanInstanceID, NewObject<UHTNPlanInstance>(this));
	check(NewInstance);
	NewInstance->Initialize(*this, PlanInstanceID, Config);
	UE_VLOG(this, LogHTN, Log, TEXT("created subplan instance: %s"), *NewInstance->GetLogPrefix());
	return *NewInstance;
}

bool UHTNComponent::RemoveSubPlanInstance(const FHTNPlanInstanceID& ID)
{
	if (UHTNPlanInstance* const Instance = SubPlanInstances.FindRef(ID))
	{
		UE_VLOG(this, LogHTN, Log, TEXT("destroying subplan instance: %s"), *Instance->GetLogPrefix());

		Instance->OnPreDestroy();

#if ENGINE_MAJOR_VERSION < 5
		Instance->MarkPendingKill();
#else
		Instance->MarkAsGarbage();
#endif
		SubPlanInstances.Remove(ID);

		return true;
	}

	return false;
}

const UHTNPlanInstance* UHTNComponent::FindPlanInstance(const FHTNPlanInstanceID& ID) const
{
	if (RootPlanInstance->GetID() == ID)
	{
		return RootPlanInstance;
	}

	return SubPlanInstances.FindRef(ID);
}

UHTNPlanInstance* UHTNComponent::FindPlanInstance(const FHTNPlanInstanceID& ID)
{
	return const_cast<UHTNPlanInstance*>(const_cast<const ThisClass*>(this)->FindPlanInstance(ID));
}

UHTNNode* UHTNComponent::GetNodeFromMemory(const UHTNNode* NodeTemplate, const uint8* NodeMemory) const
{
	if (!NodeTemplate)
	{
		return nullptr;
	}

	if (!NodeTemplate->HasInstance())
	{
		return const_cast<UHTNNode*>(NodeTemplate);
	}

	if (const UHTNPlanInstance* const OwningPlanInstance = FindPlanInstanceByNodeMemory(NodeMemory))
	{
		return OwningPlanInstance->GetNodeInstanceInCurrentPlan(*NodeTemplate, NodeMemory);
	}

	return nullptr;
}

FHTNNodeInPlanInfo UHTNComponent::FindActiveNodeInfo(const UHTNNode* Node, const uint8* NodeMemory) const
{
	if (IsValid(Node))
	{
		if (const UHTNTask* const Task = Cast<UHTNTask>(Node))
		{
			return FindActiveTaskInfo(Task, NodeMemory);
		}
		else if (const UHTNDecorator* const Decorator = Cast<UHTNDecorator>(Node))
		{
			return FindActiveDecoratorInfo(Decorator, NodeMemory);
		}
		else if (const UHTNService* const Service = Cast<UHTNService>(Node))
		{
			return FindActiveServiceInfo(Service, NodeMemory);
		}
		else
		{
			checkNoEntry();
		}
	}

	return {};
}

FHTNNodeInPlanInfo UHTNComponent::FindActiveTaskInfo(const UHTNTask* Task, const uint8* NodeMemory) const
{
	FHTNNodeInPlanInfo Result;
	const UHTNPlanInstance* const OwningPlanInstance = FindPlanInstanceBy([&](const UHTNPlanInstance& Instance) -> bool
	{
		Result = Instance.FindActiveTaskInfo(Task, NodeMemory);
		return Result.IsValid();
	});

	if (OwningPlanInstance)
	{
		check(Result.PlanInstance == OwningPlanInstance);
		check(!NodeMemory || NodeMemory == Result.NodeMemory);
		return Result;
	}
	
	return {};
}

FHTNNodeInPlanInfo UHTNComponent::FindActiveDecoratorInfo(const UHTNDecorator* Decorator, const uint8* NodeMemory) const
{
	FHTNNodeInPlanInfo Result;
	const UHTNPlanInstance* const OwningPlanInstance = FindPlanInstanceBy([&](const UHTNPlanInstance& Instance) -> bool
	{
		Result = Instance.FindActiveDecoratorInfo(Decorator, NodeMemory);
		return Result.IsValid();
	});

	if (OwningPlanInstance)
	{
		check(Result.PlanInstance == OwningPlanInstance);
		check(!NodeMemory || NodeMemory == Result.NodeMemory);
		return Result;
	}

	return {};
}

FHTNNodeInPlanInfo UHTNComponent::FindActiveServiceInfo(const UHTNService* Service, const uint8* NodeMemory) const
{
	FHTNNodeInPlanInfo Result;
	const UHTNPlanInstance* const OwningPlanInstance = FindPlanInstanceBy([&](const UHTNPlanInstance& Instance) -> bool
	{
		Result = Instance.FindActiveServiceInfo(Service, NodeMemory);
		return Result.IsValid();
	});

	if (OwningPlanInstance)
	{
		check(Result.PlanInstance == OwningPlanInstance);
		check(!NodeMemory || NodeMemory == Result.NodeMemory);
		return Result;
	}

	return {};
}

void UHTNComponent::UpdateBlackboardState() const
{
	if (const AAIController* const AIController = GetAIOwner())
	{
		if (const APawn* const Pawn = AIController->GetPawn())
		{
			if (BlackboardComp && BlackboardComp->GetBlackboardAsset())
			{
				const FBlackboard::FKey SelfLocationKey = BlackboardComp->GetBlackboardAsset()->GetKeyID(FBlackboard::KeySelfLocation);
				if (ensureAsRuntimeWarning(SelfLocationKey != FBlackboard::InvalidKey))
				{
					BlackboardComp->SetValue<UBlackboardKeyType_Vector>(SelfLocationKey, Pawn->GetActorLocation());
				}
			}
		}
	}
}

void UHTNComponent::OnPlanInstanceExecutionStarted(UHTNPlanInstance* Sender)
{
	const bool bIsRootPlanInstance = Sender == RootPlanInstance;
	check(bIsRootPlanInstance || (Sender && SubPlanInstances.Contains(Sender->GetID())));
	if (bIsRootPlanInstance)
	{
		FHTNDelegates::OnPlanExecutionStarted.Broadcast(*this, Sender->GetCurrentPlan());
		PlanExecutionStartedEvent.Broadcast(this);
		PlanExecutionStartedBPEvent.Broadcast(this);
	}
}

void UHTNComponent::OnPlanInstanceExecutionFinished(UHTNPlanInstance* Sender, EHTNPlanExecutionFinishedResult Result)
{
	const bool bIsRootPlanInstance = Sender == RootPlanInstance;
	check(bIsRootPlanInstance || (Sender && SubPlanInstances.Contains(Sender->GetID())));
	if (bIsRootPlanInstance)
	{
		PlanExecutionFinishedEvent.Broadcast(this, Result);
		PlanExecutionFinishedBPEvent.Broadcast(this, Result);
	}
}

void UHTNComponent::OnPlanInstanceFinished(UHTNPlanInstance* Sender)
{
	if (bStoppingHTN)
	{
		const bool bIsRootPlanInstance = Sender == RootPlanInstance;
		check(bIsRootPlanInstance || (Sender && SubPlanInstances.Contains(Sender->GetID())));
		if (bIsRootPlanInstance)
		{
			OnStopHTNFinished();
		}
	}
}

void UHTNComponent::OnStopHTNFinished()
{
	if (ensure(bStoppingHTN))
	{
		SetPlanningWorldState(nullptr);
		CurrentHTNAsset = nullptr;

		bStoppingHTN = false;
	}
}

void UHTNComponent::ForEachExtensionSafe(TFunctionRef<void(UHTNExtension&)> Callable) const
{
	// Take out the values first in case the Callable ends up modifying the Extensions map.
	// This prevents problems with iterating over a collection when it changes.
	TArray<TObjectPtr<UHTNExtension>, TInlineAllocator<10>> ExtensionArray;
	Extensions.GenerateValueArray(ExtensionArray);

	for (UHTNExtension* const Extension : ExtensionArray)
	{
		if (IsValid(Extension))
		{
			Callable(*Extension);
		}
	}
}

void UHTNComponent::DestroyNodeInstancePools()
{
	int32 NumNodesMarkedForDestruction = 0;

	for (auto It = NodeInstancePools.CreateIterator(); It; ++It)
	{
		FHTNNodeInstancePool& Pool = It.Value();
		while (!Pool.Instances.IsEmpty())
		{
			UHTNNode* const NodeInstance = Pool.Instances.Pop(HTN_DISALLOW_SHRINKING);
			if (IsValid(NodeInstance))
			{
#if ENGINE_MAJOR_VERSION < 5
				NodeInstance->MarkPendingKill();
#else
				NodeInstance->MarkAsGarbage();
#endif
				++NumNodesMarkedForDestruction;
			}
		}
	}
	NodeInstancePools.Empty();

	UE_CVLOG_UELOG(GetWorld(), this, LogHTN, Log, TEXT("Marked %d pooled node instances for destruction"), NumNodesMarkedForDestruction);
}

#if USE_HTN_DEBUGGER

FHTNDebugExecutionStep& UHTNComponent::StoreDebugStep(bool bIsEmpty) const
{
	FHTNDebugExecutionStep& DebugStep = DebuggerSteps.Add_GetRef();

	if (!bIsEmpty)
	{
		// Describe blackboard values
		const UBlackboardComponent* const Blackboard = GetBlackboardComponent();
		if (IsValid(Blackboard) && Blackboard->HasValidAsset())
		{
			const int32 NumKeys = Blackboard->GetNumKeys();
			DebugStep.BlackboardValues.Empty(NumKeys);
			for (int32 KeyIndex = 0; KeyIndex < NumKeys; ++KeyIndex)
			{
				const FBlackboard::FKey KeyID(KeyIndex);
				const FString Value = Blackboard->DescribeKeyValue(KeyID, EBlackboardDescription::OnlyValue);
				DebugStep.BlackboardValues.Add(Blackboard->GetKeyName(KeyID), Value.Len() ? Value : TEXT("n/a"));
			}
		}

		ForEachPlanInstance([&](const UHTNPlanInstance& PlanInstance)
		{
			DebugStep.PerPlanInstanceInfo.Add(PlanInstance.GetID(), PlanInstance.GetDebugStepInfo());
		});
	}

	return DebugStep;
}

#endif
