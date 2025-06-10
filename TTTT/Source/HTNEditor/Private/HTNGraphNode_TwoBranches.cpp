// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNGraphNode_TwoBranches.h"
#include "HTNGraphPinCategories.h"

UHTNGraphNode_TwoBranches::UHTNGraphNode_TwoBranches(const FObjectInitializer& Initializer) : Super(Initializer)
{}

void UHTNGraphNode_TwoBranches::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, FHTNGraphPinCategories::MultipleNodesAllowed, TEXT("In"));

	CreatePin(EGPD_Output, FHTNGraphPinCategories::MultipleNodesAllowed, TEXT("OutPrimary"));
	CreatePin(EGPD_Output, FHTNGraphPinCategories::MultipleNodesAllowed, TEXT("OutSecondary"));
}
