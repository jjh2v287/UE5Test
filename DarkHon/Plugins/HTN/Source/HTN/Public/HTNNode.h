// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTN.h"
#include "HTNComponent.h"

#include "GameplayTaskOwnerInterface.h"
#include "Tasks/AITask.h"

#include "HTNNode.generated.h"

struct FHTNNodeSpecialMemory
{
	// The index of the plan-specific node instance in the UHTNPlanInstance::NodeInstances array.
	int32 NodeInstanceIndex = INDEX_NONE;
};

UENUM()
enum class EHTNNodeInstancePoolingMode : uint8
{
	// Whether or not this node will have its instances reused between plans is controlled by the 
	// "Enable Node Instance Pooling" setting in project settings (Project > Plugins > HTN).
	ProjectDefault,
	// Node instances of this node will be reused between plans to reduce the load on Garbage Collection.
	Enabled,
	// Node instances of this node will be destroyed when the plan they were created for is finished.
	Disabled
};

// The base class for runtime HTN nodes.
UCLASS(Abstract, config = Game)
class HTN_API UHTNNode : public UObject, public IGameplayTaskOwnerInterface
{
	GENERATED_BODY()

public:
	UHTNNode(const FObjectInitializer& ObjectInitializer);

#if WITH_ENGINE
	virtual UWorld* GetWorld() const override;
#endif
	
	// Initialization and setup
	virtual void InitializeFromAsset(class UHTN& Asset);
	// Allows nodes (in practice only BP nodes) to keep track of their owner.
	virtual void SetOwnerComponent(UHTNComponent* OwnerComp) const { OwnerComponent = OwnerComp; }
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	FORCEINLINE UHTNComponent* GetOwnerComponent() const { return OwnerComponent; }
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	FORCEINLINE class AAIController* GetOwnerController() const { return OwnerComponent ? OwnerComponent->GetAIOwner() : nullptr; }
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	APawn* GetControlledPawn() const;

	// Begin Instancing
	// Each node can be either instanced, or given a memory block of specified size to put custom state in.
	// The latter is more efficient and is the default. See bCreateNodeInstance

	// Returns the size of the main memory block this node needs during plan execution. 
	virtual uint16 GetInstanceMemorySize() const { return 0; }
	// Returns the size of the memory block this node needs for internals.
	virtual uint16 GetSpecialMemorySize() const { return bCreateNodeInstance ? sizeof(FHTNNodeSpecialMemory) : 0; }

	// Only relevant for nodes that have node instancing *and* node instance pooling enabled.
	// Called when a node instance returns to HTNComponent's node instance pool to be reused in a future plan 
	// (which reduces the load on Garbage Collection)
	// Use this function to reset the values of member variables to the values of the template node in the HTN asset
	// from which this node instance was created (GetTemplateNode()).
	virtual void OnInstanceReturnedToPool(UHTNComponent& OwnerComp);

protected:
	virtual void InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const struct FHTNPlan& Plan, const FHTNPlanStepID& StepID) const;
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const struct FHTNPlan& Plan, const FHTNPlanStepID& StepID) const {}
	virtual void CleanupSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const {}
	virtual void CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const {}

public:
	FORCEINLINE bool HasInstance() const { return bCreateNodeInstance; }
	FORCEINLINE bool IsInstance() const { return TemplateNode != nullptr; }
	bool ShouldPoolNodeInstances() const;
	
	void InitializeInPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const struct FHTNPlan& Plan, const FHTNPlanStepID& StepID, 
		TArray<TObjectPtr<UHTNNode>>& OutNodeInstances) const;
	void CleanupInPlan(UHTNComponent& OwnerComp, uint8* NodeMemory) const;
	
	UHTNNode* GetNodeFromMemory(const UHTNComponent& OwnerComp, const uint8* NodeMemory) const;
	FORCEINLINE UHTNNode* GetTemplateNode() { return IsInstance() ? TemplateNode.Get() : this; }

	// Returns the node from the actual HTN asset, not one that was possibly instantiated for plan execution. 
	// If the node is not instanced, just returns self.
	UFUNCTION(BlueprintPure, Category = "AI|HTN")
	FORCEINLINE const UHTNNode* GetTemplateNode() const { return IsInstance() ? TemplateNode.Get() : this; }

	template<typename T>
	T* CastInstanceNodeMemory(uint8* NodeMemory) const;
	template<typename T>
	const T* CastInstanceNodeMemory(const uint8* NodeMemory) const;
	
	template<typename T>
	T* GetSpecialNodeMemory(uint8* NodeMemory) const;
	template<typename T>
	const T* GetSpecialNodeMemory(const uint8* NodeMemory) const;
	// End Instancing

	void WrappedOnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory) const;
	void WrappedOnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result) const;

protected:
	virtual void OnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory) {};
	virtual void OnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result) {};

public:
	UFUNCTION(BlueprintPure, Category = Description)
	virtual FString GetStaticDescription() const;

	UFUNCTION(BlueprintPure, Category = Description)
	virtual FString GetNodeName() const;

	// A short description suitable for prefixing debug logs with.
	UFUNCTION(BlueprintPure, Category = Description)
	virtual FString GetShortDescription() const;

	FORCEINLINE UHTN* GetHTNAsset() const { return HTNAsset; }
	FORCEINLINE class UBlackboardData* GetBlackboardAsset() const { return HTNAsset ? HTNAsset->BlackboardAsset : nullptr; }
	UHTN* GetSourceHTNAsset() const;

#if WITH_EDITOR
	// Get the name of the icon used to display this node in the editor
	virtual FName GetNodeIconName() const { return FName(); }
	virtual UObject* GetAssetToOpenOnDoubleClick(const UHTNComponent* DebuggedHTNComponent) const { return nullptr; }
#endif
	
	// Begin IGameplayTaskOwnerInterface
	virtual UGameplayTasksComponent* GetGameplayTasksComponent(const UGameplayTask& Task) const override;
	virtual AActor* GetGameplayTaskOwner(const UGameplayTask* Task) const override;
	virtual AActor* GetGameplayTaskAvatar(const UGameplayTask* Task) const override;
	virtual uint8 GetGameplayTaskDefaultPriority() const override;
	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;
	// End IGameplayTaskOwnerInterface

	UHTNComponent* GetHTNComponentByTask(UGameplayTask& Task) const;
	
	template<class T>
	T* NewHTNAITask(const UHTNComponent& HTNComponent);

	static FString GetSubStringAfterUnderscore(const FString& Input);

	UPROPERTY(EditAnywhere, Category = Description)
	FString NodeName;

	// When a new plan is created, Blueprint nodes (and C++ nodes that enabled bCreateNodeInstance)
	// from the HTN graph are duplicated to be used in the plan. This increases the load on Garbage Collection, 
	// especially when making plans frequently. Enabling node instance pooling allows such node instances 
	// to be pooled on the HTNComponent, making it possible to reuse them when creating new plans.
	// 
	// By default, when returning a node instance to the pool after its plan is over,
	// the node's properties and local variables (e.g., inside DoOnce nodes etc.) will be automatically reset 
	// to values from the template node in the HTN asset from which the node instance was created. 
	// This behavior can be augmented by overriding the OnInstanceReturnedToPool function.
	UPROPERTY(EditAnywhere, Category = Node, Meta = (EditCondition = "bCreateNodeInstance", EditConditionHides, HideEditConditionToggle))
	EHTNNodeInstancePoolingMode NodeInstancePoolingMode;

#if WITH_EDITORONLY_DATA
	// The index of the graph node of this runtime node in the graph.
	UPROPERTY()
	int32 NodeIndexInGraph;
#endif
	
protected:
	// Forces the HTN component running this node to start making a new plan. 
	// By default waits until the new plan is ready before aborting the current plan, but can be configured otherwise.
	void Replan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNReplanParameters& Params = FHTNReplanParameters::Default) const;

	struct FGuardOwnerComponent : private FNoncopyable
	{
		FGuardOwnerComponent(const UHTNNode& Node, UHTNComponent* NewOwner);
		~FGuardOwnerComponent();

		const UHTNNode& Node;
		UHTNComponent* OldOwner;
	};

	// If this node was instanced from a template node, this will point to that template node.
	UPROPERTY()
	TObjectPtr<UHTNNode> TemplateNode;
	
	// If set, node will be instanced (duplicated) for each plan instead of using a memory block.
	// Creating node instances is much slower than using a memory block, but is necessary for Blueprint-implemented nodes.
	UPROPERTY()
	uint8 bCreateNodeInstance : 1;

	// Set to true if the node owns any GameplayTasks. Note this requires tasks to be created via NewHTNAITask
    // Otherwise the specific HTN node class is responsible for ending the gameplay tasks on execution finish.
	uint8 bOwnsGameplayTasks : 1;

	uint8 bNotifyOnPlanExecutionStarted : 1;
	uint8 bNotifyOnPlanExecutionFinished : 1;

	// If set, UHTNNodeLibrary::GetOwnersWorldState(UHTNNode*) will always return a 
	// proxy to the planning worldstate instead of the blackboard.
	mutable uint8 bForceUsingPlanningWorldState : 1;
	friend class UHTNNodeLibrary;

private:
	// The source asset of this node
	UPROPERTY()
	TObjectPtr<UHTN> HTNAsset;

	// Only used by BP subclasses to get the current worldstate during planning
	UPROPERTY(Transient)
	mutable TObjectPtr<UHTNComponent> OwnerComponent;
};

template<typename T>
T* UHTNNode::CastInstanceNodeMemory(uint8* NodeMemory) const
{
	// Using '<=' rather than '==' to allow child classes to extend parent classes'
	// memory struct as well (which would make GetInstanceMemorySize() return 
	// sizeof(FChildNodeMemory) which is >= sizeof(FBaseNodeMemory)).
	checkf(sizeof(T) <= GetInstanceMemorySize(), 
		TEXT("Requesting type of %zu bytes but GetInstanceMemorySize returns %u. Make sure GetInstanceMemorySize is implemented properly in the class hierarchy of %s."), 
		sizeof(T), GetInstanceMemorySize(), *GetShortDescription());
	return reinterpret_cast<T*>(NodeMemory);
}

template<typename T>
const T* UHTNNode::CastInstanceNodeMemory(const uint8* NodeMemory) const
{
	return CastInstanceNodeMemory<T>(const_cast<uint8*>(NodeMemory));
}

template<typename T>
T* UHTNNode::GetSpecialNodeMemory(uint8* NodeMemory) const
{
	const int32 SpecialMemorySize = GetSpecialMemorySize();
	return SpecialMemorySize ? reinterpret_cast<T*>(NodeMemory - ((SpecialMemorySize + 3) & ~3)) : nullptr;
}

template<typename T>
const T* UHTNNode::GetSpecialNodeMemory(const uint8* NodeMemory) const
{
	return GetSpecialNodeMemory<T>(const_cast<uint8*>(NodeMemory));
}

template<class T>
T* UHTNNode::NewHTNAITask(const UHTNComponent& HTNComponent)
{
	check(HTNComponent.GetAIOwner());
	bOwnsGameplayTasks = true;
	return UAITask::NewAITask<T>(*HTNComponent.GetAIOwner(), *this, TEXT("Behavior"));
}
