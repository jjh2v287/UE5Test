// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNGraphNode_Root.h"

#include "HTNGraphPinCategories.h"
#include "UObject/UObjectIterator.h"
#include "BehaviorTree/BlackboardData.h"
#include "HTN.h"
#include "HTNGraph.h"

UHTNGraphNode_Root::UHTNGraphNode_Root(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bIsReadOnly = true;
}

void UHTNGraphNode_Root::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	if (UHTN* const HTNAsset = GetTypedOuter<UHTN>())
	{
		BlackboardAsset = HTNAsset->BlackboardAsset;
	}

	if (!BlackboardAsset)
	{
		for (UBlackboardData* const CandidateBlackboardAsset : TObjectRange<UBlackboardData>())
		{
			if (ensure(CandidateBlackboardAsset) && !CandidateBlackboardAsset->HasAnyFlags(RF_ClassDefaultObject))
			{
				BlackboardAsset = CandidateBlackboardAsset;
				UpdateBlackboard();
				break;
			}
		}
	}
}

void UHTNGraphNode_Root::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, FHTNGraphPinCategories::MultipleNodesAllowed, TEXT("Out"));
}

void UHTNGraphNode_Root::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UHTNGraphNode_Root, BlackboardAsset))
	{
		UpdateBlackboard();
	}
}

void UHTNGraphNode_Root::PostEditUndo()
{
	Super::PostEditUndo();
	
	UpdateBlackboard();
}

FText UHTNGraphNode_Root::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("HTNGraph", "RootNode", "Root");
}

FText UHTNGraphNode_Root::GetDescription() const
{
	return FText::FromString(GetNameSafe(BlackboardAsset));
}

FText UHTNGraphNode_Root::GetTooltipText() const
{
	// Make a tooltip from the header comment of this class.
	return UEdGraphNode::GetTooltipText();
}

void UHTNGraphNode_Root::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	AddContextMenuActionsForAddingDecorators(Menu, TEXT("HTNGraphNode"), Context);
	AddContextMenuActionsForAddingServices(Menu, TEXT("HTNGraphNode"), Context);
}

void UHTNGraphNode_Root::UpdateBlackboard() const
{
	if (UHTNGraph* const Graph = Cast<UHTNGraph>(GetOuter()))
	{
		if (UHTN* const HTNAsset = Cast<UHTN>(Graph->GetOuter()))
		{
			if (HTNAsset && HTNAsset->BlackboardAsset != BlackboardAsset)
			{
				HTNAsset->BlackboardAsset = BlackboardAsset;
				Graph->OnBlackboardChanged();
			}
		}
	}
}
