// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKSimpleMovementComponent.h"

#include "CollisionDebugDrawingPublic.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
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
		if (Hit.IsValidBlockingHit() && CheckWalkable(Hit.ImpactNormal))
		{
			UMovementComponent::SlideAlongSurface(MoveDelta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}
	}
	else
	{
		// 루트 모션 X
		// GravityTick(DeltaTime);
		SimpleTick(DeltaTime);
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
	InputVector = FVector(InputVector.X, InputVector.Y, 0.f);
	InputVector = InputVector.GetSafeNormal();
	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
    if (!InputVector.IsNearlyZero())
    {
        FVector DesiredMove = InputVector * MaxSpeed * DeltaTime;
    	DesiredMove = DesiredMove.GetClampedToMaxSize2D(MaxSpeed);
    	
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
		
		FVector SlopeMove = FVector::VectorPlaneProject(GravityMove, GravityHit.Normal);
		SlopeMove = SlopeMove.GetSafeNormal() * GravityMove.Size();
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_GravityMoveSlideAlongSurface);
			UMovementComponent::SlideAlongSurface(SlopeMove, 1.f - GravityHit.Time, GravityHit.Normal, GravityHit, true);
		}
	}
	else
	{
		CurrentFloorNormal = FVector::UpVector;
	}
}

void UUKSimpleMovementComponent::SimpleTick(float DeltaTime)
{
	// 1) 수평 이동 의도(일단 XY만 이동)
	FVector InputVector = ConsumeInputVector();
	InputVector = FVector(InputVector.X, InputVector.Y, 0.f);
	InputVector = InputVector.GetSafeNormal();
	
	// 회전 처리
	FRotator TargetRotation = InputVector.Rotation();
	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
	if (!InputVector.IsNearlyZero())
	{
		NewRotation = FMath::RInterpTo(NewRotation, TargetRotation, DeltaTime, 10.0f);
		NewRotation.Pitch = 0.0f; 
		NewRotation.Roll = 0.0f;
	}

	// 지상 상태면, 이전 프레임 바닥 노멀 기준으로 투영하고 싶지만
	// "1쿼리" 정책이라 이번 프레임에서 바닥을 찾고 그 노멀로 투영해도 충분히 자연스럽습니다.
	FVector DeltaXY = FVector(InputVector.X, InputVector.Y, 0.f) * DeltaTime * MaxSpeed;
	DeltaXY = DeltaXY.GetClampedToMaxSize(MaxSpeed);

	// 일단 XY 적용 (충돌 없이 텔레포트처럼 이동)
	// ※ 여기서 수평 충돌까지 하려면 MoveComponent의 Sweep=true가 필요하지만 쿼리 비용이 늘어납니다.
	FVector Location = UpdatedComponent->GetComponentLocation();
	Location += DeltaXY;

	// 2) 바닥 찾기(딱 1회)
	FHitResult GroundHit;
	const bool bHasGround = FindGround(Location, GroundHit, DeltaTime);

	if (bHasGround)
	{
		// 지상 처리: 중력 제거 + 바닥에 스냅
		GravityVelocity = FVector::ZeroVector;

		FVector TangentVel = FVector::VectorPlaneProject(InputVector, GroundHit.ImpactNormal);
		if (!CheckWalkable(GroundHit.ImpactNormal))
		{
			TangentVel = FVector::ZeroVector;
		}

		// 슬로프를 따라가도록 수평 속도를 평면 투영
		if (GroundHit.ImpactNormal != FVector::UpVector)
		{
			Location.X = UpdatedComponent->GetComponentLocation().X + TangentVel.X * DeltaTime * MaxSpeed;
			Location.Y = UpdatedComponent->GetComponentLocation().Y + TangentVel.Y * DeltaTime * MaxSpeed;
		}

		// 스텝 업/다운 제한
		const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
		const float TargetZ  = GroundHit.Location.Z; // 스윕 히트 위치
		const float DeltaZ   = TargetZ - CurrentZ;

		if (DeltaZ > MaxStepUp)
		{
			// 너무 높은 단차: 이동 취소(또는 Falling)
			Location.X = UpdatedComponent->GetComponentLocation().X;
			Location.Y = UpdatedComponent->GetComponentLocation().Y;
			Location.Z = CurrentZ;
		}
		else if (DeltaZ < -MaxStepDown)
		{
			// 너무 급격한 낙차: Falling로 전환
		}
		else
		{
			// 허용 범위면 Z 스냅
			Location.Z = TargetZ;
		}
	}
	else
	{
		// 3) 공중 처리: 중력 적용
		GravityVelocity.Z += GetGravityZ() * DeltaTime;
		GravityVelocity.Z = FMath::Clamp(GravityVelocity.Z, -MaxFallSpeed, MaxFallSpeed);
		Location.Z += GravityVelocity.Z * DeltaTime;
	}

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_SetWorldLocation);
		UpdatedComponent->SetWorldLocationAndRotation(Location, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

bool UUKSimpleMovementComponent::CheckWalkable(const FVector& FloorNormal) const
{
	const float Cos = FMath::Cos(FMath::DegreesToRadians(WalkableFloorAngleDeg));
	// FloorNormal · Up >= cos(angle) 이면 워커블
	return FVector::DotProduct(FloorNormal, FVector::UpVector) >= Cos;
}

bool UUKSimpleMovementComponent::FindGround(const FVector& QueryOrigin, FHitResult& OutHit, const float DeltaTime) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_FindGround);

	FVector TempGravityVelocity = GravityVelocity;
	TempGravityVelocity.Z += GetGravityZ() * DeltaTime;
	TempGravityVelocity.Z = FMath::Clamp(GravityVelocity.Z, -MaxFallSpeed, MaxFallSpeed);
	float LocationZ = TempGravityVelocity.Z * DeltaTime;
	
	// QueryOrigin은 보통 "캡슐 중심"
	// 바닥 스윕은 살짝 위에서 아래로
	FVector Location = UpdatedComponent->GetComponentLocation();
	const FVector Start = QueryOrigin + FVector(0,0, 10.f);
	const FVector End   = QueryOrigin + FVector(0,0, -10);
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SimpleMovementSweep), false);
	Params.AddIgnoredActor(PawnOwner);

	// 캡슐 형태로 아래 스윕(라인보다 안정적: 작은 단차/경사에서 바닥 놓치는 걸 줄임)
	const FCollisionShape CollisionShape = UpdatedPrimitive->GetCollisionShape();

	return GetWorld()->SweepSingleByChannel(
		OutHit,
		Start, End,
		FQuat::Identity,
		UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn,
		CollisionShape,
		Params
	);
}