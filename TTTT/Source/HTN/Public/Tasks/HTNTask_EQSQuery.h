// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/HTNTask_BlackboardBase.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/EQSParametrizedQueryExecutionRequestHTN.h"

#include "HTNTask_EQSQuery.generated.h"

// Runs an EQS query during planning and puts the result in the worldstate.
// Can be configured to output multiple results, which creates multiple branching plans.
UCLASS()
class HTN_API UHTNTask_EQSQuery : public UHTNTask_BlackboardBase
{
	GENERATED_BODY()

	UHTNTask_EQSQuery(const FObjectInitializer& Initializer);
	virtual void Serialize(FArchive& Ar) override;
	virtual void InitializeFromAsset(UHTN& Asset) override;
	virtual void CreatePlanSteps(UHTNComponent& OwnerComp, UAITask_MakeHTNPlan& PlanningTask, const TSharedRef<const FBlackboardWorldState>& WorldState) const override;

	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;
#if WITH_EDITOR
	// Update query params
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual FName GetNodeIconName() const override;
	virtual UObject* GetAssetToOpenOnDoubleClick(const UHTNComponent* DebuggedHTNComponent) const override;
#endif

	bool StoreInWorldState(class UEnvQueryItemType* ItemTypeCDO, const struct FBlackboardKeySelector& KeySelector, FBlackboardWorldState& WorldState, const uint8* RawData) const;
	
public:
	UPROPERTY(Category = EQS, EditAnywhere)
	FEQSParametrizedQueryExecutionRequestHTN EQSRequest;

	// If the RunMode is set to AllMatching, the node will produce multiple candidate plan steps from the top items of the query result.
	// This variable limits how many possible branches this node can produce.
	// For example, if the RunMode is AllMatching, this variable is 5, and the query produces 15 items, the top 5 of them will be used to create 5 alternative plan steps.
	// 0 means no limit.
	UPROPERTY(Category = Planning, EditAnywhere, Meta = (ClampMin = "0"))
	int32 MaxNumCandidatePlans;

	// The planning cost of this task.
	UPROPERTY(EditAnywhere, Category = Planning, Meta = (ClampMin = "0"))
	int32 Cost;
};