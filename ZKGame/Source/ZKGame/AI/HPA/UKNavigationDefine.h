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

struct ZKGAME_API FWayPointGraph
{
    typedef int32 FNodeRef;
    TArray<TWeakObjectPtr<AUKWayPoint>> Waypoints;

    bool IsValidRef(FNodeRef NodeRef) const
    {
        return Waypoints.IsValidIndex(NodeRef);
    }

    int32 GetNeighbourCount(FNodeRef NodeRef) const;
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const;
};

/**
 * A* 알고리즘용 필터 구조체
 */
struct ZKGAME_API FWayPointFilter
{
    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    
    // 휴리스틱 비용 (현재는 유클리드 거리 사용)
    FVector::FReal GetHeuristicCost(const FGraphAStarDefaultNode<FWayPointGraph>& StartNode, const FGraphAStarDefaultNode<FWayPointGraph>& EndNode) const;

    FVector::FReal GetTraversalCost(const FGraphAStarDefaultNode<FWayPointGraph>& StartNode, const FGraphAStarDefaultNode<FWayPointGraph>& EndNode) const;

    bool IsTraversalAllowed(int32 StartNodeRef, int32 EndNodeRef) const 
    { 
        return true; 
    }

    bool WantsPartialSolution() const { return false; }
    
    uint32 GetMaxSearchNodes() const { return INT_MAX; }
    FVector::FReal GetCostLimit() const { return TNumericLimits<FVector::FReal>::Max(); }

    const FWayPointGraph* Graph;
};

// --- A* related structure for HPA. For interior of the cluster ---
// Abstract cluster graph search
struct ZKGAME_API FClusterAStarGraph
{
    // Use ClusterID as a reference to node
    typedef int32 FNodeRef;

    // See Abstract Graph Data
    const FHPAAbstractGraph* AbstractGraphPtr = nullptr;

    FClusterAStarGraph(const FHPAAbstractGraph* InGraph) : AbstractGraphPtr(InGraph) {}

    bool IsValidRef(FNodeRef NodeRef) const;
    int32 GetNeighbourCount(FNodeRef NodeRef) const;
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const;
};

// Abstract Cluster Graph Filter
struct ZKGAME_API FClusterFilter
{
    const FClusterAStarGraph& GraphRef;

    // Source data access for cost calculation, etc.
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
// 1. WayPoint handle definition (fsmartobjectHandle imitation)
USTRUCT(BlueprintType)
struct ZKGAME_API FWayPointHandle
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
    // Only the UUKNavigationManager can set the ID
    friend class UUKNavigationManager;

    explicit FWayPointHandle(const uint64 InID) : ID(InID) {}
    
    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    uint64 ID;

public:
    static const FWayPointHandle Invalid;
};

// Thierarchicalhashgrid2d Type Definition (fsmartobjecthashgrid2d imitation)
using FWayPointHashGrid2D = THierarchicalHashGrid2D<2, 4, FWayPointHandle>;

// 2. Spatial split data definition (fsmartobjectSpatialEntryData imitation)
USTRUCT()
struct ZKGAME_API FWayPointSpatialEntryData
{
    GENERATED_BODY()
    virtual ~FWayPointSpatialEntryData() = default;
};

// 3. Space split data for hash grid (fsmartobjecthashgridentrydata imitation)
USTRUCT()
struct ZKGAME_API FWayPointHashGridEntryData : public FWayPointSpatialEntryData
{
    GENERATED_BODY()

    // Grid cell location storage
    FWayPointHashGrid2D::FCellLocation CellLoc;
};

// 4. WayPoint Runtime Data Definition (FSMARTOBJECTRUNTIME imitation)
USTRUCT()
struct ZKGAME_API FWayPointRuntimeData
{
    GENERATED_BODY()

public:
    FWayPointRuntimeData() = default;

    explicit FWayPointRuntimeData(AUKWayPoint* InWayPoint, const FWayPointHandle InHandle)
        : WayPointObject(InWayPoint)
        , Handle(InHandle)
    {
        // Initialization with space data for hash grid
        SpatialEntryData.InitializeAs<FWayPointHashGridEntryData>();
    }

    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    TWeakObjectPtr<AUKWayPoint> WayPointObject;

    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    FWayPointHandle Handle;

    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category = WayPoint)
    FBox Bounds;

    UPROPERTY()
    FInstancedStruct SpatialEntryData;
};
#pragma endregion WayPoint HashGrid