// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAnimNode_HitFeedback.h"
#include "Animation/AnimInstanceProxy.h"


FUKAnimNode_HitFeedback::FUKAnimNode_HitFeedback()
{
}

void FUKAnimNode_HitFeedback::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	Super::InitializeBoneReferences(RequiredBones);
}

void FUKAnimNode_HitFeedback::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	Super::EvaluateSkeletalControl_AnyThread(Output, OutBoneTransforms);
	check(OutBoneTransforms.Num() == 0);

	if(!TargetBoneNames.IsEqual(CopyTargetBoneNames))
	{
		CopyTargetBoneNames = TargetBoneNames;
		TimeElapsed = 0.0f;
	}

	if(TimeElapsed >= MaxTime)
	{
		return;
	}

	const FBoneContainer RequiredBones = Output.Pose.GetPose().GetBoneContainer();
	const int32 MeshBoneIndex = RequiredBones.GetPoseBoneIndexForBoneName(CopyTargetBoneNames);
	const FCompactPoseBoneIndex PoseBoneIndex = RequiredBones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshBoneIndex));
	if(PoseBoneIndex == INDEX_NONE)
	{
		return;
	}

	FTransform NewBoneTM = FTransform::Identity;
	FTransform ComponentTransform= FTransform::Identity;
	AActor* OwnerActor = nullptr;
	FRotator ActorRotationInverse = FRotator::ZeroRotator;
	FVector NewTranslation = FVector::ZeroVector;
	FVector DirectionNormal = FVector::ZeroVector;
	FVector NewPosition = FVector::ZeroVector;

	FCompactPoseBoneIndex ParentBoneIndex = RequiredBones.GetParentBoneIndex(PoseBoneIndex);
	USkeletalMeshComponent* SkelMesh = Output.AnimInstanceProxy->GetSkelMeshComponent();
	TArray<FCompactPoseBoneIndex> CompactPoseBones = RequiredBones.GetCompactPoseParentBoneArray();

	OwnerActor = SkelMesh->GetOwner();
	TimeElapsed += OwnerActor->GetWorld()->GetDeltaSeconds();
	ActorRotationInverse = OwnerActor->GetActorRotation().GetInverse();
	DirectionNormal = ActorRotationInverse.RotateVector(Direction);

	float Amplitude;
	float HalfMaxTime = (MaxTime * 0.5f);
	// n초 동안은 진폭을 일정하게 유지
	if (TimeElapsed <= HalfMaxTime)
	{
		Amplitude = InitialAmplitude;
	}
	// n초 후에는 진폭을 시간에 따라 감소시켜 원점으로 돌아오게 함
	else
	{
		float ReturnTime = (TimeElapsed - HalfMaxTime);
		Amplitude = InitialAmplitude * FMath::Exp(-Damping * ReturnTime);
	}
	NewPosition = (DirectionNormal * Amplitude * FMath::Sin(AngularVelocity * TimeElapsed));

	NewBoneTM = Output.Pose.GetComponentSpaceTransform(PoseBoneIndex);
	ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	NewBoneTM.AddToTranslation(NewPosition);

	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, PoseBoneIndex, EBoneControlSpace::BCS_ComponentSpace);
	OutBoneTransforms.Add( FBoneTransform(PoseBoneIndex, NewBoneTM) );
}

bool FUKAnimNode_HitFeedback::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return !Direction.IsZero();
}


void FUKAnimNode_HitFeedback::GatherDebugData(FNodeDebugData& DebugData)
{
	Super::GatherDebugData(DebugData);
}
