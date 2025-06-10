// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "HTNGraphNode.h"
#include "HTNGraphNode_Service.generated.h"

UCLASS()
class UHTNGraphNode_Service : public UHTNGraphNode
{
	GENERATED_BODY()
	
public:
	UHTNGraphNode_Service(const FObjectInitializer& Initializer);
	virtual void AllocateDefaultPins() override;
	virtual bool CanPlaceBreakpoints() const override { return false; }
};
