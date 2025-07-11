// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_Decorator.generated.h"

UCLASS()
class UHTNGraphNode_Decorator : public UHTNGraphNode
{
	GENERATED_BODY()
	
public:
	UHTNGraphNode_Decorator(const FObjectInitializer& Initializer);
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void AllocateDefaultPins() override;
	virtual bool CanPlaceBreakpoints() const override { return false; }
};
