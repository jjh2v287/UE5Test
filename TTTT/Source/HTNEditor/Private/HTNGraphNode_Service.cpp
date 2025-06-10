// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNGraphNode_Service.h"

UHTNGraphNode_Service::UHTNGraphNode_Service(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bIsSubNode = true;
}

void UHTNGraphNode_Service::AllocateDefaultPins()
{
	// No pins for services
}
