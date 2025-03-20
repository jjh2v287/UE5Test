// SteeringCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "SteeringCharacter.generated.h"

UCLASS()
class ZKGAME_API ASteeringCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASteeringCharacter();

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    // 이동 관련 함수
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetMoveTarget(const FVector& TargetLocation);

protected:
    // 회피 및 분리 파라미터
    UPROPERTY(EditAnywhere, Category = "Steering|Detection")
    float DetectionRadius = 400.0f;

    UPROPERTY(EditAnywhere, Category = "Steering|Separation")
    float SeparationRadiusScale = 0.9f;
    
    UPROPERTY(EditAnywhere, Category = "Steering|Separation")
    float SeparationDistance = 75.0f;
    
    UPROPERTY(EditAnywhere, Category = "Steering|Separation")
    float SeparationStiffness = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Steering|Avoidance")
    float PredictiveAvoidanceTime = 2.5f;
    
    UPROPERTY(EditAnywhere, Category = "Steering|Avoidance")
    float PredictiveAvoidanceRadiusScale = 0.65f;
    
    UPROPERTY(EditAnywhere, Category = "Steering|Avoidance")
    float PredictiveAvoidanceDistance = 75.0f;
    
    UPROPERTY(EditAnywhere, Category = "Steering|Avoidance")
    float PredictiveAvoidanceStiffness = 700.0f;

    UPROPERTY(EditAnywhere, Category = "Steering|Environment")
    float EnvironmentSeparationDistance = 50.0f;
    
    UPROPERTY(EditAnywhere, Category = "Steering|Environment")
    float EnvironmentSeparationStiffness = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Steering|Movement")
    float MaxAcceleration = 800.0f;

    UPROPERTY(EditAnywhere, Category = "Steering|Movement")
    float MaxSpeed = 100.0f;

    // 에이전트 감지용 콜리전 컴포넌트
    UPROPERTY(VisibleAnywhere, Category = "Steering")
    class USphereComponent* DetectionSphere;

private:
    // 근처의 SteeringCharacter 탐지
    TArray<ASteeringCharacter*> GetNearbySteeringCharacters() const;

    // 회피력 계산 함수
    FVector CalculateSeparationForce(const TArray<ASteeringCharacter*>& NearbyActors);
    FVector CalculatePredictiveAvoidanceForce(const TArray<ASteeringCharacter*>& NearbyActors);
    FVector CalculateEnvironmentAvoidanceForce();
    FVector CalculateSteeringForce(float DeltaTime);

    // 내부 유틸리티 함수
    float ComputeClosestPointOfApproach(const FVector& RelPos, const FVector& RelVel, const float TotalRadius, const float TimeHorizon) const;

    // 현재 목표 위치
    FVector GoalLocation;
    
    // 현재 조향 방향
    FVector SteeringForce;
};