// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HTNTask.h"
#include "HTNTask_BlueprintBase.generated.h"

UENUM()
enum class EHTNTaskFunction : uint8
{
	None,
	CreatePlanSteps,
	RecheckPlan,
	Execute,
	Abort
};

// Base class for blueprint based HTN task nodes. Do NOT use it for creating native C++ classes!
UCLASS(Abstract, Blueprintable)
class HTN_API UHTNTask_BlueprintBase : public UHTNTask
{
	GENERATED_BODY()

public:
	UHTNTask_BlueprintBase(const FObjectInitializer& Initializer);
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual FString GetStaticDescription() const override;

	// Check if the task is currently being executed
	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	bool IsTaskExecuting() const;

	// Check if the task is currently being aborted
	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	bool IsTaskAborting() const;
	
protected:
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	virtual void CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;
	virtual bool RecheckPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FBlackboardWorldState& WorldState, const FHTNPlanStep& SubmittedPlanStep) override;

	virtual EHTNNodeResult ExecuteTask(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlanStepID& PlanStepID) override;
	virtual EHTNNodeResult AbortTask(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;
	virtual void OnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result) override;
	virtual void LogToVisualLog(UHTNComponent& OwnerComp, const uint8* NodeMemory, const FHTNPlanStep& SubmittedPlanStep) const override;

	// Forces the HTN component running this node to start making a new plan. 
	// By default waits until the new plan is ready before aborting the current plan, but can be configured otherwise.
	UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "AI|HTN", Meta = (DisplayName = "Replan", AutoCreateRefTerm = "Params"))
	void BP_Replan(const FHTNReplanParameters& Params) const;

	// Called during planning on the task object in the HTN asset.
	// Check and/or modify information in the worldstate to produce any number of plan steps via SubmitPlanStep.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveCreatePlanSteps(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn) const;

	// Called on a task instance inside a plan while verifying if the plan is still valid.
	// Check values in the worldstate and return true if the task can still be executed.
	// This is necessary because values in the worldstate at this point in the plan might be different due to changing conditions.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	bool ReceiveRecheckPlan(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn);

	// Entry point for execution, task will stay active until FinishExecute is called
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveExecute(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn);

	// If overriden, task will stay active until FinishAbort is called. Otherwise the task will complete immediately when prompted to abort.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveAbort(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn);

	// Tick function, called as long as the task is executing
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveTick(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn, float DeltaSeconds);

	// Called when the task definitively finished executing, either successfully or due to being aborted
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveOnFinished(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn, EHTNNodeResult Result);

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

	// Called per frame on tasks that are in the future of the current plan (not executing yet but will be in the future).
	// Use this to log shapes to the visual logger on the HTNCurrentPlan category (given as parameter) to provide a visual representation of the current plan.
	// The worldstate in this context is the way it was submitted with SubmitPlanStep during planning. 
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|HTN")
	void ReceiveDescribePlanStepToVisualLog(AActor* Owner, AAIController* OwnerController, APawn* ControlledPawn, FName VisLogCategoryName) const;
	
	// Call during planning to submit a plan step with the currently modified worldstate.
	// The description will appear in the visual logger next to the node name. Useful for debugging.
	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	void SubmitPlanStep(int32 Cost = 100, const FString& Description = TEXT("")) const;

	// Call this during planning if you don't create any plan steps and want to indicate the reason why to the visual logger. 
	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	void SetPlanningFailureReason(const FString& FailureReason) const;

	// Following ReceiveExecute, finishes task execution with Success or Fail result
	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	void FinishExecute(bool bSuccess);

	// Following ReceiveAbort, finishes aborting task execution
	UFUNCTION(BlueprintCallable, Category = "AI|HTN")
	void FinishAbort();

	// Set on node instances so we can pass it into functions that require it.
	mutable uint8* CachedNodeMemory;

	mutable EHTNTaskFunction CurrentlyExecutedFunction;
	// The result that's set when calling FinishExecute/FinishAbort from inside ExecuteTask/AbortTask
	mutable EHTNNodeResult CurrentCallResult;

	// Intermediate values only used during CreatePlanSteps/SubmitPlanStep
	mutable TSharedPtr<const FBlackboardWorldState> OldWorldState;
	mutable TSharedPtr<FBlackboardWorldState> NextWorldState;
	UPROPERTY(Transient)
	mutable TObjectPtr<UAITask_MakeHTNPlan> OutPlanningTask;

	// If any of the Tick functions is implemented, how ofter should they be ticked.
	// Values < 0 mean 'every tick'.
	UPROPERTY(EditAnywhere, Category = Task)
	FIntervalCountdown TickInterval;

	// Show detailed information about properties
	UPROPERTY(EditInstanceOnly, Category = Description)
	uint8 bShowPropertyDetails : 1;
	
	// Property data for showing description
	TArray<FProperty*> PropertyData;

	uint8 bImplementsCreatePlanSteps : 1;
	uint8 bImplementsRecheckPlan : 1;
	uint8 bImplementsExecute : 1;
	uint8 bImplementsAbort : 1;
	uint8 bImplementsTick : 1;
	uint8 bImplementsOnFinished : 1;
	uint8 bImplementsOnPlanExecutionStarted : 1;
	uint8 bImplementsOnPlanExecutionFinished : 1;
	uint8 bImplementsLogToVisualLog : 1;
	
	uint8 bIsAborting : 1;
};
