// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#pragma once
#include "SEventGraphNode.h"
#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "EdGraphUtilities.h"
#include "SGraphNode.h"

class FEventsGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* InNode) const override
	{
		if (UEventsK2Node_EventBase* EventNode = Cast<UEventsK2Node_EventBase>(InNode))
		{
			TSharedRef<SGraphNode> GraphNode = SNew(SEventGraphNode, EventNode);
			return GraphNode;
		}

		return nullptr;
	}
};
