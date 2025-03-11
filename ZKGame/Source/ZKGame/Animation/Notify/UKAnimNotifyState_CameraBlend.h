// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "UKAnimNotifyState_CameraBlend.generated.h"

/**
 * 
 */
UCLASS()
class ZKGAME_API UUKAnimNotifyState_CameraBlend : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendInTime = 0.25f;  // Blend In 시간

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendOutTime = 0.25f; // Blend Out 시간

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float TargetFOV = 90.0f;   // 목표 FOV

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FName CameraBoneName = TEXT("CameraBone"); // 기본값 설정

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	float ElapsedTime = 0.0f;  // 경과 시간 추적
	float StartFOV = 0.0f;     // 시작 시 FOV
	float EndTime = 0.0f;
	FVector StartLocation;     // 시작 시 카메라 위치
};
