// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
// #include "UKHitFeedbackAnimGraphNode.generated.h"

// USTRUCT(BlueprintType)
// struct UE5TEST_API FUKHitFeedbackAnimNode : public FAnimNode_SkeletalControlBase
// {
// 	GENERATED_BODY()
//
// public:
// 	// 흔들려야하는 방향
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "HitFeedback", meta=(PinShownByDefault))
// 	FVector Direction;
// 	// 초기 진폭
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
// 	float InitialAmplitude = 10.0f;
// 	// 감쇠율
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
// 	float Damping = 10.0f;
// 	// 각속도
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
// 	float AngularVelocity = 60.0f;
// 	// 흔들림 최대 시간
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
// 	float MaxTime = 0.2f;
// 	// 시간 변수
// 	float TimeElapsed = 0.0f;
// 	
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HitFeedback", meta = (PinShownByDefault))
// 	FName TargetBoneNames;
//
// 	FName CopyTargetBoneNames;
//
// public:
// 	FUKHitFeedbackAnimNode();
//
// 	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
// 	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
//
// 	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
// 	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
// };
//
// UCLASS()
// class UE5TEST_API UUKHitFeedbackAnimGraphNode : public UAnimGraphNode_SkeletalControlBase
// {
// 	GENERATED_BODY()
//
// public:
// 	// UPROPERTY(EditAnywhere)
// 	// FUKHitFeedbackAnimNode Node;
// 	// FUKHitFeedbackAnimNode Node;
//
// 	virtual FLinearColor GetNodeTitleColor() const override;
//
// 	virtual FText GetTooltipText() const override;
//
// 	virtual  FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
//
// 	virtual  FString GetNodeCategory() const override;
//
// 	// virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
// 	virtual const FAnimNode_SkeletalControlBase* GetNode() const PURE_VIRTUAL(UAnimGraphNode_SkeletalControlBase::GetNode, return nullptr;);
// };
