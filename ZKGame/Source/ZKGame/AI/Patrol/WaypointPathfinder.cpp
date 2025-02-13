// WaypointPathfinder.cpp
#include "WaypointPathfinder.h"

AZKWaypoint* FWaypointPathfinder::FindNearestWaypoint(const TArray<AActor*>& Waypoints, const FVector& Location)
{
    AZKWaypoint* NearestWaypoint = nullptr;
    float MinDistance = MAX_FLT;

    for (int32 i=0; i<Waypoints.Num(); i++)
    {
        if (!Waypoints[i]) continue;
        
        float Distance = FVector::DistSquared(Location, Waypoints[i]->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestWaypoint = Cast<AZKWaypoint>(Waypoints[i]);
        }
    }
    
    return NearestWaypoint;
}

void FWaypointPathfinder::DrawDebugPath(const UWorld* World, const TArray<AZKWaypoint*>& Path, const FDebugSettings& DebugSettings)
{
    if (!World || Path.Num() < 2) return;

    // 경로 그리기
    for (int32 i = 0; i < Path.Num() - 1; ++i)
    {
        if (!Path[i] || !Path[i + 1]) continue;

        FVector Start = Path[i]->GetActorLocation();
        FVector End = Path[i + 1]->GetActorLocation();
        
        // 화살표로 경로 표시
        DrawDebugDirectionalArrow(
            World,
            Start,
            End,
            DebugSettings.ArrowSize,
            DebugSettings.PathColor,
            false,
            DebugSettings.DrawDuration,
            0,
            DebugSettings.PathThickness
        );

        // 거리 표시
        if (DebugSettings.bShowDistances)
        {
            float Distance = FVector::Dist(Start, End);
            FVector MidPoint = (Start + End) * 0.5f;
            FString DistanceStr = FString::Printf(TEXT("%.1f"), Distance);
            
            DrawDebugString(
                World,
                MidPoint,
                DistanceStr,
                nullptr,
                DebugSettings.TextColor,
                DebugSettings.DrawDuration,
                false,
                DebugSettings.TextScale
            );
        }
    }

    // 각 웨이포인트 위치에 구체 표시
    for (AZKWaypoint* Waypoint : Path)
    {
        if (!Waypoint) continue;
        
        DrawDebugSphere(
            World,
            Waypoint->GetActorLocation(),
            30.0f,
            16,
            DebugSettings.PathColor,
            false,
            DebugSettings.DrawDuration,
            0,
            0.5f
        );
    }
}

void FWaypointPathfinder::DrawDebugInteractionToPath(const UWorld* World, const FVector& InteractionPoint, const TArray<AZKWaypoint*>& Path, const FDebugSettings& DebugSettings)
{
    if (!World || Path.Num() == 0) return;

    // 상호작용 지점 표시
    DrawDebugSphere(
        World,
        InteractionPoint,
        30.0f,
        16,
        FColor::Blue,
        false,
        DebugSettings.DrawDuration,
        0,
        0.5f
    );

    // 상호작용 지점에서 첫 번째 웨이포인트까지 선 그리기
    if (Path[0])
    {
        DrawDebugDirectionalArrow(
            World,
            InteractionPoint,
            Path[0]->GetActorLocation(),
            DebugSettings.ArrowSize,
            FColor::Blue,
            false,
            DebugSettings.DrawDuration,
            0,
            DebugSettings.PathThickness
        );

        if (DebugSettings.bShowDistances)
        {
            float Distance = FVector::Dist(InteractionPoint, Path[0]->GetActorLocation());
            FVector MidPoint = (InteractionPoint + Path[0]->GetActorLocation()) * 0.5f;
            FString DistanceStr = FString::Printf(TEXT("%.1f"), Distance);
            
            DrawDebugString(
                World,
                MidPoint,
                DistanceStr,
                nullptr,
                DebugSettings.TextColor,
                DebugSettings.DrawDuration,
                false,
                DebugSettings.TextScale
            );
        }
    }

    // 나머지 경로 그리기
    DrawDebugPath(World, Path, DebugSettings);
}

TArray<AZKWaypoint*> FWaypointPathfinder::FindShortestPath(AZKWaypoint* StartWaypoint, AZKWaypoint* EndWaypoint)
{
    TArray<AZKWaypoint*> Path;
    if (!StartWaypoint || !EndWaypoint) return Path;
    
    // 방문하지 않은 노드 맵 초기화
    TMap<AZKWaypoint*, FPathNode> Nodes;
    TArray<AZKWaypoint*> UnvisitedNodes;
    
    // 시작 노드 설정
    Nodes.Add(StartWaypoint, FPathNode(StartWaypoint));
    Nodes[StartWaypoint].Distance = 0;
    UnvisitedNodes.Add(StartWaypoint);
    
    while (UnvisitedNodes.Num() > 0)
    {
        // 가장 가까운 미방문 노드 찾기
        AZKWaypoint* CurrentWaypoint = nullptr;
        float MinDistance = MAX_FLT;
        
        for (AZKWaypoint* Waypoint : UnvisitedNodes)
        {
            if (Nodes[Waypoint].Distance < MinDistance)
            {
                MinDistance = Nodes[Waypoint].Distance;
                CurrentWaypoint = Waypoint;
            }
        }
        
        if (!CurrentWaypoint) break;
        
        // 현재 노드가 목적지인 경우
        if (CurrentWaypoint == EndWaypoint)
        {
            // 경로 역추적
            AZKWaypoint* PathWaypoint = EndWaypoint;
            while (PathWaypoint)
            {
                Path.Insert(PathWaypoint, 0);
                PathWaypoint = Nodes[PathWaypoint].PreviousWaypoint;
            }
            break;
        }
        
        // 현재 노드 방문 처리
        UnvisitedNodes.Remove(CurrentWaypoint);
        
        // 인접 노드 처리 (Next 및 Previous 포인트 모두 고려)
        TArray<AZKWaypoint*> AdjacentWaypoints;
        AdjacentWaypoints.Append(CurrentWaypoint->NextPathPoints);
        AdjacentWaypoints.Append(CurrentWaypoint->PreviousPathPoints);
        
        for (AZKWaypoint* AdjacentWaypoint : AdjacentWaypoints)
        {
            if (!AdjacentWaypoint) continue;
            
            // 노드 맵에 없으면 추가
            if (!Nodes.Contains(AdjacentWaypoint))
            {
                Nodes.Add(AdjacentWaypoint, FPathNode(AdjacentWaypoint));
                UnvisitedNodes.Add(AdjacentWaypoint);
            }
            
            // 거리 계산 및 업데이트
            float NewDistance = Nodes[CurrentWaypoint].Distance + 
                FVector::Dist(CurrentWaypoint->GetActorLocation(), AdjacentWaypoint->GetActorLocation());
            
            if (NewDistance < Nodes[AdjacentWaypoint].Distance)
            {
                Nodes[AdjacentWaypoint].Distance = NewDistance;
                Nodes[AdjacentWaypoint].PreviousWaypoint = CurrentWaypoint;
            }
        }
    }
    
    return Path;
}

TArray<AZKWaypoint*> FWaypointPathfinder::FindPathFromInteractionPoint(
    const FVector& InteractionPoint,
    const TArray<AActor*>& AllWaypoints,
    AZKWaypoint* DestinationWaypoint)
{
    // 1. 상호작용 위치에서 가장 가까운 웨이포인트 찾기
    AZKWaypoint* NearestWaypoint = FindNearestWaypoint(AllWaypoints, InteractionPoint);
    if (!NearestWaypoint || !DestinationWaypoint) return TArray<AZKWaypoint*>();
    
    // 2. 가장 가까운 웨이포인트에서 목적지까지의 최단 경로 찾기
    return FindShortestPath(NearestWaypoint, DestinationWaypoint);
}