﻿// Copyright Kong Studios, Inc. All Rights Reserved.
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
    TArray<FVector> FindPath(const FVector& StartLocation, const FVector& EndLocation);

    // --- Waypoint Finding ---
    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Range"))
    AUKWayPoint* FindNearestWayPointinRange(const FVector& Location, const float Range = 1000.0f) const;
    
    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Box"))
    void FindWayPoints(const FVector Location, const float Range, TArray<FWayPointHandle>& OutWayPointHandles) const;

    // --- Move ---
    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "AI RequestMove"))
    static void RequestMove(AActor* MoveOwner, const FVector GoalLocation);
    
private:
    static UUKNavigationManager* Instance;

    // Registered Way Point Management Map (handle-> runtime data)
    UPROPERTY(Transient)
    TMap<FWayPointHandle, FWayPointRuntimeData> RuntimeWayPoints;

    // hash grid instance
    FWayPointHashGrid2D WaypointGrid;

    // Counter for creating a unique handle ID
    uint64 NextHandleID = 1;

    // Mapping for all WayPoint indexes
    UPROPERTY(Transient)
    TMap<AUKWayPoint*, FWayPointHandle> WaypointToIndexMap;
    
    // Builded hierarchy (abstract graph)
    FHPAAbstractGraph AbstractGraph;

    // --- Internal HPA function ---
    bool FindPathCluster(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath);
    TArray<AUKWayPoint*> FindPathInCluster(AUKWayPoint* StartWayPoint, AUKWayPoint* EndWayPoint, const TArray<int32>& ClusterPath);
    TArray<FVector> GetPathPointsFromStartToEnd(const FVector& StartPoint, const FVector& EndPoint, const float AgentRadius = 100.0f, const float AgentHeight = 100.0f) const;
    
    // Unique handle generation function (internal use)
    FWayPointHandle GenerateNewHandle();

    // Way Point Border Box Calculation Function (internal use)
    FBox CalculateWayPointBounds(AUKWayPoint* WayPoint) const;

    /**
     * Centripetal Catmull-Rom 스플라인을 사용하여 주어진 경로점에 대한 보간된 경로를 생성합니다. DrawCentripetalCatmullRomSpline 함수를 참고했습니다.
     * 
     * @param Points - 원본 경로점 배열
     * @param Alpha - 텐션 매개변수 (0.5 = Centripetal, 0.0 = Uniform, 1.0 = Chordal)
     * @param NumSamplesPerSegment - 각 세그먼트당 샘플 수 (높을수록 더 부드러운 곡선)
     * @return - 보간된 경로점 배열
     */
    static TArray<FVector> GenerateCentripetalCatmullRomPath(const TArray<FVector>& Points, int32 NumSamplesPerSegment = 4, float Alpha = 0.5f);

public:
    // --- Debug ---
    void DrawDebugHPA(float Duration = 0.f) const;

    // --- Debug. All Way Points forced registration and map update ---
    void AllRegisterWaypoint();
};