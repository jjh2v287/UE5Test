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
		RootMotionUpdate(TempRootMotionParams);
	}
	else
	{
		// 루트 모션 X
		SimpleWalkingUpdate(DeltaTime);
	}
}

void UUKSimpleMovementComponent::RootMotionUpdate(const FRootMotionMovementParams& TempRootMotionParams)
{
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

void UUKSimpleMovementComponent::SimpleWalkingUpdate(float DeltaTime)
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

	// 수평 이동 움직일 거리 계산
	FVector DeltaXY = FVector(InputVector.X, InputVector.Y, 0.f) * DeltaTime * MaxWalkSpeed;
	DeltaXY = DeltaXY.GetClampedToMaxSize(MaxWalkSpeed);

	// 일단 XY 적용 (충돌 없이 텔레포트처럼 이동)
	// ※ 여기서 수평 충돌까지 하려면 MoveComponent의 Sweep=true가 필요하지만 쿼리 비용이 늘어남.
	FVector Location = UpdatedComponent->GetComponentLocation();
	Location += DeltaXY;

	// 2) 바닥 찾기
	FHitResult GroundHit;
	const bool bHasGround = FindGround(Location, GroundHit, DeltaTime);

	if (bHasGround)
	{
		// 지상 처리: 중력 제거 + 바닥에 스냅
		GravityVelocity = FVector::ZeroVector;

		// 이동 가능한 경사면 체크
		FVector TangentVel = FVector::VectorPlaneProject(InputVector, GroundHit.ImpactNormal);
		if (!CheckWalkable(GroundHit.ImpactNormal))
		{
			// 오르막일때만 막고 내리막에서는 이동 가능하게
			const bool bDownhill = (TangentVel.SizeSquared() > KINDA_SMALL_NUMBER) && (TangentVel.GetSafeNormal().Z < -0.05f);
			const bool bUphill = (TangentVel.SizeSquared() > KINDA_SMALL_NUMBER) && (TangentVel.GetSafeNormal().Z > 0.05f);
			TangentVel = bUphill ? FVector::ZeroVector : TangentVel;
		}

		// 슬로프를 따라가도록 수평 속도를 평면 투영
		if (GroundHit.ImpactNormal != FVector::UpVector)
		{
			Location.X = UpdatedComponent->GetComponentLocation().X + TangentVel.X * DeltaTime * MaxWalkSpeed;
			Location.Y = UpdatedComponent->GetComponentLocation().Y + TangentVel.Y * DeltaTime * MaxWalkSpeed;
		}

		// 스텝 업/다운 제한
		const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
		const float TargetZ  = GroundHit.Location.Z; // 스윕 히트 위치
		const float DeltaZ   = TargetZ - CurrentZ;

		if (DeltaZ > MaxStepHeight)
		{
			// 너무 높은 단차: 이동 취소(또는 Falling)
			Location.X = UpdatedComponent->GetComponentLocation().X;
			Location.Y = UpdatedComponent->GetComponentLocation().Y;
			Location.Z = CurrentZ;
		}
		else if (DeltaZ < -MaxStepHeight)
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
		GravityVelocity.Z += GetGravityZ() * DeltaTime * DeltaTime;
		GravityVelocity.Z = FMath::Clamp(GravityVelocity.Z, -MaxFallSpeed, MaxFallSpeed);
		Location.Z += GravityVelocity.Z;
	}

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_SetWorldLocation);
		UpdatedComponent->SetWorldLocationAndRotation(Location, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

bool UUKSimpleMovementComponent::CheckWalkable(const FVector& FloorNormal) const
{
	const float Cos = FMath::Cos(FMath::DegreesToRadians(GetWalkableFloorAngle()));
	// FloorNormal · Up >= cos(angle) 이면 워커블
	return FVector::DotProduct(FloorNormal, FVector::UpVector) >= Cos;
}

bool UUKSimpleMovementComponent::FindGround(const FVector& QueryOrigin, FHitResult& OutHit, const float DeltaTime) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SimpleMovement_FindGround);

	// QueryOrigin은 캡슐 중심으로 수평 이동
	// 바닥 스윕은 살짝 위에서 아래로
	const FVector Start = QueryOrigin + FVector(0,0, GroundSnapDistance);
	const FVector End   = QueryOrigin + FVector(0,0, -GroundSnapDistance);
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SimpleMovementSweep), false);
	Params.AddIgnoredActor(PawnOwner);

	// 캡슐 형태로 아래 스윕
	const FCollisionShape CollisionShape = UpdatedPrimitive->GetCollisionShape();
	const ECollisionChannel CollisionChannel = UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn;
	
	return GetWorld()->SweepSingleByChannel(
		OutHit,
		Start, End,
		FQuat::Identity,
		CollisionChannel,
		CollisionShape,
		Params
	);
}