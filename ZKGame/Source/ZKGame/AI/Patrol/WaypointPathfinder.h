// WaypointPathfinder.h
#pragma once
#include "CoreMinimal.h"
#include "ZKWaypoint.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

struct FPathNode
{
	AZKWaypoint* Waypoint;
	float Distance;
	AZKWaypoint* PreviousWaypoint;
    
	FPathNode() : Waypoint(nullptr), Distance(MAX_FLT), PreviousWaypoint(nullptr) {}
	FPathNode(AZKWaypoint* InWaypoint) : Waypoint(InWaypoint), Distance(MAX_FLT), PreviousWaypoint(nullptr) {}
};

class ZKGAME_API FWaypointPathfinder
{
public:
	// 디버그 설정 구조체
	struct FDebugSettings
	{
		bool bShowPath = true;
		FColor PathColor = FColor::Yellow;
		float PathThickness = 5.0f;
		float ArrowSize = 200.0f;
		float DrawDuration = -1.0f;  // -1은 영구 표시
        
		bool bShowDistances = true;
		FColor TextColor = FColor::White;
		float TextScale = 1.0f;
	};
	// 가장 가까운 웨이포인트 찾기
	static AZKWaypoint* FindNearestWaypoint(const TArray<AActor*>& Waypoints, const FVector& Location);
    
	// 최단 경로 찾기 (다익스트라 알고리즘)
	static TArray<AZKWaypoint*> FindShortestPath(AZKWaypoint* StartWaypoint, AZKWaypoint* EndWaypoint);
    
	// 상호작용 위치에서 목적지까지의 최단 경로 찾기
	static TArray<AZKWaypoint*> FindPathFromInteractionPoint(
		const FVector& InteractionPoint,
		const TArray<AActor*>& AllWaypoints,
		AZKWaypoint* DestinationWaypoint);

	// 디버그 드로잉 함수들
	static void DrawDebugPath(const UWorld* World, const TArray<AZKWaypoint*>& Path, const FDebugSettings& DebugSettings = FDebugSettings());
	static void DrawDebugInteractionToPath(const UWorld* World, const FVector& InteractionPoint, const TArray<AZKWaypoint*>& Path, const FDebugSettings& DebugSettings = FDebugSettings());
};