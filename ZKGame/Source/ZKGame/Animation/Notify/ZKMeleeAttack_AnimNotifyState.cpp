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

	// 현재 재생 중인 몽타주의 위치 저장
	if (UAnimMontage* PlayMontage = MeshComp->GetAnimInstance()->GetCurrentActiveMontage())
	{
		CurrentMontagePosition = MeshComp->GetAnimInstance()->Montage_GetPosition(PlayMontage);
		PreMontagePosition = CurrentMontagePosition;
	}
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

	// 몽타주 위치 업데이트
	PreMontagePosition = CurrentMontagePosition;
	if (UAnimMontage* PlayMontage = MeshComp->GetAnimInstance()->GetCurrentActiveMontage())
	{
		CurrentMontagePosition = MeshComp->GetAnimInstance()->Montage_GetPosition(PlayMontage);
	}
    
	// 공격 판정 실행
	TraceTest(MeshComp, Animation, FrameDeltaTime, EventReference);
	// LegacyTrace(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UZKMeleeAttack_AnimNotifyState::TraceTest(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
        return;

    // 1. 애니메이션 시간 계산
    const float SequenceLength = Animation->GetPlayLength();
    const float ClampedTime = FMath::Fmod(CurrentMontagePosition, SequenceLength);

    // 2. 애니메이션 데이터 추출 준비
    FAnimExtractContext ExtractionContext(ClampedTime, true);
    USkeleton* Skeleton = Animation->GetSkeleton();
    const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
    const int32 NumBones = ReferenceSkeleton.GetNum();

    // 3. 본 컨테이너 설정
    static TMap<FReferenceSkeleton*, FBoneContainer> BoneContainerCache;
    FBoneContainer BoneContainer;
    
    // 캐시된 본 컨테이너가 있으면 사용
    if (const FBoneContainer* CachedBoneContainer = BoneContainerCache.Find(&ReferenceSkeleton))
    {
        BoneContainer = *CachedBoneContainer;
    }
    else
    {
        // 새로운 본 컨테이너 생성
        TArray<uint16> BoneIndices;
        BoneIndices.SetNumUninitialized(NumBones);
        for (int32 Index = 0; Index < NumBones; ++Index)
        {
            BoneIndices[Index] = static_cast<uint16>(Index);
        }
        
        BoneContainer.SetUseRAWData(true);
        BoneContainer.InitializeTo(BoneIndices, UE::Anim::FCurveFilterSettings(), *Skeleton);
        BoneContainerCache.Emplace(&(const_cast<FReferenceSkeleton&>(ReferenceSkeleton)), BoneContainer);
    }

    // 4. 포즈 데이터 준비
    FCompactPose OutPose;
    OutPose.SetBoneContainer(&BoneContainer);
    FBlendedCurve OutCurve;
    OutCurve.InitFrom(BoneContainer);
    UE::Anim::FStackAttributeContainer TempAttributes;
    FAnimationPoseData PoseData{OutPose, OutCurve, TempAttributes};

    // 5. 애니메이션 포즈 추출
    if (UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation))
    {
        AnimMontage->SlotAnimTracks[0].AnimTrack.GetAnimationPose(PoseData, ExtractionContext);
    }
    else
    {
        Animation->GetAnimationPose(PoseData, ExtractionContext);
    }

    // 6. 본 트랜스폼 계산
    FName SocketBoneName = MeshComp->GetSocketBoneName(BoneName);
    int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(SocketBoneName);
    if (BoneIndex == INDEX_NONE)
    {
        BoneIndex = 0;
        UE_LOG(LogTemp, Warning, TEXT("Bone '%s' not found in Skeleton."), *BoneName.ToString());
    }

    // 부모 본 체인 구성
    int32 ParentIndex = BoneIndex;
    TArray<int32> ParentBoneIndexes;
    while (ParentIndex != INDEX_NONE)
    {
        ParentBoneIndexes.Emplace(ParentIndex);
        ParentIndex = ReferenceSkeleton.GetParentIndex(ParentIndex);
    }
    Algo::Reverse(ParentBoneIndexes);

    // 7. 최종 트랜스폼 계산
    FTransform OutTransform = FTransform::Identity;
    FTransform RelativeTransform = MeshComp->GetComponentTransform().GetRelativeTransform(Owner->GetTransform());
    OutTransform = RelativeTransform * OutTransform;
    
    for (int32 ParentBoneIndex : ParentBoneIndexes)
    {
        if (ParentBoneIndex == 0)
            continue;
        
        const FCompactPoseBoneIndex CompactIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ParentBoneIndex));
        FTransform LocalBoneTransform = PoseData.GetPose()[CompactIndex];
        OutTransform = LocalBoneTransform * OutTransform;
    }

    // 8. 월드 공간 변환
    FVector StartFinalBoneTransform = Owner->GetActorRotation().RotateVector(OutTransform.GetLocation());
    StartFinalBoneTransform = StartFinalBoneTransform + Owner->GetTransform().GetLocation();

    // 9. 충돌 검사 실행
    TArray<FHitResult> HitResults;
    TArray<AActor*> ActorsToIgnore = {Owner};
    bool bHit = UKismetSystemLibrary::CapsuleTraceMulti(
        Owner,
        StartFinalBoneTransform,
        StartFinalBoneTransform,
        1.f,  // 캡슐 반경
        1.f,  // 캡슐 높이
        ETraceTypeQuery::TraceTypeQuery1,
        false,  // 복잡한 충돌 체크 비활성화
        ActorsToIgnore,
        EDrawDebugTrace::Type::ForDuration,
        HitResults,
        true,  // 충돌 발생 시 트레이스 중단
        FLinearColor::Red,
        FLinearColor::Blue,
        5.f  // 디버그 표시 지속 시간
    );

    // TODO: 충돌 처리 로직 추가
    if (bHit)
    {
        // 충돌 이벤트 처리
    }
}

/*void UZKMeleeAttack_AnimNotifyState::LegacyTrace(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	const float SequenceLength = Animation->GetPlayLength();
	const float ClampedTime = FMath::Fmod(CurrentMontagePosition, SequenceLength);
	USkeleton* Skeleton = Animation->GetSkeleton();
	const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
	FTransform CurrentBoneTransform = FTransform::Identity;
	AActor* Owner = MeshComp->GetOwner();
	FTransform RelativeTransform = MeshComp->GetComponentTransform().GetRelativeTransform(Owner->GetTransform());
	CurrentBoneTransform = RelativeTransform * CurrentBoneTransform;
	FName boneName = MeshComp->GetSocketBoneName(BoneName);
	int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(boneName);
	
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
		int32 CurSegmentIndex = AnimMontage->SlotAnimTracks[0].AnimTrack.GetSegmentIndexAtTime(ClampedTime);
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
			CurSequenc->GetBoneTransform(OutAtom, PoseBoneIndex, ClampedTime, false);
			CurrentBoneTransform = OutAtom * CurrentBoneTransform;
		}
	}
	
	// 액터 월드 행영 적용
	FVector StartFinalBoneTransform = Owner->GetActorRotation().RotateVector(CurrentBoneTransform.GetLocation());
	StartFinalBoneTransform = StartFinalBoneTransform + Owner->GetTransform().GetLocation();
	
	// 스윕을 수행하여 경로상의 충돌 검사를 실행
	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore = {Owner};
	bool bHit =  UKismetSystemLibrary::CapsuleTraceMulti(
		Owner,
		StartFinalBoneTransform,
		StartFinalBoneTransform,
		1.f,
		1.f,
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::Type::ForDuration,
		HitResults, true,
		FLinearColor::Red,
		FLinearColor::Blue,
		5.f);
}*/