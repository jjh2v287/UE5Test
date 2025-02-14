// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GraphAStar.h"
#include "UKWaypoint.h"
#include "UKPathFindingSubsystem.generated.h"

/**
 * FWaypointGraph
 * A* 알고리즘 구현을 위한 Graph 구조체
 */
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

/**
 * A* 알고리즘용 필터 구조체
 */
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

/**
 * 길찾기 서브시스템
 */
UCLASS()
class ZKGAME_API UUKPathFindingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // 싱글톤 접근자
    static UUKPathFindingSubsystem* Get(const UWorld* World);
    static UUKPathFindingSubsystem* Get();
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 웨이포인트 관련 함수들
    void RegisterWaypoint(AUKWaypoint* Waypoint);
    void UnregisterWaypoint(AUKWaypoint* Waypoint);
    void ClearWaypoints();
    void AllRegisterWaypoint();
    
    // 길찾기 관련 함수들
    TArray<AUKWaypoint*> FindPath(AUKWaypoint* Start, AUKWaypoint* End);
    AUKWaypoint* FindNearestWaypoint(const FVector& Location) const;

private:
    // 싱글톤 인스턴스
    static UUKPathFindingSubsystem* Instance;

    UPROPERTY(Transient)
    TArray<AUKWaypoint*> WaypointRegistry;
    FWaypointGraph PathGraph;
    
    void RebuildGraph();
};