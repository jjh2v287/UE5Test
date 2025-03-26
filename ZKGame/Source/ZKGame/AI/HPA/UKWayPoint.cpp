#include "UKWayPoint.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "NavigationInvokerComponent.h"
#include "UKHPAManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKWayPoint)

AUKWayPoint::AUKWayPoint(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    InvokerComponent = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));
}

void AUKWayPoint::BeginPlay()
{
    Super::BeginPlay();
    // 월드가 유효하고 게임 월드인지 확인 (에디터 월드 제외 등)
    UWorld* World = GetWorld();
    if (World && World->IsGameWorld())
    {
        // HPA 매니저 서브시스템 가져오기
        if (UUKHPAManager* HPAManager = World->GetSubsystem<UUKHPAManager>())
        {
            HPAManager->RegisterWaypoint(this);
        }
    }
}

void AUKWayPoint::Destroyed()
{
    // 월드가 유효하고 게임 월드인지 확인
    UWorld* World = GetWorld();
     // 종료 시점에는 서브시스템이 먼저 해제될 수 있으므로 IsValid 확인
    if (World && World->IsGameWorld() && UUKHPAManager::Get()) // Get()으로 싱글톤 유효성 확인
    {
        if (UUKHPAManager* HPAManager = World->GetSubsystem<UUKHPAManager>())
        {
            HPAManager->UnregisterWaypoint(this);
        }
    }
    Super::Destroyed();
}

void AUKWayPoint::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if WITH_EDITOR
    if (GetWorld() && !GetWorld()->IsGameWorld()) // 에디터 월드에서만 Tick에서 그리기
    {
        DrawDebugLines();
    }
#endif
}

#if WITH_EDITOR
void AUKWayPoint::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);
    // HPA 매니저에게 변경 알림 (선택적: 에디터 변경 시 계층 구조 자동 업데이트)
    // if (bFinished && UUKHPAManager::Get()) { UUKHPAManager::Get()->NotifyWayPointChanged(this); }

    // 기존 로직 유지 (스플라인 업데이트 등)
    if(GetWorld())
    {
        for (AUKWayPoint* OtherPoint : TActorRange<AUKWayPoint>(GetWorld()))
        {
            if (OtherPoint && OtherPoint != this && OtherPoint->PathPoints.Contains(this))
            {
                // OtherPoint->UpdateConnectedSplines();
            }
        }
    }
}

void AUKWayPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // PathPoints 변경 시 양방향 연결 자동 추가 로직
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AUKWayPoint, PathPoints))
        {
            for (AUKWayPoint* NextPoint : PathPoints)
            {
                if (NextPoint && !NextPoint->PathPoints.Contains(this))
                {
                    // 직접 수정 시 무한 루프 방지 필요할 수 있음 (Modify 사용 등)
                    NextPoint->Modify(); // 변경 기록
                    NextPoint->PathPoints.Add(this);
                    // 에디터에서 변경 사항 즉시 반영 알림
                    NextPoint->PostEditChange();
                }
            }
            // HPA 매니저에게 변경 알림 (선택적)
            // if (UUKHPAManager::Get()) { UUKHPAManager::Get()->NotifyWayPointChanged(this); }
        }
        // ClusterID 변경 시 알림 (선택적)
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(AUKWayPoint, ClusterID))
        {
             // if (UUKHPAManager::Get()) { UUKHPAManager::Get()->NotifyWayPointChanged(this); }
        }
    }
}

void AUKWayPoint::DrawDebugLines()
{
    const FVector StartLocation = GetActorLocation();
    const UWorld* World = GetWorld();
    if (!World) return;

    // 연결된 경로 표시 (기존 보라색 화살표)
    for (const AUKWayPoint* Point : PathPoints)
    {
        if (Point)
        {
            // 양방향 연결 시 겹치지 않게 오프셋 적용
            bool bIsMutual = Point->PathPoints.Contains(this);
            FVector Direction = (Point->GetActorLocation() - StartLocation).GetSafeNormal();
            FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
            float LineOffset = bIsMutual ? 10.0f : 0.0f; // 양방향일 때만 오프셋

            FVector OffsetStart = StartLocation + RightVector * LineOffset;
            FVector OffsetEnd = Point->GetActorLocation() + RightVector * LineOffset;

            DrawDebugDirectionalArrow(
                World,
                OffsetStart,
                OffsetEnd,
                50.0f, // 크기 줄임
                FColor::Purple,
                false,
                -1.0f, // 지속 시간 (Tick에서 호출되므로 0 또는 -1)
                0, // Depth Priority
                1.5f // 두께
            );
        }
    }
}
#endif