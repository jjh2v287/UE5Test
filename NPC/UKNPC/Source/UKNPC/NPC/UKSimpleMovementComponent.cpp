// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKSimpleMovementComponent.h"

#include "CollisionDebugDrawingPublic.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "NavMesh/RecastNavMesh.h"

UUKSimpleMovementComponent::UUKSimpleMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUKSimpleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Mesh = PawnOwner->FindComponentByClass<USkeletalMeshComponent>();
	MovementMode = EMovementMode::MOVE_Walking;
}

// 1. AI의 이동 요청 수신
void UUKSimpleMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	// AIController가 계산한 목표 속도(MoveVelocity)를 받아옵니다.
	// 이를 통해 별도의 AddInputVector 호출 없이도 AI가 의도한 방향을 알 수 있습니다.
	FVector MoveDirection = MoveVelocity.GetSafeNormal();
    
	// NPC의 이동 입력을 강제로 주입합니다.
	// (PawnMovementComponent의 내부 입력 벡터에 값을 넣습니다)
	AddInputVector(MoveDirection);
}

// 2. 매 프레임 이동 처리
void UUKSimpleMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_Tick);
    if (!PawnOwner || !UpdatedComponent || ShouldSkipUpdate(DeltaTime))
    {
       return;
    }
	
	const FRootMotionMovementParams TempRootMotionParams = Mesh ? Mesh->ConsumeRootMotion() : FRootMotionMovementParams();
	
	if (TempRootMotionParams.bHasRootMotion)
	{
		// 루트 모션 O
		// Transform 가져오기 (로컬 좌표계 기준의 델타 값)
		FTransform RootMotionDelta = TempRootMotionParams.GetRootMotionTransform();
            
		// 회전 적용 (Pawn의 회전에 누적)
		// 루트 모션의 회전을 현재 액터의 회전에 적용합니다.
		FQuat NewRotation = UpdatedComponent->GetComponentQuat() * RootMotionDelta.GetRotation();
		// UpdatedComponent->SetWorldRotation(NewRotation);

		// 이동 적용 (월드 좌표계로 변환 필요)
		// 로컬 델타 위치를 월드 공간으로 변환하여 이동시킵니다.
		FVector MoveDelta = RootMotionDelta.GetTranslation();
		MoveDelta = Mesh->GetComponentQuat().RotateVector(MoveDelta);
		// MoveDelta = UpdatedComponent->GetComponentQuat().RotateVector(MoveDelta);

		// 충돌 처리를 포함한 이동 (SlideAlongSurface 등 사용 가능)
		FHitResult Hit;
		UMovementComponent::SafeMoveUpdatedComponent(MoveDelta, NewRotation, true, Hit);

		// 충돌 시 벽 타기 처리 (경량화 수준에 따라 생략 가능)
		if (Hit.IsValidBlockingHit())
		{
			UMovementComponent::SlideAlongSurface(MoveDelta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}
	}
	else
	{
		// 루트 모션 X
		// GravityTick(DeltaTime);
		NavTick(DeltaTime);
	}
}

void UUKSimpleMovementComponent::GravityTick(float DeltaTime)
{
	// [1단계] 현재 바닥 상태 확인 (이전 프레임 정보)
    bool bIsGrounded = CurrentFloorNormal.Z > 0.7f;

    // -------------------------------------------------------
    // [2단계] 좌우(수평) 이동 처리 (Lateral Movement)
    // -------------------------------------------------------
    FVector InputVector = ConsumeInputVector();
	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
    if (!InputVector.IsNearlyZero())
    {
        FVector DesiredMove = InputVector * 70.0f * DeltaTime;

        // 경사면 보정: 바닥에 있다면 입력 벡터를 바닥 기울기에 맞춰 투영
        if (bIsGrounded)
        {
            DesiredMove = FVector::VectorPlaneProject(DesiredMove, CurrentFloorNormal);
        }
        
        // 회전 처리
        FRotator TargetRotation = InputVector.Rotation();
        NewRotation = FMath::RInterpTo(NewRotation, TargetRotation, DeltaTime, 10.0f);
        NewRotation.Pitch = 0.0f; 
        NewRotation.Roll = 0.0f;

        // A. 이동 시도
        FHitResult Hit;
        {
            TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_SafeMoveUpdatedComponent);
			UMovementComponent::SafeMoveUpdatedComponent(DesiredMove, NewRotation, true, Hit);
        }

        // B. 충돌 시 슬라이딩 (벽이나 가파른 경사)
        if (Hit.IsValidBlockingHit())
        {
            FVector SlopeMove = FVector::VectorPlaneProject(DesiredMove, Hit.Normal);
            SlopeMove = SlopeMove.GetSafeNormal() * DesiredMove.Size();
            {
            	TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_SlideAlongSurface);
				UMovementComponent::SlideAlongSurface(SlopeMove, 1.f - Hit.Time, Hit.Normal, Hit, true);
            }
        }
    }

    // -------------------------------------------------------
    // [3단계] 중력 처리 (Gravity & Floor Snap)
    // -------------------------------------------------------
	GravityVelocity += -(GetGravityDirection() * GetGravityZ() * DeltaTime);
	GravityVelocity = GravityVelocity.GetClampedToMaxSize(MaxFallSpeed);
    FVector GravityMove = GravityVelocity * DeltaTime;

    // C. 중력 이동 시도
    FHitResult GravityHit;
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_GravityMove)
		UMovementComponent::SafeMoveUpdatedComponent(GravityMove, NewRotation, true, GravityHit);
	}
	if (GravityHit.IsValidBlockingHit())
	{
		CurrentFloorNormal = GravityHit.Normal;
		GravityVelocity = FVector::ZeroVector;
	}
	else
	{
		CurrentFloorNormal = FVector::UpVector;
	}
}

void UUKSimpleMovementComponent::NavTick(float DeltaTime)
{
    FVector InputVector = ConsumeInputVector();
	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
    if (!InputVector.IsNearlyZero())
    {
    	GravityVelocity += -(GetGravityDirection() * GetGravityZ() * DeltaTime);
    	GravityVelocity = GravityVelocity.GetClampedToMaxSize(MaxFallSpeed);
    	FVector GravityMove = GravityVelocity * DeltaTime;
    	
    	// SlingLineTrace
    	
    	// 이동 계산
        FVector DesiredMove = InputVector * 70.0f * DeltaTime;
    	DesiredMove += GravityMove;
    	if (CurrentFloorNormal != FVector::UpVector)
    	{
    		DesiredMove = FVector::VectorPlaneProject(DesiredMove, CurrentFloorNormal);
    		DesiredMove = DesiredMove.GetSafeNormal() * 70.0f * DeltaTime;
    	}
    	
        // 회전 처리
        FRotator TargetRotation = InputVector.Rotation();
        NewRotation = FMath::RInterpTo(NewRotation, TargetRotation, DeltaTime, 10.0f);
        NewRotation.Pitch = 0.0f; 
        NewRotation.Roll = 0.0f;

        // A. 이동 시도
    	FHitResult Hit;
    	{
    		TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_SweepSingleByChannel)
    		FCollisionShape CollisionShape = UpdatedPrimitive->GetCollisionShape();
    		FVector Start = UpdatedComponent->GetComponentLocation();
    		FVector End = Start - CollisionShape.GetCapsuleHalfHeight();
    		End.Z += -CollisionShape.GetCapsuleHalfHeight();
    		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UKSimpleMovement), false, PawnOwner);
    		GetWorld()->SweepSingleByChannel(Hit, Start, End, PawnOwner->GetActorRotation().Quaternion(), ECC_Pawn, CollisionShape, QueryParams);
    	}
    	
        UMovementComponent::SafeMoveUpdatedComponent(DesiredMove, NewRotation, true, Hit);
    	if (Hit.IsValidBlockingHit())
    	{
    		CurrentFloorNormal = Hit.Normal;
    		GravityVelocity = FVector::ZeroVector;
    	}
	    else
	    {
		    CurrentFloorNormal = FVector::UpVector;
	    }
    }
}