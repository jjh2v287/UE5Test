// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once 

#include "HTNStandaloneNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "HTNExtension.h"
#include "HTNNode_SubNetworkDynamic.generated.h"

UENUM()
enum class EHTNNodeSubNetworkDynamicPlanAdjustmentMode : uint8
{
	// When trying to adjust the current plan, the plan is not allowed to diverge from the current plan at this node.
	// The planner will only try to plan through the same branch as in the current plan.
	// Even if the dynamic subnetwork was changed, the planner will go with the same subnetwork as in the current plan.
	NoAdjustmentAllowed,
	// When trying to adjust the current plan, the plan will try to diverge from the current plan
	// by using a new sunetwork that was possibly assigned to this node dynamically.
	// If that is the case, the planner will try to plan through the new subnetwork and mark such a plan as diverging from the current plan.
	// Plan adjustment only succeeds if a divergence like this happens at least at one node.
	TrySwitchToAnotherSubNetwork
};

// Like SubNetwork, but the HTN can be changed dynamically for each AI
// using the SetDynamicHTN function of HTNComponent.
UCLASS()
class HTN_API UHTNNode_SubNetworkDynamic : public UHTNStandaloneNode
{
	GENERATED_BODY()

public:
	UHTNNode_SubNetworkDynamic(const FObjectInitializer& Initializer);
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
	virtual UObject* GetAssetToOpenOnDoubleClick(const UHTNComponent* DebuggedHTNComponent) const override;
#endif

	UHTN* GetHTN(const UHTNComponent& OwnerComp, const class FBlackboardWorldState& WorldState) const;
	bool IsHTNValid(const UHTNComponent& OwnerComp, const UHTN* HTN, bool bLogIfNotValid = false) const;

	// The default subnetwork of this node. 
	// If there is no HTN listed under the InjectTag in the HTNComponent, this HTN will be used.
	// If None, planning will move on as if this node instantly succeeded.
	UPROPERTY(Category = Node, EditAnywhere)
	TObjectPtr<UHTN> DefaultHTN;

	UPROPERTY(Category = Node, EditAnywhere, Meta = (InlineEditConditionToggle))
	uint8 bTakeHTNFromWorldstateKey : 1;

	// The tag by which the node will find the HTN to use.
	// Call SetDynamicHTN on the HTNComponent to set the HTN for nodes with a specific tag. 
	UPROPERTY(Category = Node, EditAnywhere, Meta = (EditCondition = "!bTakeHTNFromWorldstateKey"))
	FGameplayTag InjectTag;

	// If enabled, the HTN will be taken from this key in the worldstate during planning.
	UPROPERTY(Category = Node, EditAnywhere, Meta = (EditCondition = "bTakeHTNFromWorldstateKey", DisplayName = "Take HTN From Worldstate Key"))
	FBlackboardKeySelector TakeHTNFromWorldstateKey;

	// Determines how this node can participate in the Plan Adjustment process, 
	// (i.e., when triggering a Replan with PlanningType == TryToAdjustExistingPlan).
	// See the tooltip for PlanningType TryToAdjustExistingPlan on the Replan node for more info.
	UPROPERTY(EditAnywhere, Category = Planning)
	EHTNNodeSubNetworkDynamicPlanAdjustmentMode PlanAdjustmentMode;
};

UCLASS()
class HTN_API UHTNExtension_SubNetworkDynamic : public UHTNExtension
{
	GENERATED_BODY()

public:
#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const override;
#endif

	// Returns the dynamic HTN override for the given injection tag.
	// Does not return the DefaultHTN of SubNetworkDynamic nodes, only what was set using SetDynamicHTN.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Dynamic HTN")
	UHTN* GetDynamicHTN(FGameplayTag InjectTag) const;

	// Assign an HTN asset to SubNetworkDynamic nodes with the given gameplay tag.
	// Returns true if the new HTN is different from the old one.
	// If bForceReplanIfChangedNodesInCurrentPlan is enabled, and the HTN of any SubNetworkDynamic nodes in the current plan was changed due to this call, Replan will be called.
	// If bForceReplanIfChangedDuringPlanning is enabled, the new value is different from the old one, and the HTNComponent is in the process of making a new plan, Replan will be called.
	// If force-replanning, bForceAbortCurrentPlanIfForcingReplan determines if the current plan should be aborted immediately or wait until a new plan is made.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Dynamic HTN", Meta = (ReturnDisplayName = "Value Was Changed"))
	bool SetDynamicHTN(FGameplayTag InjectTag, UHTN* HTN, 
		bool bForceReplanIfChangedNodesInCurrentPlan, bool bForceReplanIfChangedDuringPlanning, bool bForceAbortCurrentPlanIfForcingReplan);

	UFUNCTION(BlueprintCallable, Category = "AI|HTN|Dynamic HTN")
	void ResetDynamicHTNs();

private:
	// Maps from gameplay tags to HTN assets used by HTNNode_SubnetworkDynamic
	UPROPERTY(VisibleAnywhere, Category = "AI|HTN")
	TMap<FGameplayTag, TObjectPtr<UHTN>> GameplayTagToDynamicHTN;
};