// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "UKWayPoint.h"
#include "Subsystems/WorldSubsystem.h"
#include "UKHPAManager.generated.h"

/**
 * FWayPointGraph
 * A* 알고리즘 구현을 위한 Graph 구조체
 */
struct FWayPointGraph
{
    typedef int32 FNodeRef;
    TArray<AUKWayPoint*> WayPoints;

    bool IsValidRef(FNodeRef NodeRef) const
    {
        return WayPoints.IsValidIndex(NodeRef);
    }

    int32 GetNeighbourCount(FNodeRef NodeRef) const
    {
        if (!IsValidRef(NodeRef))
        {
            return 0;
        }
        return WayPoints[NodeRef]->PathPoints.Num();
    }

    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const
    {
        if (!IsValidRef(NodeRef))
        {
            return INDEX_NONE;
        }
        AUKWayPoint* CurrentWp = WayPoints[NodeRef];
        if (!CurrentWp || !CurrentWp->PathPoints.IsValidIndex(NeighbourIndex))
        {
            return INDEX_NONE;
        }
        AUKWayPoint* NeighborWp = CurrentWp->PathPoints[NeighbourIndex];
        return WayPoints.IndexOfByKey(NeighborWp);
    }
};

/**
 * A* 알고리즘용 필터 구조체
 */
struct FWayPointFilter
{
    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    
    // 휴리스틱 비용 (현재는 유클리드 거리 사용)
    FVector::FReal GetHeuristicCost(const FGraphAStarDefaultNode<FWayPointGraph>& StartNode, 
                                   const FGraphAStarDefaultNode<FWayPointGraph>& EndNode) const 
    {
        // 실제 거리 기반 휴리스틱 사용
        const AUKWayPoint* StartWP = Graph->WayPoints[StartNode.NodeRef];
        const AUKWayPoint* EndWP = Graph->WayPoints[EndNode.NodeRef];
        
        if (!IsValid(StartWP) || !IsValid(EndWP))
        {
            return MAX_flt;
        }
        
        return FVector::Dist(StartWP->GetActorLocation(), EndWP->GetActorLocation());
    }

    FVector::FReal GetTraversalCost(const FGraphAStarDefaultNode<FWayPointGraph>& StartNode,
                                   const FGraphAStarDefaultNode<FWayPointGraph>& EndNode) const 
    { 
        const AUKWayPoint* StartWP = Graph->WayPoints[StartNode.NodeRef];
        const AUKWayPoint* EndWP = Graph->WayPoints[EndNode.NodeRef];
        
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

    const FWayPointGraph* Graph;
};

UCLASS()
class ZKGAME_API UUKHPAManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// 싱글톤 접근자
	static UUKHPAManager* Get(){ return Instance;};
    
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 웨이포인트 관련 함수들
	void RegisterWaypoint(AUKWayPoint* Waypoint);
	void UnregisterWaypoint(AUKWayPoint* Waypoint);

    // 길찾기 관련 함수들
    TArray<AUKWayPoint*> FindPath(AUKWayPoint* Start, AUKWayPoint* End);
    AUKWayPoint* FindNearestWaypoint(const FVector& Location) const;
    void AllRegisterWaypoint();
    
private:
	// 싱글톤 인스턴스
	static UUKHPAManager* Instance;

    FWayPointGraph PathGraph;
};
