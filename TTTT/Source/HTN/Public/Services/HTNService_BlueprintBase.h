// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNService.h"
#include "AIController.h"
#include "HTNService_BlueprintBase.generated.h"

// Base class for blueprint based service nodes. Do NOT use it for creating native C++ classes!
UCLASS(Abstract, Blueprintable)
class HTN_API UHTNService_BlueprintBase : public UHTNService
{
	GENERATED_BODY()

public:
	UHTNService_BlueprintBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual FString GetStaticDescription() const override;
	
protected:
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	virtual void CleanupMemory(UHTNComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) override;
	virtual void OnExecutionFinish(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNNodeResult Result) override;

	virtual void OnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result) override;
	
	// Forces the HTN component running this node to start making a new plan. 
	// By default waits until the new plan is ready before aborting the current plan, but can be configured otherwise.
	UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "AI|HTN", Meta = (DisplayName = "Replan", AutoCreateRefTerm = "Params"))
	void BP_Replan(const FHTNReplanParameters& Params) const;

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
	
	uint8 bImplementsOnExecutionStart : 1;
	uint8 bImplementsTick : 1;
	uint8 bImplementsOnExecutionFinish : 1;
	uint8 bImplementsOnPlanExecutionStarted : 1;
	uint8 bImplementsOnPlanExecutionFinished : 1;

	// Set on node instances so we can pass it into functions that require it.
	mutable uint8* CachedNodeMemory;
};