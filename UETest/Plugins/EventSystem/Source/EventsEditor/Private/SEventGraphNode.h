// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EventsK2Node_EventBase.h"
#include "KismetNodes/SGraphNodeK2Base.h"

/**
 * 
 */
class EVENTSEDITOR_API SEventGraphNode : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SEventGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEventsK2Node_EventBase* InNode);


public:
	virtual void CreatePinWidgets() override;
};
