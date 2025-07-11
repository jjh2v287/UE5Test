// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "BrainComponent.h"
#include "Delegates/Delegate.h"
#include "GameplayTagContainer.h"
#include "GameplayTaskOwnerInterface.h"
#include "HTN.h"
#include "HTNPlanInstance.h"
#include "HTNTypes.h"
#include "HTNComponent.generated.h"

UENUM(BlueprintType)
enum class EHTNFindExtensionResult : uint8
{
	Found,
	NotFound
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHTNPlanExecutionStartedBP, UHTNComponent*, Sender);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHTNPlanExecutionFinishedBP, UHTNComponent*, Sender, EHTNPlanExecutionFinishedResult, Result);

struct FDeferredStopHTNInfo
{
	bool bDisregardLatentAbort = false;
};

USTRUCT()
struct FHTNNodeInstancePool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<class UHTNNode>> Instances;
};

// The HTN counterpart to UBehaviorTreeComponent. Can run a given HTN asset.
UCLASS(ClassGroup = AI, Meta = (BlueprintSpawnableComponent))
class HTN_API UHTNComponent : public UBrainComponent, public IGameplayTaskOwnerInterface
{
	GENERATED_BODY()

public:
	UHTNComponent(const FObjectInitializer& ObjectInitializer);
	
	virtual void PostInitProperties() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void StartLogic() override;
	virtual void RestartLogic() override;
	virtual void StopLogic(const FString& Reason) override;
	virtual void Cleanup() override;
	virtual void PauseLogic(const FString& Reason) override;
	virtual EAILogicResuming::Type ResumeLogic(const FString& Reason) override;
	virtual bool IsRunning() const override;
	virtual bool IsPaused() const override;

	// Begin IGameplayTaskOwnerInterface
	virtual UGameplayTasksComponent* GetGameplayTasksComponent(const UGameplayTask& Task) const override;
	virtual AActor* GetGameplayTaskOwner(const UGameplayTask* Task) const override;
	virtual AActor* GetGameplayTaskAvatar(const UGameplayTask* Task) const override;
	virtual uint8 GetGameplayTaskDefaultPriority() const override;
	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;
	// End IGameplayTaskOwnerInterface

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const override;
#endif

	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	void StartHTN(class UHTN* Asset);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	void StopHTN(bool bDisregardLatentAbort = false);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	void CancelActivePlanning();

	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (DisplayName = "Replan", AutoCreateRefTerm = "Params"))
	void BP_Replan(const FHTNReplanParameters& Params);

	void Replan(const FHTNReplanParameters& Params = FHTNReplanParameters::Default);

	// Equivalent to HasPlan() && HasActiveTasks()
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool HasActivePlan() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool HasPlan() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool HasActiveTasks() const;

	// Are there any tasks that are currently taking their time to abort? New tasks can't begin execution while some are aborting. 
	// This may be true while bAbortingPlan is not true in cases when tasks are aborting but won't cancel the plan.
	// The other way around is also possible in the moment where the last task has finished aborting. This leads to the abort being finished.
	// (i.e. when aborting the secondary branch of a Parallel node).
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool IsWaitingForAbortingTasks() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool IsPlanning() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool IsAbortingPlan() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool HasDeferredAbort() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool HasDeferredStop() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	bool IsStoppingHTN() const;

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	FORCEINLINE UHTN* GetCurrentHTN() const { return CurrentHTNAsset; }

	// Plan instance manipulation functions.
	const UHTNPlanInstance& GetRootPlanInstance() const;
	UHTNPlanInstance& GetRootPlanInstance();
	const UHTNPlanInstance* FindPlanInstanceByNodeMemory(const uint8* NodeMemory) const;
	UHTNPlanInstance* FindPlanInstanceByNodeMemory(const uint8* NodeMemory);
	UHTNPlanInstance& AddSubPlanInstance(const struct FHTNPlanInstanceConfig& Config);
	bool RemoveSubPlanInstance(const FHTNPlanInstanceID& ID);
	const UHTNPlanInstance* FindPlanInstance(const FHTNPlanInstanceID& ID) const;
	UHTNPlanInstance* FindPlanInstance(const FHTNPlanInstanceID& ID);
	template<typename PredicateType>
	FORCEINLINE const UHTNPlanInstance* FindPlanInstanceBy(PredicateType&& Predicate) const;
	template<typename PredicateType>
	FORCEINLINE UHTNPlanInstance* FindPlanInstanceBy(PredicateType&& Predicate);
	template<typename CallableType>
	FORCEINLINE void ForEachPlanInstance(CallableType&& Callable) const;
	template<typename CallableType>
	FORCEINLINE void ForEachPlanInstance(CallableType&& Callable);

	// Given a node and its memory, returns either the node itself or (in the case of bCreateNodeInstance being true on the node) 
	// an instance (copy) of it that was created specifically for a current plan.
	UHTNNode* GetNodeFromMemory(const UHTNNode* NodeTemplate, const uint8* NodeMemory) const;

	// Finds information about a currently executing node. 
	// The NodeMemory parameter is optional but helps resolve situations where the 
	// same node in an HTN asset is part of the same plan in multiple places 
	// (e.g., a Parallel node that has both branches connect to the same node)
	FHTNNodeInPlanInfo FindActiveNodeInfo(const UHTNNode* Node, const uint8* NodeMemory = nullptr) const;
	FHTNNodeInPlanInfo FindActiveTaskInfo(const class UHTNTask* Task, const uint8* NodeMemory = nullptr) const;
	FHTNNodeInPlanInfo FindActiveDecoratorInfo(const class UHTNDecorator* Decorator, const uint8* NodeMemory = nullptr) const;
	FHTNNodeInPlanInfo FindActiveServiceInfo(const class UHTNService* Service, const uint8* NodeMemory = nullptr) const;

	FORCEINLINE class UWorldStateProxy* GetPlanningWorldStateProxy() const { check(PlanningWorldStateProxy); return PlanningWorldStateProxy; }

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	FORCEINLINE class UWorldStateProxy* GetBlackboardProxy() const { check(BlackboardProxy); return BlackboardProxy; }

	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	FORCEINLINE class UWorldStateProxy* GetWorldStateProxy(bool bForPlanning) const { return bForPlanning ? GetPlanningWorldStateProxy() : GetBlackboardProxy(); }

	// Sets the "current worldstate" in the planning WorldStateProxy. Call this before handing over control to external logic like eqs contexts during planning.
	// When calling GetPlanningWorldStateProxy, they will get a proxy to the given worldstate. If null, the proxy will be pointing to the blackboard.
	void SetPlanningWorldState(TSharedPtr<class FBlackboardWorldState> WorldState, bool bIsEditable = true);

	class UHTNExtension* FindExtensionByClass(TSubclassOf<UHTNExtension> ExtensionClass) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "AI|HTN", DisplayName = "Find Extension By Class", Meta = (DynamicOutputParam = "ReturnValue", DeterminesOutputType = "ExtensionClass", ReturnDisplayName = "Extension", ExpandEnumAsExecs = "OutResult"))
	class UHTNExtension* BP_FindExtensionByClass(EHTNFindExtensionResult& OutResult, 
		UPARAM(Meta = (AllowAbstract = "false")) TSubclassOf<UHTNExtension> ExtensionClass) const;
	
	UFUNCTION(BlueprintCallable, Category = "AI|HTN", DisplayName = "Find Or Add Extension By Class", Meta = (DynamicOutputParam = "ReturnValue", DeterminesOutputType = "ExtensionClass", ReturnDisplayName = "Extension"))
	class UHTNExtension* FindOrAddExtensionByClass(UPARAM(Meta = (AllowAbstract = "false")) TSubclassOf<UHTNExtension> ExtensionClass);
	
	UFUNCTION(BlueprintCallable, Category = "AI|HTN", DisplayName = "Remove Extension By Class", Meta = (ReturnDisplayName = "Was Removed"))
	bool RemoveExtensionByClass(UPARAM(Meta = (AllowAbstract = "false")) TSubclassOf<UHTNExtension> ExtensionClass);

	template<class ExtensionType>
	ExtensionType* FindExtension(TSubclassOf<UHTNExtension> ExtensionClass = ExtensionType::StaticClass()) const;

	template<class ExtensionType>
	ExtensionType& FindOrAddExtension(TSubclassOf<UHTNExtension> ExtensionClass = ExtensionType::StaticClass());

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown", Meta = (DeprecatedFunction, DeprecationMessage = "This function is deprecated and will be removed in a future update. Please call FindOrAddExtension with HTNExtension_Cooldown and call GetCooldownEndTime on that."))
	float GetCooldownEndTime(const UObject* CooldownOwner) const;

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown", Meta = (DeprecatedFunction, DeprecationMessage = "This function is deprecated and will be removed in a future update. Please call FindOrAddExtension with HTNExtension_Cooldown and call AddCooldownDuration on that."))
	void AddCooldownDuration(const UObject* CooldownOwner, float CooldownDuration, bool bAddToExistingDuration);

	// Returns the dynamic HTN override for the given injection tag.
	// Does not return the DefaultHTN of SubNetworkDynamic nodes, only what was set using SetDynamicHTN.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Cooldown", Meta = (DeprecatedFunction, DeprecationMessage = "This function is deprecated and will be removed in a future update. Please call FindOrAddExtension with HTNExtension_SubNetworkDynamic and call GetDynamicHTN on that."))
	UHTN* GetDynamicHTN(FGameplayTag InjectTag) const;

	// Assign an HTN asset to SubNetworkDynamic nodes with the given gameplay tag.
	// Returns true if the new HTN is different from the old one.
	// If bForceReplanIfChangedNodesInCurrentPlan is enabled, and the HTN of any SubNetworkDynamic nodes in the current plan was changed due to this call, Replan will be called.
	// If bForceReplanIfChangedDuringPlanning is enabled, the new value is different from the old one, and the HTNComponent is in the process of making a new plan, Replan will be called.
	// If force-replanning, bForceAbortCurrentPlanIfForcingReplan determines if the current plan should be aborted immediately or wait until a new plan is made.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Dynamic HTN", Meta = (ReturnDisplayName = "Value Was Changed", DeprecatedFunction, DeprecationMessage = "This function is deprecated and will be removed in a future update. Please call FindOrAddExtension with HTNExtension_SubNetworkDynamic and call SetDynamicHTN on that."))
	bool SetDynamicHTN(FGameplayTag InjectTag, UHTN* HTN, 
		bool bForceReplanIfChangedNodesInCurrentPlan = true, bool bForceReplanIfChangedDuringPlanning = true, bool bForceAbortCurrentPlanIfForcingReplan = false);
	
	DECLARE_EVENT_OneParam(UHTNComponent, FOnHTNPlanExecutionStarted, UHTNComponent* /*Sender*/);
	FORCEINLINE FOnHTNPlanExecutionStarted& OnPlanExecutionStarted() { return PlanExecutionStartedEvent; }

	DECLARE_EVENT_TwoParams(UHTNComponent, FOnHTNPlanExecutionFinished, UHTNComponent* /*Sender*/, EHTNPlanExecutionFinishedResult /*Result*/);
	FORCEINLINE FOnHTNPlanExecutionFinished& OnPlanExecutionFinished() { return PlanExecutionFinishedEvent; }

	// Makes a fresh UAITask_MakeHTNPlan or reuses one from the pool.
	class UAITask_MakeHTNPlan* MakePlanningTask();

	// Makes this UAITask_MakeHTNPlan available for future planning again.
	void ReturnPlanningTaskToPool(class UAITask_MakeHTNPlan& Task);

	// Either duplicates the template node (the node in the HTN asset) or reuses one from the pool.
	UHTNNode* MakeNodeInstance(const class UHTNNode& TemplateNode);

	// Either returns the node instance to the pool for further use or destroys it, 
	// depending on its settings and the project settings.
	void RecycleNodeInstance(class UHTNNode& NodeInstance);

	// The maximum number of steps in a plan. Each standalone node (not a decorator or service) is one step.
	// This limit is used to prevent infinite recursion during planning. If it is exceeded during planning,
	// planning is aborted with an error that gets logged in the visual logger and the output log.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|HTN")
	int32 MaxPlanLength;

	// The maximum depth of nested plan instances.
	// This limit is used to prevent infinite recursion when there are SubPlan nodes deeply inside each other.
	// -1 means no limit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|HTN")
	int32 MaxNestedSubPlanDepth;

protected:
	EHTNLockFlags GetLockFlags() const;

	// When calling StartLogic (or using bStartAILogicOnPossess on the AIController, which calls StartLogic)
	// This is the HTN asset that will be run by default.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|HTN")
	TObjectPtr<class UHTN> DefaultHTNAsset;

	uint8 bIsPaused : 1;
	uint8 bDeferredCleanup : 1;
	uint8 bStoppingHTN : 1;

	TOptional<FDeferredStopHTNInfo> DeferredStopHTNInfo;

private:
	void StartPendingHTN();
	bool EnsureCompatibleBlackboardAsset(UBlackboardData* DesiredBlackboardAsset);
	void DeleteAllWorldStates();
	void ProcessDeferredActions();
	void UpdateBlackboardState() const;

	void OnPlanInstanceExecutionStarted(UHTNPlanInstance* Sender);
	void OnPlanInstanceExecutionFinished(UHTNPlanInstance* Sender, EHTNPlanExecutionFinishedResult Result);
	void OnPlanInstanceFinished(UHTNPlanInstance* Sender);
	void OnStopHTNFinished();

	void ForEachExtensionSafe(TFunctionRef<void(UHTNExtension&)> Callable) const;

	void DestroyNodeInstancePools();

#if USE_HTN_DEBUGGER
	FHTNDebugExecutionStep& StoreDebugStep(bool bIsEmpty = false) const;
#endif

	UPROPERTY(BlueprintAssignable, Category = "AI|HTN", Meta = (DisplayName = "On Plan Execution Started"))
	FOnHTNPlanExecutionStartedBP PlanExecutionStartedBPEvent;
	FOnHTNPlanExecutionStarted PlanExecutionStartedEvent;

	UPROPERTY(BlueprintAssignable, Category = "AI|HTN", Meta = (DisplayName = "On Plan Execution Finished"))
	FOnHTNPlanExecutionFinishedBP PlanExecutionFinishedBPEvent;
	FOnHTNPlanExecutionFinished PlanExecutionFinishedEvent;

	UPROPERTY()
	TObjectPtr<class UHTN> CurrentHTNAsset;

	UPROPERTY()
	TObjectPtr<class UHTN> PendingHTNAsset;

	UPROPERTY(Transient)
	TObjectPtr<class UHTNPlanInstance> RootPlanInstance;

	UPROPERTY(Transient)
	TMap<FHTNPlanInstanceID, TObjectPtr<class UHTNPlanInstance>> SubPlanInstances;

	// The proxy to the "current" worldstate.
	// When planning, proxies to the worldstate currently being processed by the planner.
	// This allows things like EQS Contexts to access future state instead of the current blackboard.
	// UHTNBlueprintLibrary functions (e.g. Get/SetWorldStateValueAsVector) use this proxy during planning.
	UPROPERTY()
	TObjectPtr<class UWorldStateProxy> PlanningWorldStateProxy;

	// The proxy to the BlackboardComponent of the AIController.
	// UHTNBlueprintLibrary functions(e.g.Get/SetWorldStateValueAsVector) use this proxy during plan execution.
	UPROPERTY()
	TObjectPtr<class UWorldStateProxy> BlackboardProxy;

	// The extensions of this component, owned by it and with the same lifetime as it.
	UPROPERTY()
	TMap<TSubclassOf<class UHTNExtension>, TObjectPtr<UHTNExtension>> Extensions;

	// We pool the UAITask_MakeHTNPlan used to make plans so that we don't create a fresh UObject 
	// that would need to get Garbage Collected later every time we need to make a plan..
	UPROPERTY()
	TArray<TObjectPtr<class UAITask_MakeHTNPlan>> PlanningTasksPool;

	// For each template node (node inside an HTN asset), we keep a pool of instances of it.
	// This way when a new plan starts and we need to create instances of nodes in it, 
	// we can reuse these instead of creating new ones.
	UPROPERTY()
	TMap<TObjectPtr<class UHTNNode>, FHTNNodeInstancePool> NodeInstancePools;
	
	friend class UHTNNode;
	friend class FHTNDebugger;
	friend struct FHTNComponentScopedLock;
	friend class UHTNPlanInstance;

#if USE_HTN_DEBUGGER
	mutable FHTNDebugSteps DebuggerSteps;
	static TArray<TWeakObjectPtr<UHTNComponent>> PlayingComponents;
#endif
};

template<class ExtensionType>
FORCEINLINE ExtensionType* UHTNComponent::FindExtension(TSubclassOf<UHTNExtension> ExtensionClass) const
{
	return StaticCast<ExtensionType*>(FindExtensionByClass(ExtensionClass));
}

template<class ExtensionType>
FORCEINLINE ExtensionType& UHTNComponent::FindOrAddExtension(TSubclassOf<UHTNExtension> ExtensionClass)
{
	ExtensionType* const Extension = StaticCast<ExtensionType*>(FindOrAddExtensionByClass(ExtensionClass));
	check(Extension);
	return *Extension;
}

template<typename PredicateType>
FORCEINLINE const UHTNPlanInstance* UHTNComponent::FindPlanInstanceBy(PredicateType&& Predicate) const
{
	if (Invoke(Predicate, *RootPlanInstance))
	{
		return RootPlanInstance;
	}

	for (const TPair<FHTNPlanInstanceID, TObjectPtr<UHTNPlanInstance>>& Pair : SubPlanInstances)
	{
		check(IsValid(Pair.Value));
		if (Invoke(Predicate, *Pair.Value))
		{
			return Pair.Value;
		}
	}

	return nullptr;
}

template<typename PredicateType>
FORCEINLINE UHTNPlanInstance* UHTNComponent::FindPlanInstanceBy(PredicateType&& Predicate)
{
	return const_cast<UHTNPlanInstance*>(const_cast<const ThisClass*>(this)->FindPlanInstanceBy(Predicate));
}

template<typename CallableType>
FORCEINLINE void UHTNComponent::ForEachPlanInstance(CallableType&& Callable) const
{
	Invoke(Callable, *ConstCast(RootPlanInstance));
	for (const TPair<FHTNPlanInstanceID, TObjectPtr<UHTNPlanInstance>>& Pair : SubPlanInstances)
	{
		check(IsValid(Pair.Value));
		Invoke(Callable, *ConstCast(Pair.Value));
	}
}

template<typename CallableType>
FORCEINLINE void UHTNComponent::ForEachPlanInstance(CallableType&& Callable)
{
	// The non-const version of ForEachPlanInstance handles cases where the Callable triggers replans, 
	// thus mutating the SubPlanInstances map (it's not safe to change a collection while iterating over it).
	TArray<TObjectPtr<UHTNPlanInstance>, TInlineAllocator<16>> SubInstances;
	SubPlanInstances.GenerateValueArray(SubInstances);

	Invoke(Callable, *RootPlanInstance);

	for (UHTNPlanInstance* SubInstance : SubInstances)
	{
		if (IsValid(SubInstance) && SubPlanInstances.Contains(SubInstance->GetID()))
		{
			Invoke(Callable, *SubInstance);
		}
	}
}
