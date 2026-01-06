// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UKAsyncMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UKNPC_API UUKAsyncMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUKAsyncMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// AIController(PathFollowingComponent)가 이동 요청을 보낼 때 호출되는 함수
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	
protected:
	// 중력 방향 단위 벡터 (일반적으로 FVector::DownVector (0,0,-1))
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	// FVector GravityDirection = FVector::DownVector;

	// 낙하 최대 속도 (Terminal Velocity)
	UPROPERTY(EditAnywhere, Category = "Movement|Gravity")
	float MaxFallSpeed = 4000.0f;
	
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Debug")
	USkeletalMeshComponent* Mesh;
	
	// 비동기 스위프 완료 콜백
	void OnAsyncSweepCompleted(const FTraceHandle& Handle, FTraceDatum& Datum);

private:
	// 비동기 요청 핸들
	FTraceHandle LastTraceHandle;
    
	// 비동기 요청 시점의 이동 의도 데이터 저장
	struct FPendingMove
	{
		FVector DesiredDelta;
		float DeltaTime;
		bool bIsActive = false;
	} PendingMoveData;

	// 바닥 법선 벡터
	FVector CurrentFloorNormal = FVector::UpVector;
	FVector GravityVelocity = FVector::ZeroVector;
    
	// 중복 요청 방지
	bool bIsAsyncTracePending = false;
};
