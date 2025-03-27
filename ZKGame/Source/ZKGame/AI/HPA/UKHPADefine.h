// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "UKWayPoint.h"

/** @struct FHPAEntrance */
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

/** @struct FHPACluster */
struct ZKGAME_API FHPACluster
{
    int32 ClusterID = INDEX_NONE;
    TArray<TWeakObjectPtr<AUKWayPoint>> Waypoints;
    TMap<int32, TArray<FHPAEntrance>> Entrances; // Key: NeighborClusterID
    FVector CenterLocation = FVector::ZeroVector;

    FHPACluster() = default;
    explicit FHPACluster(int32 InID) : ClusterID(InID) {}

    void CalculateCenter()
    {
        if (Waypoints.IsEmpty()) { CenterLocation = FVector::ZeroVector; return; }
        FVector SumPos = FVector::ZeroVector;
        int32 ValidCount = 0;
        for (const auto& WeakWP : Waypoints) {
            if (AUKWayPoint* WP = WeakWP.Get()) {
                SumPos += WP->GetActorLocation();
                ValidCount++;
            }
        }
        CenterLocation = (ValidCount > 0) ? SumPos / ValidCount : FVector::ZeroVector;
    }
};

/** @struct FHPAAbstractGraph */
struct ZKGAME_API FHPAAbstractGraph
{
    TMap<int32, FHPACluster> Clusters; // Key: ClusterID
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
    typedef int32 FNodeRef; // 전체 웨이포인트 배열의 인덱스 사용

    const TArray<TWeakObjectPtr<AUKWayPoint>>* AllWaypointsPtr = nullptr; // 전체 웨이포인트 배열 참조
    const TMap<AUKWayPoint*, int32>* WaypointToIndexMapPtr = nullptr; // 포인터 -> 인덱스 맵 참조
    int32 CurrentClusterID = INDEX_NONE; // 탐색 대상 클러스터 ID

    FWayPointAStarGraph(const TArray<TWeakObjectPtr<AUKWayPoint>>& InAllWaypoints,
                                const TMap<AUKWayPoint*, int32>& InIndexMap,
                                int32 InClusterID)
        : AllWaypointsPtr(&InAllWaypoints), WaypointToIndexMapPtr(&InIndexMap), CurrentClusterID(InClusterID) {}

    // 노드(웨이포인트) 인덱스의 유효성 검사
    bool IsValidRef(FNodeRef NodeRef) const {
        return AllWaypointsPtr && AllWaypointsPtr->IsValidIndex(NodeRef);
    }

    // 특정 노드(웨이포인트) 인덱스에 해당하는 웨이포인트 객체 반환
    AUKWayPoint* GetWaypoint(FNodeRef NodeRef) const {
        return IsValidRef(NodeRef) ? (*AllWaypointsPtr)[NodeRef].Get() : nullptr;
    }

    // 특정 노드(웨이포인트) 인덱스에서 접근 가능한 이웃 노드 수 반환 (같은 클러스터 내)
    int32 GetNeighbourCount(FNodeRef NodeRef) const; // .cpp에서 구현

    // 특정 노드(웨이포인트) 인덱스의 NeighbourIndex번째 이웃 노드 인덱스 반환 (같은 클러스터 내)
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const; // .cpp에서 구현
};

// FWayPointFilter_HPA_Internal: 클러스터 내부 탐색 필터
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


// FClusterGraph_HPA: 추상 클러스터 그래프 탐색용
struct FClusterAStarGraph
{
    typedef int32 FNodeRef; // ClusterID를 노드 참조로 사용
    const FHPAAbstractGraph* AbstractGraphPtr = nullptr; // 추상 그래프 데이터 참조

    FClusterAStarGraph(const FHPAAbstractGraph* InGraph) : AbstractGraphPtr(InGraph) {}

    bool IsValidRef(FNodeRef NodeRef) const; // .cpp 구현
    int32 GetNeighbourCount(FNodeRef NodeRef) const; // .cpp 구현
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const; // .cpp 구현
};

// FClusterFilter_HPA: 추상 클러스터 그래프 탐색 필터
struct FClusterFilter
{
    const FClusterAStarGraph& GraphRef;
    const FHPAAbstractGraph* AbstractGraphPtr; // 비용 계산 등을 위해 원본 데이터 접근

    FClusterFilter(const FClusterAStarGraph& InGraphRef, const FHPAAbstractGraph* InAbstractGraph)
        : GraphRef(InGraphRef), AbstractGraphPtr(InAbstractGraph) {}

    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    FVector::FReal GetHeuristicCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const;
    FVector::FReal GetTraversalCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const;
    bool IsTraversalAllowed(FGraphAStarDefaultNode<FClusterAStarGraph> NodeA, FGraphAStarDefaultNode<FClusterAStarGraph> NodeB) const;
    bool WantsPartialSolution() const { return false; }
};

/*
// A* 알고리즘 구현을 위한 Graph 구조체
struct FWaypointGraph
{
    typedef int32 FNodeRef;
    TArray<AUKWaypoint*> Waypoints;

    bool IsValidRef(FNodeRef NodeRef) const
    {
        return Waypoints.IsValidIndex(NodeRef);
    }

    int32 GetNeighbourCount(FNodeRef NodeRef) const
    {
        if (!IsValidRef(NodeRef))
        {
            return 0;
        }
        return Waypoints[NodeRef]->PathPoints.Num();
    }

    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const
    {
        if (!IsValidRef(NodeRef))
        {
            return INDEX_NONE;
        }
        AUKWaypoint* CurrentWP = Waypoints[NodeRef];
        if (!CurrentWP || !CurrentWP->PathPoints.IsValidIndex(NeighbourIndex))
        {
            return INDEX_NONE;
        }
        AUKWaypoint* NeighborWP = CurrentWP->PathPoints[NeighbourIndex];
        return Waypoints.IndexOfByKey(NeighborWP);
    }
};

// A* 알고리즘용 필터 구조체
struct FWaypointFilter
{
    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    
    // 휴리스틱 비용 (현재는 유클리드 거리 사용)
    FVector::FReal GetHeuristicCost(const FGraphAStarDefaultNode<FWaypointGraph>& StartNode, 
                                   const FGraphAStarDefaultNode<FWaypointGraph>& EndNode) const 
    {
        // 실제 거리 기반 휴리스틱 사용
        const AUKWaypoint* StartWP = Graph->Waypoints[StartNode.NodeRef];
        const AUKWaypoint* EndWP = Graph->Waypoints[EndNode.NodeRef];
        
        if (!IsValid(StartWP) || !IsValid(EndWP))
        {
            return MAX_flt;
        }
        
        return FVector::Dist(StartWP->GetActorLocation(), EndWP->GetActorLocation());
    }

    FVector::FReal GetTraversalCost(const FGraphAStarDefaultNode<FWaypointGraph>& StartNode,
                                   const FGraphAStarDefaultNode<FWaypointGraph>& EndNode) const 
    { 
        const AUKWaypoint* StartWP = Graph->Waypoints[StartNode.NodeRef];
        const AUKWaypoint* EndWP = Graph->Waypoints[EndNode.NodeRef];
        
        if (!IsValid(StartWP) || !IsValid(EndWP))
        {
            return MAX_flt;
        }
        
        return FVector::Dist(StartWP->GetActorLocation(), EndWP->GetActorLocation());
    }

    bool IsTraversalAllowed(int32 StartNodeRef, int32 EndNodeRef) const 
    { 
        return true; 
    }

    bool WantsPartialSolution() const { return false; }
    
    uint32 GetMaxSearchNodes() const { return INT_MAX; }
    FVector::FReal GetCostLimit() const { return TNumericLimits<FVector::FReal>::Max(); }

    const FWaypointGraph* Graph;
}; 
*/