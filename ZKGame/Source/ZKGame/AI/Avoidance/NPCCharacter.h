// NPCCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NPCCharacter.generated.h"

UCLASS()
class ZKGAME_API ANPCCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ANPCCharacter();

    virtual void Tick(float DeltaTime) override;

    // 목표 위치 설정 함수
    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    void SetTargetLocation(const FVector& NewLocation);

    // 기본 이동 파라미터
    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    float MaxMovementSpeed = 100.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    float AcceptanceRadius = 50.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    float TargetWeight = 1.0f;

    // 장애물 회피 파라미터
    UPROPERTY(EditAnywhere, Category = "AI|Obstacle Avoidance")
    float ObstacleDetectionRadius = 200.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Obstacle Avoidance")
    float ObstacleDetectionForwardDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Obstacle Avoidance")
    float ObstacleAvoidanceForce = 800.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Obstacle Avoidance")
    float MinObstacleDistance = 100.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Obstacle Avoidance")
    float AvoidanceWeight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Obstacle Avoidance")
    TArray<TEnumAsByte<EObjectTypeQuery>> ObstacleObjectTypes;

    // NPC 분리(Separation) 파라미터
    UPROPERTY(EditAnywhere, Category = "AI|Separation")
    float SeparationRadius = 250.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Separation")
    float SeparationForce = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Separation")
    float SeparationWeight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Separation")
    TSubclassOf<ANPCCharacter> NPCClass;

    // 디버그 파라미터
    UPROPERTY(EditAnywhere, Category = "AI|Debug")
    bool bShowDebugLines = true;

    // 현재 목표 위치
    UPROPERTY(BlueprintReadOnly, Category = "AI|Navigation")
    FVector TargetLocation;

    // 목표에 도달했는지 확인
    UPROPERTY(BlueprintReadOnly, Category = "AI|Navigation")
    bool bReachedTarget = true;

    FVector PreFinalDirection = FVector::ZeroVector;
    FVector FinalDirection = FVector::ZeroVector;

protected:
    virtual void BeginPlay() override;

private:
    // 목표 방향 계산 함수
    FVector CalculateTargetDirection();
    
    // 장애물 회피 관련 함수
    FVector CalculateAvoidanceVector(float DeltaTime);
    FVector SteerAwayFromObstacle(ANPCCharacter* OtherNPC, float DeltaTime);
    
    // NPC 분리 관련 함수
    FVector CalculateSeparationVector(float DeltaTime);

    void FindNearbyActors(TArray<ANPCCharacter*>& OutNearbyActors, float SearchRadius);
    
    // 최종 이동 방향 계산 및 적용
    void MoveTowardTarget(float DeltaTime);
};