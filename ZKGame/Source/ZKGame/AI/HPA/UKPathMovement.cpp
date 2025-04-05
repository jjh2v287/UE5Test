// Fill out your copyright notice in the Description page of Project Settings.

#include "UKPathMovement.h"
#include "UKNavigationManager.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UUKPathMovement::UUKPathMovement()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    // 기본값 초기화
    bIsMoving = false;
    CurrentWaypointIndex = 0;
    CurrentVelocity = FVector::ZeroVector;
}

// Called when the game starts
void UUKPathMovement::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame
void UUKPathMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 이동 중이라면 스티어링 힘 계산 및 적용
    if (bIsMoving)
    {
        // 스티어링 힘 계산
        FVector SteeringForce = CalculateSteeringForce(DeltaTime);
        
        // 스티어링 힘 적용
        ApplySteeringForce(SteeringForce, DeltaTime);
    }
}

void UUKPathMovement::Move(const FVector TargetLocation)
{
    // 네비게이션 매니저 존재 확인
    if (!UUKNavigationManager::Get())
    {
        UE_LOG(LogTemp, Warning, TEXT("UKPathMovement: Navigation Manager not found!"));
        return;
    }

    // 현재 위치 가져오기
    const FVector StartLocation = GetOwner()->GetActorLocation();
    
    // 경로 찾기
    CurrentPath = UUKNavigationManager::Get()->FindPath(StartLocation, TargetLocation);
    
    // 경로가 비어있는지 확인
    if (CurrentPath.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("UKPathMovement: No path found to target location!"));
        return;
    }

    // 첫 번째 웨이포인트부터 시작
    CurrentWaypointIndex = 0;
    
    // 목표 위치 설정
    UpdateGoalLocation();
    
    // 현재 속도 초기화
    CurrentVelocity = FVector::ZeroVector;
    
    // 이동 상태 시작
    bIsMoving = true;

    UE_LOG(LogTemp, Log, TEXT("UKPathMovement: Starting movement with %d waypoints"), CurrentPath.Num());
}

void UUKPathMovement::StopMovement()
{
    bIsMoving = false;
    CurrentPath.Empty();
    CurrentWaypointIndex = 0;
    CurrentVelocity = FVector::ZeroVector;
}

void UUKPathMovement::DestinationReached()
{
    UE_LOG(LogTemp, Log, TEXT("UKPathMovement: Destination reached"));
    
    // 이동 상태 초기화
    bIsMoving = false;
    CurrentVelocity = FVector::ZeroVector;
    
    // 델리게이트 호출
    OnDestinationReached.Broadcast();
}

bool UUKPathMovement::MoveToNextWaypoint()
{
    // 다음 웨이포인트로 인덱스 증가
    CurrentWaypointIndex++;
    
    // 모든 웨이포인트를 방문했는지 확인
    if (CurrentWaypointIndex >= CurrentPath.Num())
    {
        // 목적지에 도달
        DestinationReached();
        return false;
    }
    
    // 새 목표 위치 업데이트
    UpdateGoalLocation();
    return true;
}

void UUKPathMovement::UpdateGoalLocation()
{
    if (CurrentWaypointIndex < CurrentPath.Num())
    {
        CurrentGoalLocation = CurrentPath[CurrentWaypointIndex];
    }
}

FVector UUKPathMovement::CalculateSteeringForce(float DeltaTime)
{
    if (!GetOwner())
    {
        return FVector::ZeroVector;
    }

    // 기본 스티어링 힘 - 현재 위치에서 목표 위치로 이동하는 힘
    FVector CurrentLocation = GetOwner()->GetActorLocation();
    FVector DesiredDirection = CurrentGoalLocation - CurrentLocation;
    DesiredDirection.Z = 0.0f; // 평면 이동
    
    const float DistanceToTarget = DesiredDirection.Size();
    
    // 목표 웨이포인트에 도달했는지 확인
    float Radius = AcceptanceRadius;
    if (CurrentWaypointIndex == CurrentPath.Num() - 1)
    {
        Radius = 1.0f;
    }

    if (DistanceToTarget <= Radius)
    {
        // 다음 웨이포인트로 이동
        if (!MoveToNextWaypoint())
        {
            return FVector::ZeroVector; // 모든 웨이포인트 방문 완료
        }
        
        // 새로운 목표를 향한 방향 재계산
        DesiredDirection = CurrentGoalLocation - CurrentLocation;
        DesiredDirection.Z = 0.0f;
    }
    
    if (DistanceToTarget > 1.0f)
    {
        DesiredDirection /= DistanceToTarget; // 방향 벡터 정규화
        
        // 최대 속도로 제한된 희망 속도
        FVector DesiredVelocity = DesiredDirection * MaxSpeed;
        
        // 도착 시 감속
        if (DistanceToTarget < SlowdownRadius)
        {
            DesiredVelocity *= (DistanceToTarget / SlowdownRadius);
        }
        
        // 현재 속도 (컴포넌트에 저장된 속도 사용)
        FVector CurrentMovementVelocity = CurrentVelocity;
        CurrentMovementVelocity.Z = 0.0f;
        
        // Steering = 희망속도 - 현재속도
        float SteerStrength = 1.0f / 0.3f; // 반응 시간의 역수
        FVector NewSteeringForce = (DesiredVelocity - CurrentMovementVelocity) * SteerStrength;
        
        // 최대 힘으로 제한
        if (NewSteeringForce.SizeSquared() > MaxForce * MaxForce)
        {
            NewSteeringForce.Normalize();
            NewSteeringForce *= MaxForce;
        }

        return NewSteeringForce;
    }
    
    return FVector::ZeroVector;
}

void UUKPathMovement::ApplySteeringForce(const FVector& SteeringForce, float DeltaTime)
{
    if (!GetOwner() || SteeringForce.IsNearlyZero())
    {
        return;
    }

    // 질량에 기반한 가속도 계산
    FVector Acceleration = SteeringForce / Mass;
    
    // 새 속도 계산
    CurrentVelocity += Acceleration * DeltaTime;
    
    // 최대 속도 제한
    if (CurrentVelocity.SizeSquared() > MaxSpeed * MaxSpeed)
    {
        CurrentVelocity.Normalize();
        CurrentVelocity *= MaxSpeed;
    }
    
    // 위치 업데이트
    FVector NewLocation = GetOwner()->GetActorLocation() + CurrentVelocity * DeltaTime;
    
    // 회전 처리 (옵션)
    if (bUseRotation && !CurrentVelocity.IsNearlyZero())
    {
        // 이동 방향으로 회전
        FRotator CurrentRotation = GetOwner()->GetActorRotation();
        FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(CurrentVelocity.GetSafeNormal());
        
        // 부드러운 회전을 위한 보간
        FRotator NewRotation = UKismetMathLibrary::RInterpTo(
            CurrentRotation,
            TargetRotation,
            DeltaTime,
            RotationSpeed
        );
        
        // 회전 설정
        GetOwner()->SetActorRotation(NewRotation);
    }
    
    // 위치 설정
    GetOwner()->SetActorLocation(NewLocation, true);
    
    // 디버그 로그
    //UE_LOG(LogTemp, Log, TEXT("Velocity: %s (%f)"), *CurrentVelocity.ToString(), CurrentVelocity.Size());
}