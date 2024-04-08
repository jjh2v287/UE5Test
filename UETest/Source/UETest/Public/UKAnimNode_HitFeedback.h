// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "UKAnimNode_HitFeedback.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct UETEST_API FUKAnimNode_HitFeedback : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	// 흔들려야하는 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "HitFeedback", meta=(PinShownByDefault))
	FVector Direction;
	// 초기 진폭
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
	float InitialAmplitude = 10.0f;
	// 감쇠율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
	float Damping = 10.0f;
	// 각속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
	float AngularVelocity = 60.0f;
	// 흔들림 최대 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeedback", meta = (PinShownByDefault))
	float MaxTime = 0.2f;
	// 시간 변수
	float TimeElapsed = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HitFeedback", meta = (PinShownByDefault))
	FName TargetBoneNames;

	FName CopyTargetBoneNames;

public:
	FUKAnimNode_HitFeedback();

	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;

	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
};