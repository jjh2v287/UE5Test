// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "UObject/ObjectMacros.h"
#include "Math/Box.h"
#include "HierarchicalHashGrid2D.h"
#include "StructUtils/InstancedStruct.h"
#include "UKNavigationDefine.generated.h"

class AUKWayPoint;

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
    FVector Expansion = FVector::ZeroVector;

    FHPACluster() = default;
    explicit FHPACluster(int32 InID) : ClusterID(InID) {}

    void CalculateCenter();
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

#pragma region WayPoint HashGrid
// 1. WayPoint 핸들 정의 (FSmartObjectHandle 모방)
USTRUCT(BlueprintType)
struct FWayPointHandle
{
    GENERATED_BODY()
public:
    FWayPointHandle() : ID(0) {}

    bool IsValid() const { return ID != 0; }
    void Invalidate() { ID = 0; }

    friend FString LexToString(const FWayPointHandle Handle)
    {
        return FString::Printf(TEXT("WayPoint_0x%016llX"), Handle.ID);
    }

    bool operator==(const FWayPointHandle Other) const { return ID == Other.ID; }
    bool operator!=(const FWayPointHandle Other) const { return !(*this == Other); }
    bool operator<(const FWayPointHandle Other) const { return ID < Other.ID; }

    friend uint32 GetTypeHash(const FWayPointHandle Handle)
    {
        return CityHash32(reinterpret_cast<const char*>(&Handle.ID), sizeof Handle.ID);
    }

private:
    // 서브시스템만 ID를 설정할 수 있도록 함
    friend class UUKNavigationManager;

    explicit FWayPointHandle(const uint64 InID) : ID(InID) {}
    
    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    uint64 ID;

public:
    static const FWayPointHandle Invalid;
};

// THierarchicalHashGrid2D 타입 정의 (FSmartObjectHashGrid2D 모방)
// 템플릿 파라미터는 필요에 따라 조절 (CellSize=2, Density=4는 예시)
using FWayPointHashGrid2D = THierarchicalHashGrid2D<2, 4, FWayPointHandle>;

// 2. 공간 분할 데이터 정의 (FSmartObjectSpatialEntryData 모방)
USTRUCT()
struct FWayPointSpatialEntryData
{
    GENERATED_BODY()
    virtual ~FWayPointSpatialEntryData() = default; // 가상 소멸자 추가 권장
};

// 3. 해시 그리드용 공간 분할 데이터 (FSmartObjectHashGridEntryData 모방)
USTRUCT()
struct FWayPointHashGridEntryData : public FWayPointSpatialEntryData
{
    GENERATED_BODY()

    FWayPointHashGrid2D::FCellLocation CellLoc; // 그리드 셀 위치 저장
};

// 4. WayPoint 런타임 데이터 정의 (FSmartObjectRuntime 모방)
USTRUCT()
struct FWayPointRuntimeData
{
    GENERATED_BODY()

public:
    FWayPointRuntimeData() = default;

    explicit FWayPointRuntimeData(AUKWayPoint* InWayPoint, const FWayPointHandle InHandle)
        : WayPointObject(InWayPoint)
        , Handle(InHandle)
    {
        // 해시 그리드용 공간 데이터로 초기화
        SpatialEntryData.InitializeAs<FWayPointHashGridEntryData>();
    }

    // 웨이포인트 오브젝트 포인터 (WeakObjectPtr 권장)
    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    TWeakObjectPtr<AUKWayPoint> WayPointObject;

    // 고유 핸들
    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    FWayPointHandle Handle;

    // 웨이포인트의 월드 트랜스폼 (등록 시 업데이트)
    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    FTransform Transform;

    // 웨이포인트의 경계 상자 (등록 시 업데이트)
    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    FBox Bounds;

    // 공간 분할 데이터 (FSmartObjectRuntime의 SpatialEntryData 모방)
    UPROPERTY()
    FInstancedStruct SpatialEntryData;
};
#pragma endregion WayPoint HashGrid