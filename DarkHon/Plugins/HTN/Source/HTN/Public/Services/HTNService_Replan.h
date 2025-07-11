// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNService.h"
#include "HTNService_Replan.generated.h"

// Keeps triggering a replan at some interval. 
// Skips the replanning if a replan is already in progress.
// This is useful for triggering a "try adjust current plan" sort of replan periorically.
UCLASS()
class HTN_API UHTNService_Replan : public UHTNService
{
	GENERATED_BODY()

public:
	UHTNService_Replan(const FObjectInitializer& Initializer);

	UPROPERTY(EditAnywhere, Category = Node, Meta = (ShowOnlyInnerProperties))
	FHTNReplanParameters Parameters;
	
protected:
	virtual FString GetNodeName() const override;
	virtual FString GetStaticDescription() const override;
	
	virtual void TickNode(UHTNComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) override;	
};
