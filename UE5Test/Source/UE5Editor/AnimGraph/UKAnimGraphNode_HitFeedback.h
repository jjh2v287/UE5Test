// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "UE5Test/AnimGraph/UKAnimNode_HitFeedback.h"
#include "UKAnimGraphNode_HitFeedback.generated.h"

/**
 * 
 */
UCLASS()
class UUKAnimGraphNode_HitFeedback : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FUKAnimNode_HitFeedback Node;

	virtual FLinearColor GetNodeTitleColor() const override;

	virtual FText GetTooltipText() const override;

	virtual  FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual  FString GetNodeCategory() const override;

	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
};
