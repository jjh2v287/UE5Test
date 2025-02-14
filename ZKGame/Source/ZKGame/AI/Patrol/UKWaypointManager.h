// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "UKWaypoint.h"
#include "UObject/Object.h"
#include "UKWaypointManager.generated.h"

/**
 * FWaypointGraph
 * A* 알고리즘 구현을 위한 Graph 구조체
 * - AUKWaypoint들을 래핑하여 A* 탐색 알고리즘에서 사용될 그래프 인터페이스를 제공합니다.
 */
struct FWaypointGraph
{
	typedef int32 FNodeRef; // 웨이포인트 인덱스를 노드 식별자로 사용

	// 모든 웨이포인트 배열 (예: 레벨에서 수집한 모든 AUKWaypoint)
	TArray<AUKWaypoint*> Waypoints;

	// 노드(웨이포인트)가 유효한지 확인
	bool IsValidRef(FNodeRef NodeRef) const
	{
		return Waypoints.IsValidIndex(NodeRef);
	}

	// 특정 노드의 이웃 개수를 반환 (PathPoints 배열의 크기)
	int32 GetNeighbourCount(FNodeRef NodeRef) const
	{
		if (!IsValidRef(NodeRef))
		{
			return 0;
		}
		return Waypoints[NodeRef]->PathPoints.Num();
	}

	// 특정 노드의 i번째 이웃을 반환 (배열에서 인덱스)
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
		// Waypoints 배열에서 NeighborWP의 인덱스를 반환
		return Waypoints.IndexOfByKey(NeighborWP);
	}
};

// A* 필터 구조체
// 4. 간단한 Query Filter 구현 (휴리스틱 0, 이동 비용 1 등으로 단순화)
struct FWaypointFilter
{
	FVector::FReal GetHeuristicScale() const { return 1.0f; }

	// 휴리스틱 비용 (예제에서는 0을 리턴하여 Dijkstra처럼 동작)
	FVector::FReal GetHeuristicCost(const FGraphAStarDefaultNode<FWaypointGraph>& StartNode, 
									const FGraphAStarDefaultNode<FWaypointGraph>& EndNode) const 
	{
		return 0.0f;
		// NodeRef가 AZKWaypoint*이므로 위치 정보를 얻을 수 있습니다
		// const AActor* StartWaypoint = AUKWaypointManager::Get()->AllWaypoints[StartNode.NodeRef];
		// const AActor* EndWaypoint = AUKWaypointManager::Get()->AllWaypoints[EndNode.NodeRef];
		//
		// if (!IsValid(StartWaypoint) || !IsValid(EndWaypoint))
		// {
		// 	return MAX_flt;
		// }
		
		// // 유클리드 거리 계산 
		// return FVector::Dist(StartWaypoint->GetActorLocation(), EndWaypoint->GetActorLocation());

		// 또는 맨해튼 거리를 사용할 수도 있습니다
		// const FVector Diff = EndWaypoint->GetActorLocation() - StartWaypoint->GetActorLocation();
		// return FMath::Abs(Diff.X) + FMath::Abs(Diff.Y) + FMath::Abs(Diff.Z);
	}

	// 이동 비용: 간단히 1
	FVector::FReal GetTraversalCost(const FGraphAStarDefaultNode<FWaypointGraph>&, 
									const FGraphAStarDefaultNode<FWaypointGraph>&) const 
	{ 
		return 1.0f; 
	}

	// 모든 이동을 허용
	bool IsTraversalAllowed(int32, int32) const { return true; }

	bool WantsPartialSolution() const { return false; }

	// 최대 노드 수 제한 및 비용 제한 (없음)
	uint32 GetMaxSearchNodes() const { return INT_MAX; }
	FVector::FReal GetCostLimit() const { return TNumericLimits<FVector::FReal>::Max(); }
};

    
// 웨이포인트 매니저 클래스
UCLASS()
class ZKGAME_API AUKWaypointManager  : public AActor
{
	GENERATED_BODY()
public:
	AUKWaypointManager();

	// 싱글턴 인스턴스 접근자
	static AUKWaypointManager* Get();

	// 레벨의 모든 웨이포인트 참조
	UPROPERTY(Transient)
	TArray<AActor*> AllWaypoints;
    
	// 웨이포인트 그래프 초기화
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* StartActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* EndActor = nullptr;
    
	// A* 경로 찾기
	TArray<AUKWaypoint*> FindPath(AUKWaypoint* Start, AUKWaypoint* End);
    
	// 가장 가까운 웨이포인트 찾기
	AUKWaypoint* FindNearestWaypoint(const FVector& Location);
	
	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	};

private:
	// 싱글턴 인스턴스를 저장할 정적 포인터
	static AUKWaypointManager* SingletonInstance;
};