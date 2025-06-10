// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNConnectionDrawingPolicy.h"
#include "HTNColors.h"
#include "HTNGraphNode.h"
#include "Nodes/HTNNode_SubNetwork.h"

FHTNConnectionDrawingPolicy::FHTNConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FAIGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
{}

void FHTNConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;
	Params.WireThickness = 1.5f;
	Params.WireColor = HTNColors::Connection::Default;

	if (HoveredPins.Num())
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}

	UHTNGraphNode* const FromNode = OutputPin ? Cast<UHTNGraphNode>(OutputPin->GetOwningNode()) : nullptr;
	UHTNGraphNode* const ToNode = InputPin ? Cast<UHTNGraphNode>(InputPin->GetOwningNode()) : nullptr;
	if (FromNode && ToNode) 
	{
		const bool bIsConnectionInPlan = ToNode->DebuggerPlanEntries.ContainsByPredicate([&](const UHTNGraphNode::FDebuggerPlanEntry& ToEntry)
		{
			return ToEntry.PreviousNode == FromNode;
		});
		
		if (bIsConnectionInPlan)
		{
			const bool bIsConnectionInFuture = ToNode->DebuggerPlanEntries.ContainsByPredicate([&](const UHTNGraphNode::FDebuggerPlanEntry& ToEntry)
			{
				return ToEntry.bIsInFutureOfPlan && FromNode->DebuggerPlanEntries.ContainsByPredicate([&](const UHTNGraphNode::FDebuggerPlanEntry& FromEntry)
				{
					return (ToEntry.bIsExecuting || FromEntry.bIsInFutureOfPlan) && FMath::Abs(ToEntry.DepthInPlan - FromEntry.DepthInPlan) <= 1;
				});
			});

			const bool bIsConnectionActive = bIsConnectionInFuture && ToNode->DebuggerPlanEntries.ContainsByPredicate([&](const UHTNGraphNode::FDebuggerPlanEntry& ToEntry) 
			{
				return ToEntry.bIsExecuting && FromNode->DebuggerPlanEntries.ContainsByPredicate([&](const UHTNGraphNode::FDebuggerPlanEntry& FromEntry)
				{
					return FMath::Abs(ToEntry.DepthInPlan - FromEntry.DepthInPlan) <= 1;
				});
			});
			
			Params.WireColor = bIsConnectionInFuture ? HTNColors::Connection::DebugFuturePartOfPlan : HTNColors::Connection::DebugPastPartOfPlan;
			Params.WireThickness = bIsConnectionInFuture ? 10.0f : 1.5f;
			Params.bDrawBubbles = bIsConnectionActive;
		}
	}
}
