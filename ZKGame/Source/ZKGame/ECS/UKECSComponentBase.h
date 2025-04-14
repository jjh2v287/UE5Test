// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKAgentManager.h"
#include "UObject/Object.h"
#include "UKECSComponentBase.generated.h"

/** 모든 컴포넌트의 기본 구조체 */
USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSComponentBase
{
	GENERATED_BODY()
};

// --- 예시 컴포넌트 타입들 ---
USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSPositionComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FVector Position = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSVelocityComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FVector Velocity = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSMovementSpeedComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	float Speed = 100.0f;
};

USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSForwardMoveComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FVector TargetLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FRotator Rotator = FRotator::ZeroRotator;
	
	UPROPERTY(VisibleAnywhere, Category = "ECS")
	FVector ForwardVector = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	float Speed = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "ECS")
	TWeakObjectPtr<AActor> OwnerActor = nullptr;
};

USTRUCT(BlueprintType)
struct ZKGAME_API FUKECSAvoidMoveComponent : public FUKECSComponentBase
{
	GENERATED_BODY()

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	FAgentHandle AgentHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	TWeakObjectPtr<AUKAgent> OwnerActor = nullptr;

	// 이동 타겟 정보
	FVector MoveTargetLocation;
	FVector MoveTargetForward;
	float DesiredSpeed = 0.0f;
	float DistanceToGoal = 0.0f;
	
	// 현재 상태
	FVector CurrentVelocity;
	FVector DesiredVelocity;
	FVector SteeringForce;

	FVector SeparationForce;
	FVector PredictiveAvoidanceForce;
	FVector SteerForce;

	FVector NewLocation;
	FRotator NewRotation;
};