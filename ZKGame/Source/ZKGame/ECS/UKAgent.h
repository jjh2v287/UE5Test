// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKAgentManager.h"
#include "GameFramework/Actor.h"
#include "UKAgent.generated.h"

UCLASS()
class ZKGAME_API AUKAgent : public AActor
{
	GENERATED_BODY()

public:
	AUKAgent();
	
	virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 분리 및 회피 벡터 계산 함수들
    FVector CalculateSeparationForce(const TArray<AUKAgent*>& NearbyActors);
    FVector CalculatePredictiveAvoidanceForce(const TArray<AUKAgent*>& NearbyActors);
    FVector CalculateEnvironmentAvoidanceForce();
    
    // 근처 액터 탐색
    void FindNearbyActors(TArray<AUKAgent*>& OutNearbyActors, float SearchRadius);
    
    // 충돌 접근 시간 계산 (Closest Point of Approach)
    float ComputeClosestPointOfApproach(const FVector& RelPos, const FVector& RelVel, float TotalRadius, float TimeHorizon);
    
    // 이동 관련 함수
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetMoveTarget(const FVector& TargetLocation, const FVector& TargetForward);
    
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetDesiredSpeed(float Speed);

    // 조향 벡터 계산
    FVector CalculateSteeringForce(float DeltaTime);

	FAgentHandle GetAgentHandle() const
	{
		return AgentHandle;
	}

	void SetAgentHandle(FAgentHandle Handle)
	{
		AgentHandle = Handle;
	}

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool IsRunECS = false;
	
    // 이동 속성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxSpeed = 100.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxAcceleration = 500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float AgentRadius = 50.0f;
    
    // 회피 속성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleDetectionDistance = 400.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float SeparationRadiusScale = 0.9f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleSeparationDistance = 75.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleSeparationStiffness = 250.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float PredictiveAvoidanceTime = 2.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float PredictiveAvoidanceRadiusScale = 0.65f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float PredictiveAvoidanceDistance = 75.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstaclePredictiveAvoidanceStiffness = 700.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float StandingObstacleAvoidanceScale = 0.65f;
    
    // 방향 보간 속성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float OrientationSmoothingTime = 0.3f;
    
    // 현재 상태
    FVector CurrentVelocity;
    FVector DesiredVelocity;
    FVector SteeringForce;
    
    // 이동 타겟 정보
    FVector MoveTargetLocation;
    FVector MoveTargetForward;
    float DesiredSpeed = 0.0f;
    float DistanceToGoal = 0.0f;

	FAgentHandle AgentHandle;
    
    // 디버그용
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDrawDebug = false;
};
