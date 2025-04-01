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

// --- A* related structure for HPA. For interior of the cluster ---
struct ZKGAME_API FWayPointAStarGraph
{
    // Use of indexes of full Wey -point array
    typedef int32 FNodeRef;

    // Refer to the entire way point array
    const TArray<TWeakObjectPtr<AUKWayPoint>>* AllWaypointsPtr = nullptr;

    // Pointer-> See index maps
    const TMap<AUKWayPoint*, int32>* WaypointToIndexMapPtr = nullptr;

    // Cluster ID to be explored
    int32 CurrentClusterID = INDEX_NONE;

    FWayPointAStarGraph(const TArray<TWeakObjectPtr<AUKWayPoint>>& InAllWaypoints, const TMap<AUKWayPoint*, int32>& InIndexMap, int32 InClusterID)
        : AllWaypointsPtr(&InAllWaypoints), WaypointToIndexMapPtr(&InIndexMap), CurrentClusterID(InClusterID) {}

    // Validate waypoint index
    bool IsValidRef(FNodeRef NodeRef) const
    {
        return AllWaypointsPtr && AllWaypointsPtr->IsValidIndex(NodeRef);
    }

    // Return of the Way Point Object, which corresponds to a specific way point index
    AUKWayPoint* GetWaypoint(FNodeRef NodeRef) const
    {
        return IsValidRef(NodeRef) ? (*AllWaypointsPtr)[NodeRef].Get() : nullptr;
    }

    // Returns to neighboring nodes accessible from certain Wei Point indexes (within the same cluster)
    int32 GetNeighbourCount(FNodeRef NodeRef) const;

    // Returns the index of the Nth neighboring node of a given waypoint index (within the same cluster).
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const;
};

// Cluster internal search filter
struct ZKGAME_API FWayPointFilter
{
    const FWayPointAStarGraph& Graph;

    FWayPointFilter(const FWayPointAStarGraph& InGraph) : Graph(InGraph) {}

    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    FVector::FReal GetHeuristicCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const;
    FVector::FReal GetTraversalCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const;
    bool IsTraversalAllowed(FGraphAStarDefaultNode<FWayPointAStarGraph> NodeA, FGraphAStarDefaultNode<FWayPointAStarGraph> NodeB) const;
    bool WantsPartialSolution() const { return false; }
};


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