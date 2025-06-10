// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNNode_TwoBranches.h"
#include "HTNNode_Parallel.generated.h"

// A node that plans to execute both of its branches at the same time.
// The bottom branch is secondary to the top one and will be aborted once the top branch completes, unless configured otherwise.
// The secondary branch can also be configured to continuously repeat execution until the top branch completes.
// Note: the secondary branch does not contribute to the worldstate used by the rest of the plan, 
// but the tasks *within* the secondary branch can still influence each other during planning through the worldstate.
UCLASS()
class HTN_API UHTNNode_Parallel : public UHTNNode_TwoBranches
{
	GENERATED_BODY()

public:
	UHTNNode_Parallel(const FObjectInitializer& Initializer);
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual bool OnSubLevelFinishedPlanning(FHTNPlan& Plan, const FHTNPlanStepID& ThisStepID, int32 SubLevelIndex, TSharedPtr<FBlackboardWorldState> WorldState) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;
	virtual bool OnSubLevelFinished(UHTNPlanInstance& PlanInstance, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID, int32 FinishedSubLevelIndex) override;

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FNodeMemory); }
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;
	
	virtual FString GetStaticDescription() const override;
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif

	struct FNodeMemory
	{
		FNodeMemory();

		uint8 bIsPrimaryBranchComplete : 1;
		uint8 bIsSecondaryBranchComplete : 1;
		uint8 bIsExecutionComplete : 1;
		mutable uint8 bSecondaryBranchReentryFlag : 1;
	};
	
	// If true, the secondary branch won't be immediately aborted once the primary branch completes execution.
	UPROPERTY(EditAnywhere, Category = Execution)
	uint8 bWaitForSecondaryBranchToComplete : 1;

	// If true, the secondary branch will keep repeating until the primary branch completes execution.
	UPROPERTY(EditAnywhere, Category = Execution)
	uint8 bLoopSecondaryBranchUntilPrimaryBranchCompletes : 1;
};
