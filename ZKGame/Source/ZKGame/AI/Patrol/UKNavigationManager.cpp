#include "UKNavigationManager.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKNavigationManager)

// 정적 멤버 변수 초기화
UUKNavigationManager* UUKNavigationManager::Instance = nullptr;

void UUKNavigationManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Instance = this;
}

void UUKNavigationManager::Deinitialize()
{
    PathGraph.Waypoints.Empty();
    Instance = nullptr;
    Super::Deinitialize();
}

void UUKNavigationManager::RegisterWaypoint(AUKWaypoint* Waypoint)
{
    if (IsValid(Waypoint) && !PathGraph.Waypoints.Contains(Waypoint))
    {
        PathGraph.Waypoints.Add(Waypoint);
    }
}

void UUKNavigationManager::UnregisterWaypoint(AUKWaypoint* Waypoint)
{
    if (IsValid(Waypoint) && PathGraph.Waypoints.Contains(Waypoint))
    {
        PathGraph.Waypoints.Remove(Waypoint);
    }
}

void UUKNavigationManager::AllRegisterWaypoint()
{
    // 월드의 모든 웨이포인트 수집
    PathGraph.Waypoints.Empty();
    TArray<AActor*> FoundWaypoints;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUKWaypoint::StaticClass(), FoundWaypoints);
    
    for (AActor* Actor : FoundWaypoints)
    {
        if (AUKWaypoint* Waypoint = Cast<AUKWaypoint>(Actor))
        {
            RegisterWaypoint(Waypoint);
        }
    }
}

AUKWaypoint* UUKNavigationManager::FindNearestWaypoint(const FVector& Location) const
{
    if (PathGraph.Waypoints.IsEmpty())
    {
        return nullptr;
    }

    AUKWaypoint* NearestWaypoint = nullptr;
    float NearestDistanceSq = TNumericLimits<float>::Max();

    for (AUKWaypoint* Waypoint : PathGraph.Waypoints)
    {
        if (!IsValid(Waypoint))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared(Location, Waypoint->GetActorLocation());
        if (DistanceSq < NearestDistanceSq)
        {
            NearestDistanceSq = DistanceSq;
            NearestWaypoint = Waypoint;
        }
    }

    return NearestWaypoint;
}

TArray<AUKWaypoint*> UUKNavigationManager::FindPath(AUKWaypoint* Start, AUKWaypoint* End)
{
    if (!IsValid(Start) || !IsValid(End))
    {
        return TArray<AUKWaypoint*>();
    }

    // 그래프에서 시작과 끝 웨이포인트의 인덱스 찾기
    int32 StartIndex = PathGraph.Waypoints.IndexOfByKey(Start);
    int32 EndIndex = PathGraph.Waypoints.IndexOfByKey(End);
    
    if (StartIndex == INDEX_NONE || EndIndex == INDEX_NONE)
    {
        return TArray<AUKWaypoint*>();
    }

    // A* 노드 생성
    FGraphAStarDefaultNode<FWaypointGraph> StartNode(StartIndex);
    FGraphAStarDefaultNode<FWaypointGraph> EndNode(EndIndex);

    // 필터 설정
    FWaypointFilter QueryFilter;
    QueryFilter.Graph = &PathGraph;

    // A* 알고리즘 실행
    FGraphAStar<FWaypointGraph> Pathfinder(PathGraph);
    TArray<int32> OutPath;
    
    EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, QueryFilter, OutPath);

    if (Result == SearchSuccess)
    {
        TArray<AUKWaypoint*> WaypointPath = {Start};
        for (int32 Index : OutPath)
        {
            if (PathGraph.Waypoints.IsValidIndex(Index))
            {
                WaypointPath.Add(PathGraph.Waypoints[Index]);
            }
        }
        return WaypointPath;
    }

    return TArray<AUKWaypoint*>();
}