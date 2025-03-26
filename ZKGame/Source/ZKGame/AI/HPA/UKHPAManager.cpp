// Fill out your copyright notice in the Description page of Project Settings.


#include "UKHPAManager.h"

#include "UKWayPoint.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKHPAManager)

// 정적 멤버 변수 초기화
UUKHPAManager* UUKHPAManager::Instance = nullptr;

void UUKHPAManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
}

void UUKHPAManager::Deinitialize()
{
	PathGraph.WayPoints.Empty();
	Instance = nullptr;
	Super::Deinitialize();
}

void UUKHPAManager::RegisterWaypoint(AUKWayPoint* Waypoint)
{
	if (IsValid(Waypoint) && !PathGraph.WayPoints.Contains(Waypoint))
	{
		PathGraph.WayPoints.Add(Waypoint);
	}
}

void UUKHPAManager::UnregisterWaypoint(AUKWayPoint* Waypoint)
{
	if (IsValid(Waypoint) && PathGraph.WayPoints.Contains(Waypoint))
	{
		PathGraph.WayPoints.Remove(Waypoint);
	}
}

void UUKHPAManager::AllRegisterWaypoint()
{
    // 월드의 모든 웨이포인트 수집
    PathGraph.WayPoints.Empty();
    TArray<AActor*> FoundWaypoints;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUKWayPoint::StaticClass(), FoundWaypoints);
    
    for (AActor* Actor : FoundWaypoints)
    {
        if (AUKWayPoint* Waypoint = Cast<AUKWayPoint>(Actor))
        {
            RegisterWaypoint(Waypoint);
        }
    }
}

AUKWayPoint* UUKHPAManager::FindNearestWaypoint(const FVector& Location) const
{
    if (PathGraph.WayPoints.IsEmpty())
    {
        return nullptr;
    }

    AUKWayPoint* NearestWayPoint = nullptr;
    float NearestDistanceSq = TNumericLimits<float>::Max();

    for (AUKWayPoint* WayPoint : PathGraph.WayPoints)
    {
        if (!IsValid(WayPoint))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared(Location, WayPoint->GetActorLocation());
        if (DistanceSq < NearestDistanceSq)
        {
            NearestDistanceSq = DistanceSq;
            NearestWayPoint = WayPoint;
        }
    }

    return NearestWayPoint;
}

TArray<AUKWayPoint*> UUKHPAManager::FindPath(AUKWayPoint* Start, AUKWayPoint* End)
{
    if (!IsValid(Start) || !IsValid(End))
    {
        return TArray<AUKWayPoint*>();
    }

    // 그래프에서 시작과 끝 웨이포인트의 인덱스 찾기
    int32 StartIndex = PathGraph.WayPoints.IndexOfByKey(Start);
    int32 EndIndex = PathGraph.WayPoints.IndexOfByKey(End);
    
    if (StartIndex == INDEX_NONE || EndIndex == INDEX_NONE)
    {
        return TArray<AUKWayPoint*>();
    }

    // A* 노드 생성
    FGraphAStarDefaultNode<FWayPointGraph> StartNode(StartIndex);
    FGraphAStarDefaultNode<FWayPointGraph> EndNode(EndIndex);

    // 필터 설정
    FWayPointFilter QueryFilter;
    QueryFilter.Graph = &PathGraph;

    // A* 알고리즘 실행
    FGraphAStar<FWayPointGraph> Pathfinder(PathGraph);
    TArray<int32> OutPath;
    
    EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, QueryFilter, OutPath);

    if (Result == SearchSuccess)
    {
        TArray<AUKWayPoint*> WaypointPath = {Start};
        for (int32 Index : OutPath)
        {
            if (PathGraph.WayPoints.IsValidIndex(Index))
            {
                WaypointPath.Add(PathGraph.WayPoints[Index]);
            }
        }
        return WaypointPath;
    }

    return TArray<AUKWayPoint*>();
}