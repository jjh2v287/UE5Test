// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "HTNStandaloneNode.h"
#include "HTNNode_SubNetwork.generated.h"

UCLASS()
class HTN_API UHTNNode_SubNetwork : public UHTNStandaloneNode
{
	GENERATED_BODY()

public:
	virtual FString GetStaticDescription() const override;
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;

	virtual FString GetNodeName() const override;
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
	virtual UObject* GetAssetToOpenOnDoubleClick(const UHTNComponent* DebuggedHTNComponent) const override;
#endif
	
	// The subnetwork of this node.
	UPROPERTY(Category = Node, EditAnywhere)
	TObjectPtr<UHTN> HTN;
};
