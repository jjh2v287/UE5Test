// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNNode.h"
#include "HTNPlanStep.h"
#include "HTNTypes.h"
#include "HTNDecorator.generated.h"

// A subnode used for conditions, plan cost modification, scoping etc.
UCLASS(Abstract)
class HTN_API UHTNDecorator : public UHTNNode
{
	GENERATED_BODY()

public:
	UHTNDecorator(const FObjectInitializer& Initializer);
	virtual void Serialize(FArchive& Ar) override;

	virtual uint16 GetSpecialMemorySize() const { return sizeof(FHTNDecoratorSpecialMemory); }
	virtual FString GetStaticDescription() const override;

	void WrappedEnterPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const;
	void WrappedModifyStepCost(UHTNComponent& OwnerComp, FHTNPlanStep& Step) const;
	void WrappedExitPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const;
	
	bool WrappedRecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStep& SubmittedPlanStep) const;
	void WrappedExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) const;
	void WrappedTickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) const;
	void WrappedExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult NodeResult) const;

	EHTNDecoratorTestResult WrappedTestCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const;
	EHTNDecoratorTestResult TestCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const;
	EHTNDecoratorTestResult GetLastEffectiveConditionValue(const uint8* NodeMemory) const;
	virtual bool ShouldCheckCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const;

	UFUNCTION(BlueprintPure, Category = AI)
	FORCEINLINE bool IsInversed() const { return bInverseCondition; }

protected:
	virtual void InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const;

	// Note that NodeMemory will be nullptr during plan-time checks, as memory blocks are only allocated once a plan is selected for execution.
	virtual bool CalculateRawConditionValue(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNDecoratorConditionCheckType CheckType) const { return true; }
	virtual void OnEnterPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const {}
	virtual void ModifyStepCost(UHTNComponent& OwnerComp, FHTNPlanStep& Step) const {}
	virtual void OnExitPlan(UHTNComponent& OwnerComp, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const {}
	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) {}
	virtual void TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) {}
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) {}

	FString GetConditionCheckDescription() const;
	FString GetTickFunctionIntervalDescription() const;
	float GetConditionCheckInterval() const;
	float GetTickFunctionInterval() const;

	static UWorldStateProxy* GetWorldStateProxy(UHTNComponent& OwnerComp, EHTNDecoratorConditionCheckType CheckType);

	void SetLastEffectiveConditionValue(uint8* NodeMemory, EHTNDecoratorTestResult Value) const;

	// Call this from a decorator that checks its condition in an event-based manner when it checks the condition in its event.
	// Takes the raw condition value, as returned from CalculateRawConditionValue.
	// Returns true if this resulted in an replan.
	bool NotifyEventBasedCondition(UHTNComponent& OwnerComp, uint8* NodeMemory, bool bRawConditionValue, bool bCanAbortPlanInstantly = true) const;

	struct FHTNDecoratorSpecialMemory : public FHTNNodeSpecialMemory
	{
		FHTNDecoratorSpecialMemory(float ConditionCheckInterval, float TickFunctionInterval);

		// The result of the last Execution condition evaluation or condition notification event, 
		// cached to avoid re-evaluation during event-based processing of decorator conditions.
		// 
		// This is useful for deciding if the False branch of an If node should aborted when conditions change. 
		// All decorators must turn to true for the False branch to be aborted, so caching the result of that evaluation
		// prevents reevaluating all decorators on the If whenever one decorator notifies of its condition.
		//
		// This is also used to prevent excessive evaluation on tick if bCheckConditionOnTickOnlyOnce is true.
		EHTNDecoratorTestResult LastEffectiveConditionValue = EHTNDecoratorTestResult::NotTested;

		FIntervalCountdown ConditionCheckCountdown;

		FIntervalCountdown TickFunctionCountdown;

		uint8 bFirstTick : 1;
	};

	uint8 bNotifyOnEnterPlan : 1;
	uint8 bModifyStepCost : 1;
	uint8 bNotifyOnExitPlan : 1;
	uint8 bNotifyExecutionStart : 1;
	uint8 bNotifyTick : 1;
	uint8 bNotifyExecutionFinish : 1;
	
	// If set, condition check result will be inversed
	UPROPERTY(Category = Condition, EditAnywhere)
	uint8 bInverseCondition : 1;

	UPROPERTY(Category = Condition, EditAnywhere)
	uint8 bCheckConditionOnPlanEnter : 1;

	UPROPERTY(Category = Condition, EditAnywhere)
	uint8 bCheckConditionOnPlanExit : 1;

	UPROPERTY(Category = Condition, EditAnywhere)
	uint8 bCheckConditionOnPlanRecheck : 1;

	// Whether the condition should be checked on tick. Note that how often it is checked is controlled by ConditionCheckInterval and not TickFunctionInterval.
	UPROPERTY(Category = Condition, EditAnywhere)
	uint8 bCheckConditionOnTick : 1;

	// If this and CheckConditionOnTick are true, the condition will only be checked on the first tick (when starting execution).
	// This is useful for nodes that rely on events to notify of their condition instead of checking on tick (such as the Blackboard Decorator).
	UPROPERTY(Category = Condition, EditAnywhere, Meta = (EditCondition = "bCheckConditionOnTick"))
	uint8 bCheckConditionOnTickOnlyOnce : 1;

	// During execution, how often should the condition be checked? Values less than or equal to zero will result in the condition being checked every tick.
	// Note that this is independent of TickFunctionInterval.
	UPROPERTY(EditAnywhere, Category = Condition, Meta = (ForceUnits = s, EditCondition = "bCheckConditionOnTick && !bCheckConditionOnTickOnlyOnce"))
	float ConditionCheckInterval;

	// A random value between -this and this is added to ConditionCheckInterval to get the actual interval at which the condition is checked.
	UPROPERTY(EditAnywhere, Category = Condition, Meta = (ClampMin = "0.0", ForceUnits = s), Meta = (EditCondition = "bCheckConditionOnTick && !bCheckConditionOnTickOnlyOnce && ConditionCheckInterval > 0"))
	float ConditionCheckIntervalRandomDeviation;

	// How often should the Tick function be called during execution? Values less than or equal to zero will result in Tick being called every frame.
	// Note that this is independent of ConditionCheckInterval.
	UPROPERTY(EditAnywhere, Category = Node, Meta = (ForceUnits = s))
	float TickFunctionInterval;

	// A random value between -this and this is added to TickFunctionInterval to get the actual interval at which the condition is checked.
	UPROPERTY(EditAnywhere, Category = Node, Meta = (ClampMin = "0.0", ForceUnits = s), Meta = (EditCondition = "TickFunctionInterval > 0"))
	float TickFunctionIntervalRandomDeviation;
};

FORCEINLINE UWorldStateProxy* UHTNDecorator::GetWorldStateProxy(UHTNComponent& OwnerComp, EHTNDecoratorConditionCheckType CheckType)
{
	return OwnerComp.GetWorldStateProxy(/*bForPlanning=*/CheckType != EHTNDecoratorConditionCheckType::Execution);
}
