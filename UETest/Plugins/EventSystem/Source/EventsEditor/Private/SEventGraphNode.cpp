// Fill out your copyright notice in the Description page of Project Settings.
#include "SEventGraphNode.h"
#include "EventsK2Node_EventBase.h"



void SEventGraphNode::Construct(const FArguments& InArgs, UEventsK2Node_EventBase* InNode)
{
	this->GraphNode = InNode;

	this->SetCursor(EMouseCursor::CardinalCross);

	this->UpdateGraphNode();
}

void SEventGraphNode::CreatePinWidgets()
{
	// Create Pin widgets for each of the pins.
	for (int32 PinIndex = 0; PinIndex < GraphNode->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* CurPin = GraphNode->Pins[PinIndex];

		if (!ensureMsgf(CurPin->GetOuter() == GraphNode
			, TEXT("Graph node ('%s' - %s) has an invalid %s pin: '%s'; (with a bad %s outer: '%s'); skiping creation of a widget for this pin.")
			, *GraphNode->GetNodeTitle(ENodeTitleType::ListView).ToString()
			, *GraphNode->GetPathName()
			, (CurPin->Direction == EEdGraphPinDirection::EGPD_Input) ? TEXT("input") : TEXT("output")
			, CurPin->PinFriendlyName.IsEmpty() ? *CurPin->PinName.ToString() : *CurPin->PinFriendlyName.ToString()
			, CurPin->GetOuter() ? *CurPin->GetOuter()->GetClass()->GetName() : TEXT("UNKNOWN")
			, CurPin->GetOuter() ? *CurPin->GetOuter()->GetPathName() : TEXT("NULL")))
		{
			continue;
		}

		CreateStandardPinWidget(CurPin);
	}
}
