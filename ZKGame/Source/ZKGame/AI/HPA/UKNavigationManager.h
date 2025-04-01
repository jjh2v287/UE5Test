// Copyright Kong Studios, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GraphAStar.h"
#include "UKNavigationDefine.h"
#include "UKWayPoint.h"
#include "UKNavigationManager.generated.h"

class AUKWayPoint;

UCLASS()
class ZKGAME_API UUKNavigationManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    static UUKNavigationManager* Get() { return Instance; }

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- Waypoint management ---
    FWayPointHandle RegisterWaypoint(AUKWayPoint* Waypoint);
    bool UnregisterWaypoint(AUKWayPoint* Waypoint);
    // All Wei Points forced registration and map update test
    void AllRegisterWaypoint();

    // --- HPA hierarchy structure build ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    void BuildHierarchy(bool bForceRebuild = false);

    // --- Path Finding (HPA application) ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    TArray<AUKWayPoint*> FindPath(const FVector& StartLocation, const FVector& EndLocation);

    // --- Waypoint Finding ---
    AUKWayPoint* FindNearestWaypoint(const FVector& Location, int32 PreferredClusterID = INDEX_NONE) const;

    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Range"))
    AUKWayPoint* FindNearestWayPointinRange(const FVector& Location, const float Range = 1000.0f) const;
    
    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Box"))
    void FindWayPoints(const FVector Location, const float Range, TArray<FWayPointHandle>& OutWayPointHandles) const;
    
    int32 GetClusterIDFromLocation(const FVector& Location) const;

    // --- Debug ---
    void DrawDebugHPA(float Duration = 0.f) const;

private:
    static UUKNavigationManager* Instance;

    // Registered Wei Point Management Map (handle-> runtime data)
    UPROPERTY(Transient)
    TMap<FWayPointHandle, FWayPointRuntimeData> RuntimeWayPoints;

    // hash grid instance
    FWayPointHashGrid2D WaypointGrid;

    // Counter for creating a unique handle ID
    uint64 NextHandleID = 1;

    // Unique handle generation function (internal use)
    FWayPointHandle GenerateNewHandle();

    // Wei Point Border Box Calculation Function (internal use)
    FBox CalculateWayPointBounds(AUKWayPoint* WayPoint) const;
    
    // All registered HPA Way Points lists (used or synchronized instead of existing PATHRAPH.WAYPOINTS)
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<AUKWayPoint>> AllWaypoints;

    // Mapping quickly to an index of allwayPoints arrangements to an index of all wayPoints from Wei Points
    UPROPERTY(Transient)
    TMap<AUKWayPoint*, int32> WaypointToIndexMap;
    
    // Builded hierarchy (abstract graph)
    FHPAAbstractGraph AbstractGraph;

    // --- Internal HPA function ---
    bool FindPathLowLevel(AUKWayPoint* StartWayPoint, AUKWayPoint* EndWayPoint, int32 ClusterID, TArray<int32>& OutPathIndices);
    bool FindPathHighLevel(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath);
    TArray<AUKWayPoint*> StitchPath(const FVector& StartLocation, const FVector& EndLocation, AUKWayPoint* StartWayPoint, AUKWayPoint* EndWayPoint, const TArray<int32>& ClusterPath);
    TArray<AUKWayPoint*> ConvertIndicesToWaypoints(const TArray<int32>& Indices) const; // const 추가
    bool FindBestEntranceToNeighbor(AUKWayPoint* CurrentWayPoint, int32 NeighborClusterID, FHPAEntrance& OutEntrance, TArray<int32>& OutPathIndices);
};