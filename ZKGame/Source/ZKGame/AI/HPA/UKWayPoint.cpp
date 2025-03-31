// Copyright Kong Studios, Inc. All Rights Reserved.

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
           WayPointHandle = HPAManager->RegisterWaypoint(this);
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
}

#if WITH_EDITOR
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
        }
    }
}
#endif