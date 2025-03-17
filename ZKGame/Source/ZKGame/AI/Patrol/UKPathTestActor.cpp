#include "UKPathTestActor.h"

#include "DrawDebugHelpers.h"
#include "UKPathFindingSubsystem.h"
#include "UKWaypoint.h"

AUKPathTestActor::AUKPathTestActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUKPathTestActor::BeginPlay()
{
    Super::BeginPlay();
}

void AUKPathTestActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 에디터에서만 실행
#if WITH_EDITOR
    UpdatePathVisualization();
#endif
}

void AUKPathTestActor::UpdatePathVisualization()
{
    UWorld* World = GetWorld();
    if (!UUKPathFindingSubsystem::Get(World))
    {
        return;
    }
    
    if (!EndActor)
    {
        return;
    }

    // 서브시스템 참조 가져오기
    UUKPathFindingSubsystem* PathFindingSubsystem = GetWorld()->GetSubsystem<UUKPathFindingSubsystem>();
    PathFindingSubsystem->AllRegisterWaypoint();

    // 시작점과 끝점에서 가장 가까운 웨이포인트 찾기
    AUKWaypoint* StartWaypoint = PathFindingSubsystem->FindNearestWaypoint(GetActorLocation());
    AUKWaypoint* EndWaypoint = PathFindingSubsystem->FindNearestWaypoint(EndActor->GetActorLocation());

    if (!StartWaypoint || !EndWaypoint)
    {
        return;
    }

    // 시작점과 끝점 표시
    DrawDebugSphere(
        GetWorld(),
        GetActorLocation(),
        DebugSphereRadius,
        16,
        DebugColor,
        false,
        -1.0f,
        0,
        DebugThickness
    );

    DrawDebugSphere(
        GetWorld(),
        EndActor->GetActorLocation(),
        DebugSphereRadius,
        16,
        DebugColor,
        false,
        -1.0f,
        0,
        DebugThickness
    );

    // 경로 찾기
    TArray<AUKWaypoint*> Path = PathFindingSubsystem->FindPath(StartWaypoint, EndWaypoint);
    
    // 시작점부터 첫 웨이포인트까지
    if (Path.Num() > 0)
    {
        DrawDebugDirectionalArrow(
            GetWorld(),
            GetActorLocation(),
            Path[0]->GetActorLocation(),
            DebugArrowSize,
            DebugColor,
            false,
            -1.0f,
            0,
            DebugThickness
        );
    }

    // 웨이포인트 간 경로 표시
    for (int32 i = 0; i < Path.Num() - 1; i++)
    {
        DrawDebugDirectionalArrow(
            GetWorld(),
            Path[i]->GetActorLocation(),
            Path[i + 1]->GetActorLocation(),
            DebugArrowSize,
            DebugColor,
            false,
            -1.0f,
            0,
            DebugThickness
        );
    }

    // 마지막 웨이포인트에서 목적지까지
    if (Path.Num() > 0)
    {
        DrawDebugDirectionalArrow(
            GetWorld(),
            Path.Last()->GetActorLocation(),
            EndActor->GetActorLocation(),
            DebugArrowSize,
            DebugColor,
            false,
            -1.0f,
            0,
            DebugThickness
        );
    }
}