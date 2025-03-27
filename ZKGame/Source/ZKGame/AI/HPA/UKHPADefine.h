// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "UKWayPoint.h"

struct ZKGAME_API FHPAEntrance
{
    TWeakObjectPtr<AUKWayPoint> LocalWaypoint = nullptr;
    TWeakObjectPtr<AUKWayPoint> NeighborWaypoint = nullptr;
    int32 NeighborClusterID = INDEX_NONE;
    float Cost = 0.0f;

    FHPAEntrance() = default;
    FHPAEntrance(AUKWayPoint* InLocal, AUKWayPoint* InNeighbor, int32 InNeighborClusterID, float InCost)
        : LocalWaypoint(InLocal), NeighborWaypoint(InNeighbor), NeighborClusterID(InNeighborClusterID), Cost(InCost)
    {}
};

struct ZKGAME_API FHPACluster
{
    int32 ClusterID = INDEX_NONE;
    
    TArray<TWeakObjectPtr<AUKWayPoint>> Waypoints;
    
    // Key: NeighborClusterID
    TMap<int32, TArray<FHPAEntrance>> Entrances;
    
    FVector CenterLocation = FVector::ZeroVector;

    FHPACluster() = default;
    explicit FHPACluster(int32 InID) : ClusterID(InID) {}

    void CalculateCenter()
    {
        if (Waypoints.IsEmpty())
        {
            CenterLocation = FVector::ZeroVector; return;
        }
        
        FVector SumPos = FVector::ZeroVector;
        int32 ValidCount = 0;
        for (const auto& WeakWP : Waypoints)
        {
            if (AUKWayPoint* WP = WeakWP.Get())
            {
                SumPos += WP->GetActorLocation();
                ValidCount++;
            }
        }
        CenterLocation = (ValidCount > 0) ? SumPos / ValidCount : FVector::ZeroVector;
    }
};

struct ZKGAME_API FHPAAbstractGraph
{
    // Key: ClusterID
    TMap<int32, FHPACluster> Clusters;
    bool bIsBuilt = false;

    void Clear() {
        Clusters.Empty();
        bIsBuilt = false;
    }
};

// --- HPA용 A* 관련 구조체 ---
// FWayPointAStarGraph: 클러스터 내부 탐색용
struct FWayPointAStarGraph
{
    // 전체 웨이포인트 배열의 인덱스 사용
    typedef int32 FNodeRef;

    // 전체 웨이포인트 배열 참조
    const TArray<TWeakObjectPtr<AUKWayPoint>>* AllWaypointsPtr = nullptr;

    // 포인터 -> 인덱스 맵 참조
    const TMap<AUKWayPoint*, int32>* WaypointToIndexMapPtr = nullptr;

    // 탐색 대상 클러스터 ID
    int32 CurrentClusterID = INDEX_NONE;

    FWayPointAStarGraph(const TArray<TWeakObjectPtr<AUKWayPoint>>& InAllWaypoints, const TMap<AUKWayPoint*, int32>& InIndexMap, int32 InClusterID)
        : AllWaypointsPtr(&InAllWaypoints), WaypointToIndexMapPtr(&InIndexMap), CurrentClusterID(InClusterID) {}

    // 노드(웨이포인트) 인덱스의 유효성 검사
    bool IsValidRef(FNodeRef NodeRef) const
    {
        return AllWaypointsPtr && AllWaypointsPtr->IsValidIndex(NodeRef);
    }

    // 특정 노드(웨이포인트) 인덱스에 해당하는 웨이포인트 객체 반환
    AUKWayPoint* GetWaypoint(FNodeRef NodeRef) const
    {
        return IsValidRef(NodeRef) ? (*AllWaypointsPtr)[NodeRef].Get() : nullptr;
    }

    // 특정 노드(웨이포인트) 인덱스에서 접근 가능한 이웃 노드 수 반환 (같은 클러스터 내)
    int32 GetNeighbourCount(FNodeRef NodeRef) const;

    // 특정 노드(웨이포인트) 인덱스의 NeighbourIndex번째 이웃 노드 인덱스 반환 (같은 클러스터 내)
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const;
};

// FWayPointFilter: 클러스터 내부 탐색 필터
struct FWayPointFilter
{
    const FWayPointAStarGraph& Graph;

    FWayPointFilter(const FWayPointAStarGraph& InGraph) : Graph(InGraph) {}

    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    FVector::FReal GetHeuristicCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const;
    FVector::FReal GetTraversalCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const;
    bool IsTraversalAllowed(FGraphAStarDefaultNode<FWayPointAStarGraph> NodeA, FGraphAStarDefaultNode<FWayPointAStarGraph> NodeB) const;
    bool WantsPartialSolution() const { return false; }
};


// FClusterAStarGraph: 추상 클러스터 그래프 탐색용
struct FClusterAStarGraph
{
    // ClusterID를 노드 참조로 사용
    typedef int32 FNodeRef;

    // 추상 그래프 데이터 참조
    const FHPAAbstractGraph* AbstractGraphPtr = nullptr;

    FClusterAStarGraph(const FHPAAbstractGraph* InGraph) : AbstractGraphPtr(InGraph) {}

    bool IsValidRef(FNodeRef NodeRef) const;
    int32 GetNeighbourCount(FNodeRef NodeRef) const;
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const;
};

// FClusterFilter: 추상 클러스터 그래프 탐색 필터
struct FClusterFilter
{
    const FClusterAStarGraph& GraphRef;

    // 비용 계산 등을 위해 원본 데이터 접근
    const FHPAAbstractGraph* AbstractGraphPtr;

    FClusterFilter(const FClusterAStarGraph& InGraphRef, const FHPAAbstractGraph* InAbstractGraph)
        : GraphRef(InGraphRef), AbstractGraphPtr(InAbstractGraph) {}

    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    FVector::FReal GetHeuristicCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const;
    FVector::FReal GetTraversalCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const;
    bool IsTraversalAllowed(FGraphAStarDefaultNode<FClusterAStarGraph> NodeA, FGraphAStarDefaultNode<FClusterAStarGraph> NodeB) const;
    bool WantsPartialSolution() const { return false; }
};