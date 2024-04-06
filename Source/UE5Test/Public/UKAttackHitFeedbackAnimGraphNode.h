// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "UKAttackHitFeedbackAnimGraphNode.generated.h"

USTRUCT(BlueprintType)
struct UE5TEST_API FUKAttackHitFeedbackAnimNode : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Links")
	FPoseLink BasePoseNode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Links", meta = (PinShownByDefault))
	FVector AttackHitVec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shake", meta = (PinShownByDefault))
	float ShakeIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shake", meta = (PinShownByDefault))
	float ShakeFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shake", meta = (PinShownByDefault))
	float ShakeDurationMin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shake", meta = (PinShownByDefault))
	float ShakeDurationMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shake", meta = (PinShownByDefault))
	TArray<FName> TargetBoneNames;

	float ShakeDurationDelta;
	
	float ShakeDuration;
public:
	FUKAttackHitFeedbackAnimNode();

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	// 액터의 헤더 파일에 선언
	FVector OriginalPosition; // 객체의 원래 위치를 저장
	FVector ForceVector; // 적용할 힘 벡터
	FVector NewPosition;
	float ElapsedTime = 0.0f; // 경과 시간
	bool bIsShaking = false; // 흔들림 상태 여부
	FVector CalculateDampedOscillation(float DeltaTime, FVector StartPosition, FVector Force, float Duration);

// private:
// 	FAnimNode_ConvertLocalToComponentSpace LocalToComponentNode;
// 	FAnimNode_ConvertComponentToLocalSpace ComponentToLocalNode;
//
// 	FAnimNode_ModifyBone ModifyBoneNode1;
// 	FAnimNode_ModifyBone ModifyBoneNode2;
//
// 	float RollVal = 0.0f;
};

UCLASS()
class UE5TEST_API UUKAttackHitFeedbackAnimGraphNode : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FUKAttackHitFeedbackAnimNode Node;

	virtual FLinearColor GetNodeTitleColor() const override;

	virtual FText GetTooltipText() const override;

	virtual  FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual  FString GetNodeCategory() const override;
};
