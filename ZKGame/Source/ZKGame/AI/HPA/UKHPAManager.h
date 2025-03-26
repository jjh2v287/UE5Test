#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GraphAStar.h"
#include "UKWayPoint.h"
#include "UKHPAManager.generated.h"

// --- HPA 데이터 구조체 정의 ---

const int32 INVALID_CLUSTER_ID_HPA = -1; 

// 전방 선언
class AUKWayPoint;

/** @struct FHPAEntrance */
struct ZKGAME_API FHPAEntrance
{
    TWeakObjectPtr<AUKWayPoint> LocalWaypoint = nullptr;
    TWeakObjectPtr<AUKWayPoint> NeighborWaypoint = nullptr;
    int32 NeighborClusterID = INVALID_CLUSTER_ID_HPA;
    float Cost = 0.0f;

    FHPAEntrance() = default;
    FHPAEntrance(AUKWayPoint* InLocal, AUKWayPoint* InNeighbor, int32 InNeighborClusterID, float InCost)
        : LocalWaypoint(InLocal), NeighborWaypoint(InNeighbor), NeighborClusterID(InNeighborClusterID), Cost(InCost)
    {}
};

/** @struct FHPACluster */
struct ZKGAME_API FHPACluster
{
    int32 ClusterID = INVALID_CLUSTER_ID_HPA;
    TArray<TWeakObjectPtr<AUKWayPoint>> Waypoints;
    TMap<int32, TArray<FHPAEntrance>> Entrances; // Key: NeighborClusterID
    FVector CenterLocation = FVector::ZeroVector;

    FHPACluster() = default;
    explicit FHPACluster(int32 InID) : ClusterID(InID) {}

    void CalculateCenter()
    {
        if (Waypoints.IsEmpty()) { CenterLocation = FVector::ZeroVector; return; }
        FVector SumPos = FVector::ZeroVector;
        int32 ValidCount = 0;
        for (const auto& WeakWP : Waypoints) {
            if (AUKWayPoint* WP = WeakWP.Get()) {
                SumPos += WP->GetActorLocation();
                ValidCount++;
            }
        }
        CenterLocation = (ValidCount > 0) ? SumPos / ValidCount : FVector::ZeroVector;
    }
};

/** @struct FHPAAbstractGraph */
struct ZKGAME_API FHPAAbstractGraph
{
    TMap<int32, FHPACluster> Clusters; // Key: ClusterID
    bool bIsBuilt = false;

    void Clear() {
        Clusters.Empty();
        bIsBuilt = false;
    }
};


// --- HPA용 A* 관련 구조체 ---

// FWayPointGraph_HPA_Internal: 클러스터 내부 탐색용
struct FWayPointGraph_HPA_Internal
{
    typedef int32 FNodeRef; // 전체 웨이포인트 배열의 인덱스 사용

    const TArray<TWeakObjectPtr<AUKWayPoint>>* AllWaypointsPtr = nullptr; // 전체 웨이포인트 배열 참조
    const TMap<AUKWayPoint*, int32>* WaypointToIndexMapPtr = nullptr; // 포인터 -> 인덱스 맵 참조
    int32 CurrentClusterID = INVALID_CLUSTER_ID_HPA; // 탐색 대상 클러스터 ID

    FWayPointGraph_HPA_Internal(const TArray<TWeakObjectPtr<AUKWayPoint>>& InAllWaypoints,
                                const TMap<AUKWayPoint*, int32>& InIndexMap,
                                int32 InClusterID)
        : AllWaypointsPtr(&InAllWaypoints), WaypointToIndexMapPtr(&InIndexMap), CurrentClusterID(InClusterID) {}

    // 노드(웨이포인트) 인덱스의 유효성 검사
    bool IsValidRef(FNodeRef NodeRef) const {
        return AllWaypointsPtr && AllWaypointsPtr->IsValidIndex(NodeRef);
    }

    // 특정 노드(웨이포인트) 인덱스에 해당하는 웨이포인트 객체 반환
    AUKWayPoint* GetWaypoint(FNodeRef NodeRef) const {
        return IsValidRef(NodeRef) ? (*AllWaypointsPtr)[NodeRef].Get() : nullptr;
    }

    // 특정 노드(웨이포인트) 인덱스에서 접근 가능한 이웃 노드 수 반환 (같은 클러스터 내)
    int32 GetNeighbourCount(FNodeRef NodeRef) const; // .cpp에서 구현

    // 특정 노드(웨이포인트) 인덱스의 NeighbourIndex번째 이웃 노드 인덱스 반환 (같은 클러스터 내)
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const; // .cpp에서 구현
};

// FWayPointFilter_HPA_Internal: 클러스터 내부 탐색 필터
struct FWayPointFilter_HPA_Internal
{
    const FWayPointGraph_HPA_Internal& Graph;

    FWayPointFilter_HPA_Internal(const FWayPointGraph_HPA_Internal& InGraph) : Graph(InGraph) {}

    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    FVector::FReal GetHeuristicCost(FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> StartNode, FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> EndNode) const;
    FVector::FReal GetTraversalCost(FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> StartNode, FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> EndNode) const;
    bool IsTraversalAllowed(FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> NodeA, FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> NodeB) const;
    bool WantsPartialSolution() const { return false; }
};


// FClusterGraph_HPA: 추상 클러스터 그래프 탐색용
struct FClusterGraph_HPA
{
    typedef int32 FNodeRef; // ClusterID를 노드 참조로 사용
    const FHPAAbstractGraph* AbstractGraphPtr = nullptr; // 추상 그래프 데이터 참조

    FClusterGraph_HPA(const FHPAAbstractGraph* InGraph) : AbstractGraphPtr(InGraph) {}

    bool IsValidRef(FNodeRef NodeRef) const; // .cpp 구현
    int32 GetNeighbourCount(FNodeRef NodeRef) const; // .cpp 구현
    FNodeRef GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const; // .cpp 구현
};

// FClusterFilter_HPA: 추상 클러스터 그래프 탐색 필터
struct FClusterFilter_HPA
{
    const FClusterGraph_HPA& GraphRef;
    const FHPAAbstractGraph* AbstractGraphPtr; // 비용 계산 등을 위해 원본 데이터 접근

    FClusterFilter_HPA(const FClusterGraph_HPA& InGraphRef, const FHPAAbstractGraph* InAbstractGraph)
        : GraphRef(InGraphRef), AbstractGraphPtr(InAbstractGraph) {}

    FVector::FReal GetHeuristicScale() const { return 1.0f; }
    FVector::FReal GetHeuristicCost(FGraphAStarDefaultNode<FClusterGraph_HPA> StartNode, FGraphAStarDefaultNode<FClusterGraph_HPA> EndNode) const;
    FVector::FReal GetTraversalCost(FGraphAStarDefaultNode<FClusterGraph_HPA> StartNode, FGraphAStarDefaultNode<FClusterGraph_HPA> EndNode) const;
    bool IsTraversalAllowed(FGraphAStarDefaultNode<FClusterGraph_HPA> NodeA, FGraphAStarDefaultNode<FClusterGraph_HPA> NodeB) const;
    bool WantsPartialSolution() const { return false; }
};


// --- UUKHPAManager ---

UCLASS()
class ZKGAME_API UUKHPAManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // 싱글톤 접근자
    static UUKHPAManager* Get() { return Instance; } // 기존 Get() 유지

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- 웨이포인트 관리 ---
    void RegisterWaypoint(AUKWayPoint* Waypoint);
    void UnregisterWaypoint(AUKWayPoint* Waypoint);
    void AllRegisterWaypoint(); // 모든 웨이포인트 강제 재등록 및 맵 업데이트

    // --- HPA 계층 구조 빌드 ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    void BuildHierarchy(bool bForceRebuild = false);

    // --- 경로 탐색 (HPA 적용) ---
    UFUNCTION(BlueprintCallable, Category = "HPA")
    TArray<AUKWayPoint*> FindPath(const FVector& StartLocation, const FVector& EndLocation);

    // --- 유틸리티 ---
    AUKWayPoint* FindNearestWaypoint(const FVector& Location, int32 PreferredClusterID = INVALID_CLUSTER_ID_HPA) const;
    int32 GetClusterIDFromLocation(const FVector& Location) const;

    // --- 디버그 ---
    void DrawDebugHPA(float Duration = 0.f) const; // 디버그 드로잉 함수 (const 추가)

    // 에디터 변경 알림 (선택적)
    // void NotifyWayPointChanged(AUKWayPoint* ChangedWaypoint);

private:
    // 싱글톤 인스턴스
    static UUKHPAManager* Instance;

    // --- 데이터 ---
    // 모든 등록된 HPA 웨이포인트 목록 (기존 PathGraph.WayPoints 대신 사용 또는 동기화)
    TArray<TWeakObjectPtr<AUKWayPoint>> AllWaypoints;
    // 웨이포인트 포인터에서 AllWaypoints 배열의 인덱스로 빠르게 매핑
    TMap<AUKWayPoint*, int32> WaypointToIndexMap;
    // 빌드된 계층 구조 (추상 그래프)
    FHPAAbstractGraph AbstractGraph;

    TWeakObjectPtr<UWorld> WorldPtr; // World 참조

    // --- 내부 HPA 함수 ---
    bool FindPath_LowLevel(AUKWayPoint* StartWP, AUKWayPoint* EndWP, int32 ClusterID, TArray<int32>& OutPathIndices);
    bool FindPath_HighLevel(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath);
    TArray<AUKWayPoint*> StitchPath(const FVector& StartLocation, const FVector& EndLocation, AUKWayPoint* StartWP, AUKWayPoint* EndWP, const TArray<int32>& ClusterPath);
    TArray<AUKWayPoint*> ConvertIndicesToWaypoints(const TArray<int32>& Indices) const; // const 추가
    bool FindEntranceBetweenClusters(int32 FromClusterID, int32 ToClusterID, FHPAEntrance& OutEntrance) const; // const 추가
    bool FindBestEntranceToNeighbor(AUKWayPoint* CurrentWP, int32 NeighborClusterID, FHPAEntrance& OutEntrance, TArray<int32>& OutPathIndices);
};