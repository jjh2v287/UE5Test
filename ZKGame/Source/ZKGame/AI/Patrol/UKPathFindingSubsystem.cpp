#include "UKPathFindingSubsystem.h"
#include "Kismet/GameplayStatics.h"

// 정적 멤버 변수 초기화
UUKPathFindingSubsystem* UUKPathFindingSubsystem::Instance = nullptr;

UUKPathFindingSubsystem* UUKPathFindingSubsystem::Get(const UWorld* World)
{
    if (!IsValid(World))
    {
        return nullptr;
    }
    
    if (!Instance)
    {
        Instance = World->GetSubsystem<UUKPathFindingSubsystem>();
    }
    
    return Instance;
}

UUKPathFindingSubsystem* UUKPathFindingSubsystem::Get()
{
    if (!Instance)
    {
        // GEngine이 유효한 경우에만 GameWorld를 가져옴
        if (GEngine)
        {
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
                {
                    if (UWorld* World = Context.World())
                    {
                        Instance = World->GetSubsystem<UUKPathFindingSubsystem>();
                        break;
                    }
                }
            }
        }
    }
    
    return Instance;
}

void UUKPathFindingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 싱글톤 인스턴스 설정
    if (Instance)
    {
        UE_LOG(LogTemp, Warning, TEXT("UKPathFindingSubsystem: 이미 인스턴스가 존재합니다!"));
    }
    Instance = this;
}

void UUKPathFindingSubsystem::Deinitialize()
{
    // 싱글톤 인스턴스 정리
    if (Instance == this)
    {
        Instance = nullptr;
    }
    
    ClearWaypoints();
    Super::Deinitialize();
}

void UUKPathFindingSubsystem::RegisterWaypoint(AUKWaypoint* Waypoint)
{
    if (IsValid(Waypoint) && !WaypointRegistry.Contains(Waypoint))
    {
        WaypointRegistry.Add(Waypoint);
        RebuildGraph();
    }
}

void UUKPathFindingSubsystem::UnregisterWaypoint(AUKWaypoint* Waypoint)
{
    if (WaypointRegistry.Remove(Waypoint) > 0)
    {
        RebuildGraph();
    }
}

void UUKPathFindingSubsystem::ClearWaypoints()
{
    WaypointRegistry.Empty();
    RebuildGraph();
}

void UUKPathFindingSubsystem::AllRegisterWaypoint()
{
    // 월드의 모든 웨이포인트 수집
    WaypointRegistry.Empty();
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

void UUKPathFindingSubsystem::RebuildGraph()
{
    PathGraph.Waypoints = WaypointRegistry;
}

AUKWaypoint* UUKPathFindingSubsystem::FindNearestWaypoint(const FVector& Location) const
{
    if (WaypointRegistry.IsEmpty())
    {
        return nullptr;
    }

    AUKWaypoint* NearestWaypoint = nullptr;
    float NearestDistanceSq = TNumericLimits<float>::Max();

    for (AUKWaypoint* Waypoint : WaypointRegistry)
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

TArray<AUKWaypoint*> UUKPathFindingSubsystem::FindPath(AUKWaypoint* Start, AUKWaypoint* End)
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