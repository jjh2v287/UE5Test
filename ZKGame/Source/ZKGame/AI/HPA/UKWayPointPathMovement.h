// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UKWayPointPathMovement.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZKGAME_API UUKWayPointPathMovement : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UUKWayPointPathMovement();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 목표 위치로 이동 시작
    UFUNCTION(BlueprintCallable, Category = "PathMovement")
    void MoveToLocation(const FVector TargetLocation);

    // 이동 중지
    UFUNCTION(BlueprintCallable, Category = "PathMovement")
    void StopMovement();

    // 현재 이동 중인지 확인
    UFUNCTION(BlueprintPure, Category = "PathMovement")
    bool IsMoving() const { return bIsMoving; }

    // 목표 지점 도달 시 호출될 델리게이트
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDestinationReached);
    UPROPERTY(BlueprintAssignable, Category = "PathMovement")
    FOnDestinationReached OnDestinationReached;

protected:
    // 현재 따라가는 경로
    UPROPERTY()
    TArray<FVector> CurrentPath;

    // 현재 목표로 하는 웨이포인트 인덱스
    UPROPERTY()
    int32 CurrentWaypointIndex;

    // 이동 중인지 여부
    UPROPERTY()
    bool bIsMoving;

    // 목적지에 도달했을 때 호출되는 함수
    void DestinationReached();

    // 다음 웨이포인트로 이동
    bool MoveToNextWaypoint();

    // 웨이포인트를 향해 이동
    void MoveTowardsWaypoint(float DeltaTime);

public:
    // 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PathMovement")
    float MovementSpeed = 300.0f;

    // 웨이포인트 도달 허용 오차
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PathMovement")
    float AcceptanceRadius = 50.0f;

    // 회전 속도 (초당 각도)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PathMovement")
    float RotationSpeed = 180.0f;

    // 이동 시 회전 사용 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PathMovement")
    bool bUseRotation = true;
};