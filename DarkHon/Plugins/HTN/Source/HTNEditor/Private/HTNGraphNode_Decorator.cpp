// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNGraphNode_Decorator.h"
#include "HTNDecorator.h"

UHTNGraphNode_Decorator::UHTNGraphNode_Decorator(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bIsSubNode = true;
}

FText UHTNGraphNode_Decorator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (const UHTNDecorator* const Decorator = Cast<UHTNDecorator>(NodeInstance))
	{
		return FText::FromString(Decorator->GetNodeName());
	}
	else if (!ClassData.GetClassName().IsEmpty())
	{
		FString StoredClassName = ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		return FText::Format(NSLOCTEXT("AIGraph", "NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	return Super::GetNodeTitle(TitleType);
}

void UHTNGraphNode_Decorator::AllocateDefaultPins()
{
	// No pins for decorators
}
