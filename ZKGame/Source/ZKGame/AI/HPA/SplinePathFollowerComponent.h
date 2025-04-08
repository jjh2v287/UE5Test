// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "SplinePathFollowerComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZKGAME_API USplinePathFollowerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USplinePathFollowerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * 이동할 경로 포인트를 설정합니다. 기존 경로는 초기화됩니다.
     * @param NewPathPoints - 새로운 경로 포인트 배열 (FVector 배열).
     * @param CoordinateSpace - NewPathPoints 배열의 좌표계 (World 또는 Local).
     * @param bUpdateSpline - 즉시 스플라인을 업데이트할지 여부.
     */
    UFUNCTION(BlueprintCallable, Category = "Spline Path Follower")
    void SetPathPoints(const TArray<FVector>& NewPathPoints, ESplineCoordinateSpace::Type CoordinateSpace = ESplineCoordinateSpace::World, bool bUpdateSpline = true);

    /**
     * 스플라인 경로를 따라 이동을 시작합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "Spline Path Follower")
    void StartMovement(FVector TargetLocation);

    /**
     * 스플라인 경로 이동을 중지합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "Spline Path Follower")
    void StopMovement();

    /**
     * 이동을 초기 위치(시작점)로 리셋합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "Spline Path Follower")
    void ResetMovement();

    /**
     * 현재 이동 중인지 여부를 반환합니다.
     */
    UFUNCTION(BlueprintPure, Category = "Spline Path Follower")
    bool IsMoving() const { return bIsMoving; }

    /**
     * 액터가 이동할 속도 (cm/s).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Path Follower", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float MovementSpeed = 100.0f;

    /**
     * 이동 완료 시 자동으로 루프할지 여부.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Path Follower")
    bool bLoop = false;

    /**
     * 액터가 스플라인의 방향을 따라 회전할지 여부.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Path Follower")
    bool bFollowSplineRotation = true;

    /**
     * 액터 회전 속도 (더 높은 값이 더 빠르게 회전).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Path Follower", meta = (EditCondition = "bFollowSplineRotation", ClampMin = "0.0", UIMin = "0.0"))
    float RotationInterpSpeed = 5.0f;

    /**
     * 디버그용으로 생성된 스플라인을 그릴지 여부.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Path Follower")
    bool bDrawDebugSpline = true;


private:
    /** 경로 생성을 위한 내부 스플라인 컴포넌트 */
    UPROPERTY(VisibleAnywhere, Category = "Spline Path Follower", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USplineComponent> SplineComp;

    /** 소유 액터 캐시 */
    UPROPERTY()
    TObjectPtr<AActor> OwnerActor;

    /** 소유 액터의 캐릭터 무브먼트 컴포넌트 캐시 (캐릭터인 경우) */
    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> MovementComponent;

    /** 현재 이동 중인지 나타내는 플래그 */
    bool bIsMoving = false;

    /** 현재 스플라인을 따라 이동한 거리 */
    float CurrentDistance = 0.0f;

    /** 입력된 경로 포인트 저장 (디버깅 또는 재설정 용도) */
    TArray<FVector> InputPathPoints;
    ESplineCoordinateSpace::Type InputCoordinateSpace;
};