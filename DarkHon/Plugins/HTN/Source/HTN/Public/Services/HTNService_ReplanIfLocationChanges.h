// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HTNService.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "HTNService_ReplanIfLocationChanges.generated.h"

// If the location of the specified blackboard key changes too much from what it was at the beginning of execution, forces a replan.
UCLASS()
class HTN_API UHTNService_ReplanIfLocationChanges : public UHTNService
{
	GENERATED_BODY()

public:
	// Changes below this value will be ignored.
	UPROPERTY(EditAnywhere, Category = Node, Meta = (ClampMin = 0, ForceUnits = cm))
	float Tolerance;

	UPROPERTY(EditAnywhere, Category = Node)
	FBlackboardKeySelector BlackboardKey;

	UPROPERTY()
	bool bForceAbortPlan_DEPRECATED;

	UPROPERTY()
	bool bForceRestartActivePlanning_DEPRECATED;

	UPROPERTY(EditAnywhere, Category = Node, Meta = (ShowOnlyInnerProperties))
	FHTNReplanParameters ReplanParameters;

	UHTNService_ReplanIfLocationChanges(const FObjectInitializer& Initializer);
	
protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const override;

	virtual FString GetStaticDescription() const override;
	
	virtual void OnExecutionStart(UHTNComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) override;

private:
	bool GetLocation(UHTNComponent& OwnerComp, FVector& OutLocation) const;
	bool SetInitialLocation(UHTNComponent& OwnerComp, uint8* NodeMemory) const;
	
	struct FMemory
	{
		TOptional<FVector> InitialLocation;
	};
};