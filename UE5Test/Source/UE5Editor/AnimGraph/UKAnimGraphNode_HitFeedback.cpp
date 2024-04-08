// Copyright Kong Studios, Inc. All Rights Reserved.


#include "AnimGraph/UKAnimGraphNode_HitFeedback.h"


FLinearColor UUKAnimGraphNode_HitFeedback::GetNodeTitleColor() const
{
	return( FLinearColor::Blue );
}

FText UUKAnimGraphNode_HitFeedback::GetTooltipText() const
{
	return Super::GetTooltipText();
}

FText UUKAnimGraphNode_HitFeedback::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return Super::GetNodeTitle(TitleType);
}

FString UUKAnimGraphNode_HitFeedback::GetNodeCategory() const
{
	return Super::GetNodeCategory();
}