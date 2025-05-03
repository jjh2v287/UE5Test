// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomPBDRopeActor.h"

// Sets default values
ACustomPBDRopeActor::ACustomPBDRopeActor()
{
 	// Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ACustomPBDRopeActor::BeginPlay()
{
	Super::BeginPlay();

	// 파티클 배열 초기화
	Particles.Empty();
	Particles.SetNum(NumSegments + 1); // 파티클 수는 세그먼트 수 + 1

	// 시작점과 끝점 사이의 벡터
	const FVector Delta = EndLocation - StartLocation;
	const FVector Direction = Delta.GetSafeNormal();
	const float InitialSegmentLength = Delta.Size() / FMath::Max(1, NumSegments); // 초기 세그먼트 길이

	for (int32 i = 0; i <= NumSegments; ++i)
	{
		Particles[i].Position = StartLocation + Direction * InitialSegmentLength * i;
		Particles[i].OldPosition = Particles[i].Position; // 초기 속도 0
		Particles[i].PredictedPosition = Particles[i].Position;
		Particles[i].Velocity = FVector::ZeroVector;

		// 첫 번째 파티클은 고정
		if (i == 0)
		{
			Particles[i].InvMass = 0.0f;
		}
		else
		{
			Particles[i].InvMass = 1.0f; // 예시: 모든 파티클 질량 동일 (고정점 제외)
		}
	}
}

// Called every frame
void ACustomPBDRopeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Particles.Num() < 2) return; // 파티클이 충분하지 않으면 시뮬레이션 불가

	// 1. 외부 힘 적용 및 위치 예측
	PredictPositions(DeltaTime);

	// 2. 제약 조건 해결 (여러 번 반복)
	SolveConstraints();

	// 3. 속도 및 최종 위치 업데이트
	UpdateVelocitiesAndPositions(DeltaTime);

	// 4. 디버그 시각화
	if (bDrawDebug)
	{
		DrawDebug();
	}
}

// 1. 외부 힘(중력)을 적용하고 다음 위치 예측
void ACustomPBDRopeActor::PredictPositions(float DeltaTime)
{
	for (FRopeParticle& Particle : Particles)
	{
		if (Particle.IsFixed()) // 고정된 파티클은 위치 예측 안 함
		{
			Particle.PredictedPosition = Particle.Position; // 현재 위치 유지
			continue;
		}

		// 중력 적용 (질량 무시, 가속도 직접 적용)
		FVector Acceleration = Gravity;

		// 이전 위치로부터 현재 속도 추정 (PBD에서는 이 속도를 사용해 예측)
		// 더 정확하게는 이전 프레임의 Velocity를 사용할 수 있음
		// Particle.Velocity += Acceleration * DeltaTime; // 속도 업데이트 (선택적)

		// 다음 위치 예측 (Explicit Euler 방식)
		// Particle.PredictedPosition = Particle.Position + Particle.Velocity * DeltaTime;
		// 또는 Verlet 스타일 예측:
		Particle.PredictedPosition = Particle.Position + (Particle.Position - Particle.OldPosition) * Damping + Acceleration * DeltaTime * DeltaTime;
	}
}

// 2. 거리 제약 조건 해결 (반복)
void ACustomPBDRopeActor::SolveConstraints()
{
	for (int32 Iteration = 0; Iteration < SolverIterations; ++Iteration)
	{
		// 각 세그먼트(인접한 두 파티클)에 대해 거리 제약 조건 해결
		for (int32 i = 0; i < NumSegments; ++i)
		{
			SolveDistanceConstraint(Particles[i], Particles[i + 1], SegmentLength);
		}

		// (선택 사항) 여기에 굽힘(Bending) 제약 조건이나 충돌(Collision) 처리 로직 추가 가능
	}
}

// 거리 제약 조건 해결 (단일 세그먼트)
void ACustomPBDRopeActor::SolveDistanceConstraint(FRopeParticle& ParticleA, FRopeParticle& ParticleB, float DesiredDistance)
{
	// 두 파티클의 예측 위치 사이의 벡터 및 현재 거리 계산
	FVector Delta = ParticleB.PredictedPosition - ParticleA.PredictedPosition;
	float CurrentDistance = Delta.Size();

	// 거리가 너무 작으면 (0에 가까우면) 계산 생략 (0으로 나누기 방지)
	if (CurrentDistance < KINDA_SMALL_NUMBER) return;

	// 목표 거리와의 차이 계산
	float Error = CurrentDistance - DesiredDistance;

	// 보정량 계산 (역질량 가중치 적용)
	float TotalInvMass = ParticleA.InvMass + ParticleB.InvMass;
	if (TotalInvMass <= KINDA_SMALL_NUMBER) return; // 두 점 모두 고정되어 있으면 이동 불가

	// 각 파티클에 적용될 보정 벡터
	// (Error / CurrentDistance) 는 정규화된 방향 벡터에 곱해질 스칼라 오차 비율
	FVector Correction = Delta * (Error / CurrentDistance) / TotalInvMass;

	// 예측 위치 수정
	if (!ParticleA.IsFixed())
	{
		ParticleA.PredictedPosition += Correction * ParticleA.InvMass;
	}
	if (!ParticleB.IsFixed())
	{
		ParticleB.PredictedPosition -= Correction * ParticleB.InvMass;
	}
}


// 3. 최종 위치 및 속도 업데이트
void ACustomPBDRopeActor::UpdateVelocitiesAndPositions(float DeltaTime)
{
	if (DeltaTime <= KINDA_SMALL_NUMBER) return;

	for (FRopeParticle& Particle : Particles)
	{
		if (Particle.IsFixed()) continue; // 고정점은 업데이트 안 함

		// 속도 계산 (위치 변화로부터)
		Particle.Velocity = (Particle.PredictedPosition - Particle.Position) / DeltaTime;

		// 이전 위치 저장
		Particle.OldPosition = Particle.Position;

		// 최종 위치 업데이트
		Particle.Position = Particle.PredictedPosition;
	}
}

// 4. 디버그 시각화
void ACustomPBDRopeActor::DrawDebug()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 파티클 위치에 구체 그리기
	for (const FRopeParticle& Particle : Particles)
	{
		DrawDebugSphere(World, Particle.Position, DebugSphereRadius, 8, FColor::Cyan, false, -1.0f);
	}

	// 세그먼트 연결선 그리기
	for (int32 i = 0; i < NumSegments; ++i)
	{
		DrawDebugLine(World, Particles[i].Position, Particles[i + 1].Position, FColor::Yellow, false, -1.0f);
	}
}