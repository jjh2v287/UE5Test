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
    // 싱글톤 접근자
    static UUKNavigationManager* Get() { return Instance; }

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- 웨이포인트 관리 ---
    FWayPointHandle RegisterWaypoint(AUKWayPoint* Waypoint);
    bool UnregisterWaypoint(AUKWayPoint* Waypoint);
    void AllRegisterWaypoint(); // 모든 웨이포인트 강제 재등록 및 맵 업데이트

    // --- HPA 계층 구조 빌드 ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    void BuildHierarchy(bool bForceRebuild = false);

    // --- 경로 탐색 (HPA 적용) ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    TArray<AUKWayPoint*> FindPath(const FVector& StartLocation, const FVector& EndLocation);

    // --- 유틸리티 ---
    AUKWayPoint* FindNearestWaypoint(const FVector& Location, int32 PreferredClusterID = INDEX_NONE) const;

    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Range"))
    AUKWayPoint* FindNearestWayPointinRange(const FVector& Location, const float Range = 1000.0f) const;
    
    UFUNCTION(BlueprintCallable, Category = "WayPoint", meta = (DisplayName = "Find WayPoints In Box"))
    void FindWayPoints(const FVector Location, const float Range, TArray<FWayPointHandle>& OutWayPointHandles) const;
    
    int32 GetClusterIDFromLocation(const FVector& Location) const;

    // --- 디버그 ---
    void DrawDebugHPA(float Duration = 0.f) const;

private:
    // 싱글톤 인스턴스
    static UUKNavigationManager* Instance;

    // 등록된 웨이포인트 관리 맵 (핸들 -> 런타임 데이터)
    UPROPERTY(Transient)
    TMap<FWayPointHandle, FWayPointRuntimeData> RuntimeWayPoints;

    // 공간 해시 그리드 인스턴스
    FWayPointHashGrid2D WaypointGrid;

    // 고유 핸들 ID 생성을 위한 카운터
    uint64 NextHandleID = 1;

    // 고유 핸들 생성 함수 (내부용)
    FWayPointHandle GenerateNewHandle();

    // 웨이포인트 경계 상자 계산 함수 (내부용)
    FBox CalculateWayPointBounds(AUKWayPoint* WayPoint) const;
    
    // 모든 등록된 HPA 웨이포인트 목록 (기존 PathGraph.WayPoints 대신 사용 또는 동기화)
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<AUKWayPoint>> AllWaypoints;

    // 웨이포인트 포인터에서 AllWaypoints 배열의 인덱스로 빠르게 매핑
    UPROPERTY(Transient)
    TMap<AUKWayPoint*, int32> WaypointToIndexMap;
    
    // 빌드된 계층 구조 (추상 그래프)
    FHPAAbstractGraph AbstractGraph;

    // --- 내부 HPA 함수 ---
    bool FindPathLowLevel(AUKWayPoint* StartWP, AUKWayPoint* EndWP, int32 ClusterID, TArray<int32>& OutPathIndices);
    bool FindPathHighLevel(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath);
    TArray<AUKWayPoint*> StitchPath(const FVector& StartLocation, const FVector& EndLocation, AUKWayPoint* StartWP, AUKWayPoint* EndWP, const TArray<int32>& ClusterPath);
    TArray<AUKWayPoint*> ConvertIndicesToWaypoints(const TArray<int32>& Indices) const; // const 추가
    bool FindBestEntranceToNeighbor(AUKWayPoint* CurrentWP, int32 NeighborClusterID, FHPAEntrance& OutEntrance, TArray<int32>& OutPathIndices);
};