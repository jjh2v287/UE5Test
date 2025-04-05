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

    // 이동 중이라면 웨이포인트를 향해 이동
    if (bIsMoving)
    {
        MoveTowardsWaypoint(DeltaTime);
    }
}

void UUKPathMovement::MoveToLocation(const FVector TargetLocation)
{
    // 네비게이션 매니저 존재 확인
    if (!UUKNavigationManager::Get())
    {
        UE_LOG(LogTemp, Warning, TEXT("UKPathMovement: Navigation Manager not found!"));
        return;
    }

    // 시작 위치 가져오기
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
    bIsMoving = true;

    UE_LOG(LogTemp, Log, TEXT("UKPathMovement: Starting movement with %d waypoints"), CurrentPath.Num());
}

void UUKPathMovement::StopMovement()
{
    bIsMoving = false;
    CurrentPath.Empty();
    CurrentWaypointIndex = 0;
}

void UUKPathMovement::DestinationReached()
{
    UE_LOG(LogTemp, Log, TEXT("UKPathMovement: Destination reached"));
    
    // 이동 상태 초기화
    bIsMoving = false;
    
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
    
    return true;
}

void UUKPathMovement::MoveTowardsWaypoint(float DeltaTime)
{
    if (!GetOwner() || CurrentWaypointIndex >= CurrentPath.Num() || !bIsMoving)
    {
        return;
    }

    // 현재 액터 위치 및 현재 목표 웨이포인트 위치
    FVector CurrentLocation = GetOwner()->GetActorLocation();
    FVector TargetWaypoint = CurrentPath[CurrentWaypointIndex];
    
    // Z축 높이 맞추기 (필요에 따라 조정)
    TargetWaypoint.Z = CurrentLocation.Z;
    
    // 현재 위치에서 목표 웨이포인트까지의 방향 및 거리 계산
    FVector Direction = TargetWaypoint - CurrentLocation;
    float DistanceSquared = Direction.SizeSquared();
    
    // 목표 웨이포인트에 도달했는지 확인
    if (DistanceSquared <= FMath::Square(AcceptanceRadius))
    {
        // 다음 웨이포인트로 이동
        if (!MoveToNextWaypoint())
        {
            return; // 모든 웨이포인트 방문 완료
        }
        
        // 다음 웨이포인트에 대한 정보 업데이트
        TargetWaypoint = CurrentPath[CurrentWaypointIndex];
        TargetWaypoint.Z = CurrentLocation.Z;
        Direction = TargetWaypoint - CurrentLocation;
    }
    
    // 이동 벡터 정규화 및 속도 적용
    Direction.Normalize();
    FVector MoveDelta = Direction * MovementSpeed * DeltaTime;
    
    // 회전 처리 (옵션)
    if (bUseRotation)
    {
        // 목표 방향으로 회전
        FRotator CurrentRotation = GetOwner()->GetActorRotation();
        FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(Direction);
        
        // 부드러운 회전을 위한 보간
        FRotator NewRotation = UKismetMathLibrary::RInterpTo(
            CurrentRotation,
            TargetRotation,
            DeltaTime,
            RotationSpeed
        );
        
        GetOwner()->SetActorRotation(NewRotation);
    }
    
    // 액터 위치 업데이트
    GetOwner()->SetActorLocation(CurrentLocation + MoveDelta, true);
}