// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h" // 디버그 드로잉을 위해 필요
#include "CustomPBDRopeActor.generated.h"

// 로프를 구성하는 파티클(점)의 데이터를 저장하는 구조체
USTRUCT(BlueprintType)
struct ZKGAME_API FRopeParticle
{
	GENERATED_BODY()

	// 현재 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Rope Particle")
	FVector Position = FVector::ZeroVector;

	// 이전 프레임 위치 (속도 계산용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Rope Particle")
	FVector OldPosition = FVector::ZeroVector;

	// 현재 프레임에서 예측된 위치 (제약 조건 해결 전)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Rope Particle")
	FVector PredictedPosition = FVector::ZeroVector;

	// 현재 속도
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Rope Particle")
	FVector Velocity = FVector::ZeroVector;

	// 질량의 역수 (0이면 고정된 파티클)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope Particle")
	float InvMass = 1.0f; // 0.0f 이면 고정됨

	// 기본 생성자
	FRopeParticle() : InvMass(1.0f) {}

	// 고정 여부 확인
	bool IsFixed() const { return InvMass <= KINDA_SMALL_NUMBER; }
};

UCLASS()
class ZKGAME_API ACustomPBDRopeActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACustomPBDRopeActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// --- 로프 속성 ---

	// 로프를 구성하는 파티클 배열
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Rope")
	TArray<FRopeParticle> Particles;

	// 로프의 총 세그먼트(마디) 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Parameters")
	int32 NumSegments = 10;

	// 각 세그먼트의 목표 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Parameters")
	float SegmentLength = 20.0f;

	// 로프의 시작점 (월드 좌표)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Parameters", Meta = (MakeEditWidget = true))
	FVector StartLocation = FVector(0, 0, 0);

	// 로프의 끝점 (월드 좌표, 시작점 기준 상대 좌표 아님)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Parameters", Meta = (MakeEditWidget = true))
	FVector EndLocation = FVector(100, 0, 0);

	// PBD 제약 조건 솔버 반복 횟수 (높을수록 정확하지만 무거움)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Parameters")
	int32 SolverIterations = 10;

	// 적용할 중력 가속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Parameters")
	FVector Gravity = FVector(0.0f, 0.0f, -980.0f);

	// 속도 감쇠 계수 (0 ~ 1, 에너지를 잃게 함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Damping = 0.99f;

	// 디버그 구체 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Debug")
	float DebugSphereRadius = 5.0f;

	// 디버그 드로잉 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Rope|Debug")
	bool bDrawDebug = true;


private:
	// --- PBD 시뮬레이션 함수 ---

	// 외부 힘(중력)을 적용하고 다음 위치 예측
	void PredictPositions(float DeltaTime);

	// 거리 제약 조건 해결 (반복)
	void SolveConstraints();

	// 최종 위치 및 속도 업데이트
	void UpdateVelocitiesAndPositions(float DeltaTime);

	// 거리 제약 조건 해결 (단일 세그먼트)
	void SolveDistanceConstraint(FRopeParticle& ParticleA, FRopeParticle& ParticleB, float DesiredDistance);

	// 디버그 시각화
	void DrawDebug();
};