// Copyright Epic Games, Inc. All Rights Reserved.


#include "UKAttackHitFeedbackAnimGraphNode.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_BlendBoneByChannel.h"

FUKAttackHitFeedbackAnimNode::FUKAttackHitFeedbackAnimNode()
	: AttackHitVec(FVector::ZeroVector),
	ShakeDurationDelta(1),
	OriginalPosition(FVector::ZeroVector),
	ForceVector(FVector(0.0f, 100.f, 0.0f))
{
}

void FUKAttackHitFeedbackAnimNode::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	// FAnimNode_Base::Initialize_AnyThread(Context);
	BasePoseNode.Initialize(Context);
}

void FUKAttackHitFeedbackAnimNode::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	// FAnimNode_Base::CacheBones_AnyThread(Context);
	BasePoseNode.CacheBones(Context);
}

void FUKAttackHitFeedbackAnimNode::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	// FAnimNode_Base::Update_AnyThread(Context);
	BasePoseNode.Update(Context);

	if (ShakeDuration < ShakeDurationMin)
	{
		ShakeDurationDelta = Context.GetDeltaTime() * ShakeIntensity;
	}
	else if (ShakeDuration > ShakeDurationMax)
	{
		ShakeDurationDelta = -(Context.GetDeltaTime() * ShakeIntensity);
	}

	ShakeDuration += ShakeDurationDelta;

	// if (bIsShaking) {
		NewPosition = CalculateDampedOscillation(Context.GetDeltaTime(), OriginalPosition, ForceVector, 2.0f); // 2초 동안 진동
		// SetActorLocation(NewPosition);
	// }
}

// 감쇠 진동을 계산하는 함수
FVector FUKAttackHitFeedbackAnimNode::CalculateDampedOscillation(float DeltaTime, FVector StartPosition, FVector Force, float Duration)
{
	ElapsedTime += DeltaTime;

	// 탄성 계수와 감쇠 비율
	float Elasticity = 1.0f; // 탄성 계수, 낮을수록 더 많이 흔들림
	float Damping = 10.0f; // 감쇠 비율, 높을수록 빠르게 진정됨

	FVector Displacement = Force * Elasticity * cos(sqrt(Elasticity) * ElapsedTime) * exp(-Damping * ElapsedTime);
    
	if (ElapsedTime >= Duration) {
		bIsShaking = false;
		ElapsedTime = 0.0f; // 시간 초기화
		return StartPosition; // 원래 위치로 복귀
	}

	return StartPosition + Displacement; // 새로운 위치 계산
}

void FUKAttackHitFeedbackAnimNode::Evaluate_AnyThread(FPoseContext& Output)
{
	BasePoseNode.Evaluate(Output);
	
	// 타겟 본에 대한 인덱스 찾기
	FCompactPose& TargetPose = Output.Pose;
	for (auto BoneName : TargetBoneNames)
	{
		const int32 MeshBoneIndex = Output.Pose.GetBoneContainer().GetPoseBoneIndexForBoneName(BoneName);
		const FCompactPoseBoneIndex PoseBoneIndex = Output.Pose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshBoneIndex));
		if(PoseBoneIndex == -1)
		{
			continue;
		}
		// 현재 본의 변형 가져오기
		FTransform& BoneTransform = TargetPose[PoseBoneIndex];

		// 현재 본의 변형 가져오기
		FCSPose<FCompactPose> GlobalPose;
		GlobalPose.InitPose(Output.Pose);
		// FTransform BoneTransform = GlobalPose.GetComponentSpaceTransform(PoseBoneIndex);
    
		// 쉐이킹 로직 적용
		// 예시: BoneTransform의 위치에 간단한 쉐이킹을 적용
		FVector Translation = BoneTransform.GetTranslation();
		Translation += NewPosition;
		BoneTransform.SetTranslation(Translation);
	}

	/*
	Output.Pose.MoveBonesFrom(Output.Pose);
	Output.Curve.MoveFrom(Output.Curve);
	Output.CustomAttributes.MoveFrom(Output.CustomAttributes);
	*/
}

void FUKAttackHitFeedbackAnimNode::GatherDebugData(FNodeDebugData& DebugData)
{
	// FAnimNode_Base::GatherDebugData(DebugData);
	// DebugData.AddDebugItem();
	BasePoseNode.GatherDebugData(DebugData);
}

FLinearColor UUKAttackHitFeedbackAnimGraphNode::GetNodeTitleColor() const
{
	return( FLinearColor::Blue );
}

FText UUKAttackHitFeedbackAnimGraphNode::GetTooltipText() const
{
	return Super::GetTooltipText();
}

FText UUKAttackHitFeedbackAnimGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return Super::GetNodeTitle(TitleType);
}

FString UUKAttackHitFeedbackAnimGraphNode::GetNodeCategory() const
{
	return Super::GetNodeCategory();
}
