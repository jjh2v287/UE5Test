// Fill out your copyright notice in the Description page of Project Settings.

#include "NpcSimpleMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

void UNpcSimpleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	MovementMode = EMovementMode::MOVE_Walking;
}

void UNpcSimpleMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	Super::RequestDirectMove(MoveVelocity, bForceMaxSpeed);
	
	// AIController가 계산한 목표 속도(MoveVelocity)를 받아옵니다.
	// 이를 통해 별도의 AddInputVector 호출 없이도 AI가 의도한 방향을 알 수 있습니다.
	FVector MoveDirection = MoveVelocity.GetSafeNormal();
    
	// NPC의 이동 입력을 강제로 주입합니다.
	// (PawnMovementComponent의 내부 입력 벡터에 값을 넣습니다)
	AddInputVector(MoveDirection);
	
	SetDesiredVelocityXY(MoveDirection);
}

void UNpcSimpleMovementComponent::SetDesiredVelocityXY(const FVector& InVelXY)
{
	DesiredVelXY = FVector(InVelXY.X, InVelXY.Y, 0.f);
	DesiredVelXY = DesiredVelXY.GetClampedToMaxSize(MaxSpeed);
}

bool UNpcSimpleMovementComponent::CheckWalkable(const FVector& FloorNormal) const
{
	const float Cos = FMath::Cos(FMath::DegreesToRadians(WalkableFloorAngleDeg));
	// FloorNormal · Up >= cos(angle) 이면 워커블
	return FVector::DotProduct(FloorNormal, FVector::UpVector) >= Cos;
}

bool UNpcSimpleMovementComponent::FindGround(const FVector& QueryOrigin, FHitResult& OutHit) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(NpcSimpleMovement_FindGround);
	const AActor* Owner = GetOwner();
	if (!Owner) return false;

	const UCapsuleComponent* Capsule = Owner->FindComponentByClass<UCapsuleComponent>();
	if (!Capsule) return false;

	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	// QueryOrigin은 보통 "캡슐 중심"
	// 바닥 스윕은 살짝 위에서 아래로
	const FVector Start = QueryOrigin + FVector(0,0, 10.f);
	const FVector End   = QueryOrigin - FVector(0,0, GroundSnapDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NpcGroundSweep), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(Owner);

	// 캡슐 형태로 아래 스윕(라인보다 안정적: 작은 단차/경사에서 바닥 놓치는 걸 줄임)
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	return GetWorld()->SweepSingleByChannel(
		OutHit,
		Start, End,
		FQuat::Identity,
		UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn,
		Shape,
		Params
	);
}

void UNpcSimpleMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(NpcSimpleMovement_Tick);

	if (!UpdatedComponent || ShouldSkipUpdate(DeltaTime))
		return;

	FVector Location = UpdatedComponent->GetComponentLocation();

	// 1) 수평 이동 의도(일단 XY만 이동)
	// FVector VelXY = DesiredVelXY;
	FVector VelXY = ConsumeInputVector();

	// 지상 상태면, 이전 프레임 바닥 노멀 기준으로 투영하고 싶지만
	// "1쿼리" 정책이라 이번 프레임에서 바닥을 찾고 그 노멀로 투영해도 충분히 자연스럽습니다.
	FVector DeltaXY = FVector(VelXY.X, VelXY.Y, 0.f) * DeltaTime * MaxSpeed;
	DeltaXY = DeltaXY.GetClampedToMaxSize(MaxSpeed);

	// 일단 XY 적용 (충돌 없이 텔레포트처럼 이동)
	// ※ 여기서 수평 충돌까지 하려면 MoveComponent의 Sweep=true가 필요하지만 쿼리 비용이 늘어납니다.
	Location += DeltaXY;

	// 2) 바닥 찾기(딱 1회)
	FHitResult GroundHit;
	const bool bHasGround = FindGround(Location, GroundHit);

	if (bHasGround && CheckWalkable(GroundHit.ImpactNormal))
	{
		// 지상 처리: 중력 제거 + 바닥에 스냅
		bGrounded = true;
		VerticalSpeed = 0.f;

		// 슬로프를 따라가도록 수평 속도를 평면 투영
		const FVector TangentVel = ProjectOnPlane(VelXY, GroundHit.ImpactNormal);
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
			bGrounded = false;
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
		bGrounded = false;
		VerticalSpeed += GravityZ * DeltaTime;
		VerticalSpeed = FMath::Clamp(VerticalSpeed, -TerminalSpeed, TerminalSpeed);

		Location.Z += VerticalSpeed * DeltaTime;
	}

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(NpcSimpleMovement_SetWorldLocation);
		UpdatedComponent->SetWorldLocation(Location, false, nullptr, ETeleportType::None);
	}
}
