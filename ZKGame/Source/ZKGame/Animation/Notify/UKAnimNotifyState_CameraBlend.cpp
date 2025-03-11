// Copyright Kong Studios, Inc. All Rights Reserved.


#include "UKAnimNotifyState_CameraBlend.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"

void UUKAnimNotifyState_CameraBlend::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Owner->GetInstigatorController());
	if (!PlayerController)
	{
		return;
	}

	APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(PlayerController->PlayerCameraManager);
	if (!CameraManager)
	{
		return;
	}

	// 종료 시간 저장
	EndTime = TotalDuration;
	
	// 현재 카메라 상태 저장
	StartFOV = CameraManager->GetFOVAngle();
	StartLocation = CameraManager->GetCameraLocation();

	// 초기화
	ElapsedTime = 0.0f;
}

void UUKAnimNotifyState_CameraBlend::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

    APlayerController* PlayerController = Cast<APlayerController>(Owner->GetInstigatorController());
    if (!PlayerController)
    {
    	return;
    }

    APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(PlayerController->PlayerCameraManager);
    if (!CameraManager)
    {
    	return;
    }

    ElapsedTime += FrameDeltaTime;

	float Alpha = 1.0f;
	
    // Blend In/Out에 따른 Alpha 계산
    if (ElapsedTime <= BlendInTime) // Blend In
    {
        Alpha = FMath::Clamp(ElapsedTime / BlendInTime, 0.0f, 1.0f);
    }
    else if (ElapsedTime >= (EndTime - BlendOutTime))
    {
    	Alpha -= FMath::Clamp((ElapsedTime - (EndTime - BlendOutTime)) / BlendOutTime, 0.0f, 1.0f);
    }

    // FOV 보간
    float CurrentFOV = FMath::Lerp(StartFOV, TargetFOV, Alpha);
    // CameraManager->SetFOV(CurrentFOV);

	// 카메라 본 위치 가져오기
	int32 BoneIndex = MeshComp->GetBoneIndex(CameraBoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraBone '%s' not found in SkeletalMesh"), *CameraBoneName.ToString());
		return;
	}
	FTransform CameraBoneTransform = MeshComp->GetBoneTransform(BoneIndex);
	FVector TargetLocation = CameraBoneTransform.GetLocation();

	/*// 플레이어 위치
	const FVector PlayerLocation = Owner->GetActorLocation();

	// UpdateLockOnView 스타일의 계산 적용
	const FVector Offset = (TargetLocation - PlayerLocation);
	const float Distance = FMath::Max(Offset.Size() * 0.5f, CameraManager->MinimumLookAtDist); // CameraManager에서 가져왔다고 가정
	const FVector MidPoint = PlayerLocation + Offset.GetSafeNormal2D() * Distance;
	const FVector LookDirection = MidPoint - (CameraManager->GetCameraLocation() + CameraManager->LookAtOffset); // CameraManager에서 가져왔다고 가정
	
	// 목표 회전 계산
	const FRotator TargetRotation = FRotationMatrix::MakeFromX(LookDirection).Rotator();
	const FRotator CurrentRotation = PlayerController->GetControlRotation();
	
    
    // 회전 보간
    const float LagSpeed = UUKGameSettings::GetGameSettings()->LockOn.CameraLagSpeed; // 설정에서 가져옴
    FRotator NextRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, FrameDeltaTime, LagSpeed);
    NextRotation.Pitch = FMath::Clamp(NextRotation.Pitch, 
                                      UUKGameSettings::GetGameSettings()->LockOn.PitchMin, 
                                      UUKGameSettings::GetGameSettings()->LockOn.PitchMax);*/

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	FRotator NewRotation = (TargetLocation - CameraLocation).Rotation();
	
    PlayerController->SetControlRotation(NewRotation);
}

void UUKAnimNotifyState_CameraBlend::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Owner->GetInstigatorController());
	if (!PlayerController)
	{
		return;
	}

	APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(PlayerController->PlayerCameraManager);
	if (!CameraManager)
	{
		return;
	}

	// Blend Out은 별도 타이머나 상태로 처리 가능
	// 여기서는 단순히 원래 값으로 복귀한다고 가정
	CameraManager->SetFOV(StartFOV);
	// CameraManager->SetCameraLocation(StartLocation);
}
