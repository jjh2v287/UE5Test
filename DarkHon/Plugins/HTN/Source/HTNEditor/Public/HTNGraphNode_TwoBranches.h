// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_TwoBranches.generated.h"

UCLASS()
class UHTNGraphNode_TwoBranches : public UHTNGraphNode
{
	GENERATED_BODY()
	
public:
	UHTNGraphNode_TwoBranches(const FObjectInitializer& Initializer);
	virtual void AllocateDefaultPins() override;
};
