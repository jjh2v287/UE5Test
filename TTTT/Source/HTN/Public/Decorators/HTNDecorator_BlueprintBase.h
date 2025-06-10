// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HTNDecorator.h"
#include "AIController.h"
#include "HTNDecorator_BlueprintBase.generated.h"

// Base class for blueprint based HTN decorator nodes. Do NOT use it for creating native C++ classes!
UCLASS(Abstract, Blueprintable)
class HTN_API UHTNDecorator_BlueprintBase : public UHTNDecorator
{
	GENERATED_BODY()

public:
	UHTNDecorator_BlueprintBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual FString GetStaticDescription() const override;
	
protected:
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	virtual void CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual bool CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const override;
	virtual void ModifyStepCost(UHTNComponent& OwnerComp, FHTNPlanStep& Step) const override;

	virtual void OnEnterPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	virtual void OnExitPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;

	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) override;
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;
	
	virtual void OnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result) override;
	
	// Forces the HTN component running this node to start making a new plan. 
	// By default waits until the new plan is ready before aborting the current plan, but can be configured otherwise.
	UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "AI|HTN", Meta = (DisplayName = "Replan", AutoCreateRefTerm = "Params"))
	void BP_Replan(const FHTNReplanParameters& Params) const;

	// Call this if the condition changed as the result of an event.
	// Returns true if this resulted in an replan.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN", Meta = (DisplayName = "Notify Event Based Condition", ReturnDisplayName = "Caused Replan"))
	bool BP_NotifyEventBasedCondition(bool bRawConditionValue, bool bCanAbortPlanInstantly = true);

	// Called when testing if the underlying node can be added to the plan or executed.
	// The CheckType parameter indicates what kind of check it is: during planning, during execution etc.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	bool PerformConditionCheck(AActor* Owner, AAIController* OwnerAsController, APawn* ControlledPawn, EHTNDecoratorConditionCheckType CheckType) const;

	// During planning, this can be used to modify the cost of the plan step this decorator is on.
	// Return the new cost of the plan step (must be non-negative).
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	int32 ReceiveModifyStepCost(int32 OriginalCost, AActor* Owner, AAIController* OwnerAsController, APawn* ControlledPawn) const;

	// This provides an opportunity to change worldstate values during planning, before entering the underlying task or subnetwork.
	// Only called if the condition check passed (or regardless if the decorator is on an If node and planning took the False branch).
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveOnPlanEnter(AActor* Owner, AAIController* OwnerAsController, APawn* ControlledPawn) const;

	// This provides an opportunity to change worldstate values during planning, after exiting the underlying task or subnetwork.
	// Only called if the condition check passed (or regardless if the decorator is on an If node and planning took the False branch).
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveOnPlanExit(AActor* Owner, AAIController* OwnerAsController, APawn* ControlledPawn) const;

	// Called when execution of the underlying task or subnetwork begins.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveExecutionStart(AActor* Owner, AAIController* OwnerAsController, APawn* ControlledPawn);

	// Tick function, called for as long as the underlying task or subnetwork is executing.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveTick(AActor* Owner, AAIController* OwnerAsController, APawn* ControlledPawn, float DeltaTime);

	// Called when execution of the underlying task or subnetwork finishes.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveExecutionFinish(AActor* Owner, AAIController* OwnerAsController, APawn* ControlledPawn, EHTNNodeResult NodeResult);

	// Called when a plan containing this node begins executing.
	// Together with ReceiveOnPlanExecutionFinished, this can be used to lock resources or notify other characters/systems 
	// about what the AI indends to do in the plan before this node actually begins executing.
	// (e.g. reserve a specific movement target to prevent others from moving to it).
	// 
	// NOTE: If called from inside this function, the GetWorldStateValue functions will work with the worldstate with which this plan step finished planning. 
	// That worldstate cannot be modified further from this function.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveOnPlanExecutionStarted(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn);

	// Called when a plan containing this node finishes executing, for any reason.
	// This is called even if the plan was aborted before this node could execute.
	// 
	// NOTE: If called from inside this function, the GetWorldStateValue functions will work with the worldstate with which this plan step finished planning. 
	// That worldstate cannot be modified further from this function.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveOnPlanExecutionFinished(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn, EHTNPlanExecutionFinishedResult Result);

	// Show detailed information about properties
	UPROPERTY(EditInstanceOnly, Category = Description)
	uint8 bShowPropertyDetails : 1;

	// Property data for showing description
	TArray<FProperty*> PropertyData;
	
	uint8 bImplementsPerformConditionCheck : 1;
	uint8 bImplementsModifyStepCost : 1;
	uint8 bImplementsOnPlanEnter : 1;
	uint8 bImplementsOnPlanExit : 1;
	uint8 bImplementsOnExecutionStart : 1;
	uint8 bImplementsTick : 1;
	uint8 bImplementsOnExecutionFinish : 1;
	uint8 bImplementsOnPlanExecutionStarted : 1;
	uint8 bImplementsOnPlanExecutionFinished : 1;

	// Set on node instances so we can pass it into functions that require it.
	mutable uint8* CachedNodeMemory;
};