// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UKSimpleMovementComponent.generated.h"

struct FUKScopedMeshBoneUpdateOverride
{
	FUKScopedMeshBoneUpdateOverride(USkeletalMeshComponent* Mesh, EKinematicBonesUpdateToPhysics::Type OverrideSetting)
		: MeshRef(Mesh)
	{
		if (MeshRef)
		{
			// 현재 상태 저장
			SavedUpdateSetting = MeshRef->KinematicBonesUpdateType;
			// 오버라이드 설정 적용
			MeshRef->KinematicBonesUpdateType = OverrideSetting;
		}
	}

	~FUKScopedMeshBoneUpdateOverride()
	{
		if (MeshRef)
		{
			// 원래 설정 복원
			MeshRef->KinematicBonesUpdateType = SavedUpdateSetting;
		}
	}

private:
	USkeletalMeshComponent* MeshRef = nullptr;
	EKinematicBonesUpdateToPhysics::Type SavedUpdateSetting;
};

struct FUKScopedCapsuleMovementUpdate : public FScopedMovementUpdate
{
	typedef FScopedMovementUpdate Super;

	FUKScopedCapsuleMovementUpdate(USceneComponent* UpdatedComponent, bool bEnabled)
	: Super(bEnabled ? UpdatedComponent : nullptr, EScopedUpdate::DeferredUpdates)
	{
	}
};


/**
 * Similar to FScopedCapsuleMovementUpdate, but intended for the character mesh instead.
 * @see FScopedCapsuleMovementUpdate
 */
struct FUKScopedMeshMovementUpdate
{
	FUKScopedMeshMovementUpdate(USkeletalMeshComponent* Mesh, bool bEnabled = true)
	: ScopedMoveUpdate(bEnabled ? Mesh : nullptr, EScopedUpdate::DeferredUpdates)
	{
	}

private:
	FScopedMovementUpdate ScopedMoveUpdate;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UKNPC_API UUKSimpleMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	explicit UUKSimpleMovementComponent(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// AIController(PathFollowingComponent)가 이동 요청을 보낼 때 호출되는 함수
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	
protected:
	void RootMotionUpdate(const FRootMotionMovementParams& TempRootMotionParams);
	void SimpleWalkingUpdate(float DeltaTime);
	
	bool CheckWalkable(const FVector& FloorNormal) const;
	bool FindGround(const FVector& QueryOrigin, FHitResult& OutHit, const float DeltaTime) const;
	
protected:
	// 낙하 최대 속도 (Terminal Velocity)
	UPROPERTY(EditAnywhere, Category = "Move|Simple")
	float MaxFallSpeed = 4000.0f;
	
	// 아래로 찾을 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Move|Simple")
	float GroundSnapDistance = 10.f;
	
	// 현재 중력에 의해 누적된 속도
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Move|Simple")
	FVector GravityVelocity = FVector::ZeroVector;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Move|Simple")
	USkeletalMeshComponent* Mesh;
};
