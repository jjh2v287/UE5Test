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
    FWayPointHandle RegisterWaypoint(AUKWayPoint* WayPoint);
    bool UnregisterWaypoint(const AUKWayPoint* WayPoint);
    
    // --- HPA hierarchy structure build ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    void BuildHierarchy(bool bForceRebuild = false);

    // --- Path Finding (HPA application) ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    TArray<AUKWayPoint*> FindPath(const FVector& StartLocation, const FVector& EndLocation);

    // --- Waypoint Finding ---
    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Range"))
    AUKWayPoint* FindNearestWayPointinRange(const FVector& Location, const float Range = 1000.0f) const;
    
    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Box"))
    void FindWayPoints(const FVector Location, const float Range, TArray<FWayPointHandle>& OutWayPointHandles) const;
    
private:
    static UUKNavigationManager* Instance;

    // Registered Wei Point Management Map (handle-> runtime data)
    UPROPERTY(Transient)
    TMap<FWayPointHandle, FWayPointRuntimeData> RuntimeWayPoints;

    // hash grid instance
    FWayPointHashGrid2D WaypointGrid;

    // Counter for creating a unique handle ID
    uint64 NextHandleID = 1;

    // All registered HPA Way Points lists (used or synchronized instead of existing PATHRAPH.WAYPOINTS)
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<AUKWayPoint>> AllWaypoints;

    // Mapping quickly to an index of allwayPoints arrangements to an index of all wayPoints from Wei Points
    UPROPERTY(Transient)
    TMap<AUKWayPoint*, int32> WaypointToIndexMap;
    
    // Builded hierarchy (abstract graph)
    FHPAAbstractGraph AbstractGraph;

    // --- Internal HPA function ---
    bool FindPathCluster(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath);
    TArray<AUKWayPoint*> FindPathInCluster(AUKWayPoint* StartWayPoint, AUKWayPoint* EndWayPoint, const TArray<int32>& ClusterPath);
    TArray<FVector> GetPathPointsFromStartToEnd(const FVector& StartPoint, const FVector& EndPoint) const;
    
    // Unique handle generation function (internal use)
    FWayPointHandle GenerateNewHandle();

    // Wei Point Border Box Calculation Function (internal use)
    FBox CalculateWayPointBounds(AUKWayPoint* WayPoint) const;

public:
    // --- Debug ---
    void DrawDebugHPA(float Duration = 0.f) const;

    // --- Debug. All Wei Points forced registration and map update ---
    void AllRegisterWaypoint();
};