// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notify/ZKMeleeAttack_AnimNotifyState.h"
#include "Kismet/KismetSystemLibrary.h"

UZKMeleeAttack_AnimNotifyState::UZKMeleeAttack_AnimNotifyState()
{
}

void UZKMeleeAttack_AnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UAnimMontage* PlayMontage = MeshComp->GetAnimInstance()->GetCurrentActiveMontage();
	CurrentMontagePosition = MeshComp->GetAnimInstance()->Montage_GetPosition(PlayMontage);
	PreMontagePosition = CurrentMontagePosition;
}

void UZKMeleeAttack_AnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

void UZKMeleeAttack_AnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	PreMontagePosition = CurrentMontagePosition;

	UAnimMontage* PlayMontage = MeshComp->GetAnimInstance()->GetCurrentActiveMontage();
	CurrentMontagePosition = MeshComp->GetAnimInstance()->Montage_GetPosition(PlayMontage);

	// 1. 애니메이션 길이 내에서 PlaybackTime 보정 (루핑 고려)
	const float SequenceLength = Animation->GetPlayLength();
	const float ClampedTime = FMath::Fmod(CurrentMontagePosition, SequenceLength);

	// 2. FAnimExtractContext를 구성합니다.
	FAnimExtractContext ExtractionContext(ClampedTime, /*bExtractRootMotion=*/true);

	// 3. 애니메이션의 스켈레톤 참조 및 전체 본 개수
	USkeleton* Skeleton = Animation->GetSkeleton();
	const int32 NumBones = Skeleton->GetReferenceSkeleton().GetNum();

	// 4. 모든 본의 Transform을 저장할 배열 준비 (레퍼런스 공간 기준)
	TArray<FTransform> BonePoses;
	BonePoses.SetNum(NumBones);

	const FReferenceSkeleton& ReferenceSkeleton = Animation->GetSkeleton()->GetReferenceSkeleton();
	static TMap<FReferenceSkeleton*, FBoneContainer> BoneContainerCache;
	FBoneContainer BoneContainer;
	if (const FBoneContainer* CachedBoneContainer = BoneContainerCache.Find(&ReferenceSkeleton))
	{
		// Use cached bone container if available.
		BoneContainer = *CachedBoneContainer;
	}
	else
	{
		// Create a new bone container
		TArray<uint16> BoneIndices;
		BoneIndices.SetNumUninitialized(NumBones);
		for (int32 Index = 0; Index < NumBones; ++Index)
		{
			BoneIndices[Index] = static_cast<uint16>(Index);
		}
    
		BoneContainer.SetUseRAWData(true);
		BoneContainer.InitializeTo(BoneIndices, UE::Anim::FCurveFilterSettings(), *Animation->GetSkeleton());

		BoneContainerCache.Emplace(&(const_cast<FReferenceSkeleton&>(ReferenceSkeleton)), BoneContainer);
	}

	FCompactPose OutPose;
	OutPose.SetBoneContainer(&BoneContainer);
	FBlendedCurve OutCurve;
	OutCurve.InitFrom(BoneContainer);
	UE::Anim::FStackAttributeContainer TempAttributes;
	FAnimationPoseData PoseData{OutPose, OutCurve, TempAttributes};
	
	// 5. 애니메이션 데이터를 보간하여 BonePoses를 채웁니다.
	// GetBonePose는 애니메이션의 평가된(보간된) 포즈를 출력합니다.
	if (UAnimMontage* AnimSeq = Cast<UAnimMontage>(Animation))
	{
		AnimSeq->SlotAnimTracks[0].AnimTrack.GetAnimationPose(PoseData, ExtractionContext);
	}else
	{
		Animation->GetAnimationPose(PoseData, ExtractionContext);
	}

	// 6. 원하는 본의 인덱스를 찾습니다.
	int32 BoneIndex2 = Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName);
	if (BoneIndex2 == INDEX_NONE)
	{
		BoneIndex2 = 0;
		UE_LOG(LogTemp, Warning, TEXT("GetBoneTransformAtTime: Bone '%s' not found in Skeleton."), *BoneName.ToString());
	}

	FTransform MeshTransform = MeshComp->GetComponentTransform();

	// 7. 추출된 본의 Transform을 가져온 후, Skeletal Mesh의 컴포넌트 Transform을 적용해 월드 공간으로 변환합니다.
	//    (필요에 따라 추가 오프셋이 있을 경우 더 계산할 수 있음)
	const FCompactPoseBoneIndex CompactIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneIndex2));
	FTransform LocalBoneTransform = PoseData.GetPose()[CompactIndex];
	FTransform OutTransform = LocalBoneTransform * MeshTransform;
	
	FTransform ComponentToWorld = MeshComp->GetComponentTransform();
	FTransform CurrentBoneTransform = FTransform::Identity;
	FName boneName = MeshComp->GetSocketBoneName(BoneName);
	int32 BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(boneName);

	int32 ParentIndex = BoneIndex;
	TArray<int32> ParentBoneIndexes;
	while (ParentIndex != INDEX_NONE)
	{
		ParentBoneIndexes.Emplace(ParentIndex);
		ParentIndex = ReferenceSkeleton.GetParentIndex(ParentIndex);
	}
	Algo::Reverse(ParentBoneIndexes);
	
	if (UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation))
	{
		int32 CurSegmentIndex = AnimMontage->SlotAnimTracks[0].AnimTrack.GetSegmentIndexAtTime(CurrentMontagePosition);
		if (CurSegmentIndex < 0)
		{
			return;
		}
		UAnimSequence* CurSequenc = Cast<UAnimSequence>(AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[CurSegmentIndex].GetAnimReference().Get());
		for (int32 ParentBoneIndex : ParentBoneIndexes)
		{
			if (ParentBoneIndex == 0)
				continue;
			FSkeletonPoseBoneIndex PoseBoneIndex = FSkeletonPoseBoneIndex(ParentBoneIndex);
			FTransform OutAtom = FTransform::Identity;
			CurSequenc->GetBoneTransform(OutAtom, PoseBoneIndex, CurrentMontagePosition, false);
			CurrentBoneTransform *= OutAtom;
		}
	}

	// 스켈레탈 메시 컴포넌트의 월드 변환을 가져옵니다.
	FTransform WorldCurrentBoneTransform = CurrentBoneTransform * ComponentToWorld;

	// 스윕을 수행하여 경로상의 충돌 검사를 실행
	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore = {MeshComp->GetOwner()};
	bool bHit =  UKismetSystemLibrary::CapsuleTraceMulti(
		MeshComp->GetOwner(),
		WorldCurrentBoneTransform.GetLocation(),
		WorldCurrentBoneTransform.GetLocation(),
		1.f,
		1.f,
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::Type::ForDuration,
		HitResults, true,
		FLinearColor::Red,
		FLinearColor::Blue,
		20.f);
}
