#include "UKHPAManager.h" // 헤더 이름 확인
#include "UKWayPoint.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Algo/Reverse.h" // StitchPath에서 사용 가능성 있음

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKHPAManager) // 헤더 이름 확인

// 정적 멤버 변수 초기화
UUKHPAManager* UUKHPAManager::Instance = nullptr;

void UUKHPAManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Instance = this;
    WorldPtr = GetWorld();
    UE_LOG(LogTemp, Log, TEXT("UUKHPAManager Initialized"));
    // 게임 시작 시 자동 빌드 고려
    // BuildHierarchy();
}

void UUKHPAManager::Deinitialize()
{
    AllWaypoints.Empty();
    WaypointToIndexMap.Empty();
    AbstractGraph.Clear();
    WorldPtr = nullptr;
    Instance = nullptr;
    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("UUKHPAManager Deinitialized"));
}

void UUKHPAManager::RegisterWaypoint(AUKWayPoint* Waypoint)
{
    if (IsValid(Waypoint) && !WaypointToIndexMap.Contains(Waypoint))
    {
        AllWaypoints.Add(Waypoint);
        WaypointToIndexMap.Add(Waypoint, AllWaypoints.Num() - 1);
        AbstractGraph.bIsBuilt = false; // 웨이포인트 추가 시 계층 구조 무효화
        // UE_LOG(LogTemp, Verbose, TEXT("Registered Waypoint: %s (Index: %d)"), *Waypoint->GetName(), AllWaypoints.Num() - 1);
    }
}

void UUKHPAManager::UnregisterWaypoint(AUKWayPoint* Waypoint)
{
    if (IsValid(Waypoint))
    {
        int32* IndexPtr = WaypointToIndexMap.Find(Waypoint);
        if (IndexPtr)
        {
            int32 IndexToRemove = *IndexPtr;
            WaypointToIndexMap.Remove(Waypoint); // 맵에서 먼저 제거

            if (AllWaypoints.IsValidIndex(IndexToRemove))
            {
                // 배열 끝 요소와 Swap 후 Pop (인덱스 재조정)
                if (IndexToRemove < AllWaypoints.Num() - 1)
                {
                    AllWaypoints.Swap(IndexToRemove, AllWaypoints.Num() - 1);
                    // 이동된 요소의 인덱스 업데이트
                    AUKWayPoint* SwappedWP = AllWaypoints[IndexToRemove].Get(); // Swap 후 해당 인덱스
                    if (SwappedWP)
                    {
                        // 기존 맵 항목 제거 후 새로 추가 (혹시 모를 중복 방지)
                        WaypointToIndexMap.Remove(SwappedWP);
                        WaypointToIndexMap.Add(SwappedWP, IndexToRemove);
                         // UE_LOG(LogTemp, Verbose, TEXT("Updated index for %s to %d after swap."), *SwappedWP->GetName(), IndexToRemove);
                    }
                }
                // 마지막 요소 제거 (실제로는 Swap 후 마지막이 된 원래 요소를 제거)
                 AllWaypoints.Pop(false); // Shrink 안함
            }
             AbstractGraph.bIsBuilt = false; // 웨이포인트 제거 시 계층 구조 무효화
             // UE_LOG(LogTemp, Verbose, TEXT("Unregistered Waypoint: %s (Removed index: %d)"), *Waypoint->GetName(), IndexToRemove);
        }
    }
}


void UUKHPAManager::AllRegisterWaypoint()
{
    // 1. 기존 정보 클리어
    AllWaypoints.Empty();
    WaypointToIndexMap.Empty();
    AbstractGraph.Clear(); // 계층 구조도 초기화

    // 2. 월드에서 모든 AUKWayPoint 액터 찾기
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> FoundWaypoints;
    UGameplayStatics::GetAllActorsOfClass(World, AUKWayPoint::StaticClass(), FoundWaypoints);

    // 3. 찾은 액터를 캐스팅하여 등록
    AllWaypoints.Reserve(FoundWaypoints.Num()); // 메모리 미리 할당
    for (AActor* Actor : FoundWaypoints)
    {
        if (AUKWayPoint* Waypoint = Cast<AUKWayPoint>(Actor))
        {
            // RegisterWaypoint 내부 로직 직접 수행 (중복 호출 방지)
             if (IsValid(Waypoint) && !WaypointToIndexMap.Contains(Waypoint))
             {
                 AllWaypoints.Add(Waypoint);
                 WaypointToIndexMap.Add(Waypoint, AllWaypoints.Num() - 1);
             }
        }
    }
    UE_LOG(LogTemp, Log, TEXT("AllRegisterWaypoint: Registered %d waypoints."), AllWaypoints.Num());

    // 4. 계층 구조 빌드 (선택적 - 여기서 바로 빌드할지 결정)
    BuildHierarchy(true); // 강제 리빌드
}


void UUKHPAManager::BuildHierarchy(bool bForceRebuild)
{
    if (AbstractGraph.bIsBuilt && !bForceRebuild) {
        // UE_LOG(LogTemp, Log, TEXT("HPA Hierarchy already built. Skipping."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Building HPA Hierarchy..."));
    AbstractGraph.Clear();

    // 1. 클러스터 생성 및 웨이포인트 할당
    for (int32 i = 0; i < AllWaypoints.Num(); ++i) {
        AUKWayPoint* WP = AllWaypoints[i].Get();
        if (!WP) continue;
        if (WP->ClusterID == INVALID_CLUSTER_ID_HPA) {
            UE_LOG(LogTemp, Warning, TEXT("Waypoint %s has invalid ClusterID. Skipping."), *WP->GetName());
            continue;
        }

        FHPACluster& Cluster = AbstractGraph.Clusters.FindOrAdd(WP->ClusterID);
        if (Cluster.ClusterID == INVALID_CLUSTER_ID_HPA) { // 새로 추가된 경우 ID 설정
            Cluster.ClusterID = WP->ClusterID;
        }
        Cluster.Waypoints.Add(WP);
    }

    // 2. 클러스터 간 입구(Entrance) 식별
    for (auto& ClusterPair : AbstractGraph.Clusters) {
        int32 CurrentClusterID = ClusterPair.Key;
        FHPACluster& CurrentCluster = ClusterPair.Value; // 직접 수정 위해 레퍼런스 사용

        for (const auto& WeakWP : CurrentCluster.Waypoints) {
            AUKWayPoint* LocalWP = WeakWP.Get();
            if (!LocalWP) continue;

            for (AUKWayPoint* NeighborWP : LocalWP->PathPoints) { // 직접 연결된 이웃 확인
                if (!NeighborWP) continue;

                // 이웃이 유효하고 다른 클러스터에 속하는지 확인
                if (NeighborWP->ClusterID != INVALID_CLUSTER_ID_HPA && NeighborWP->ClusterID != CurrentClusterID) {
                    // 입구 발견
                    int32 NeighborClusterID = NeighborWP->ClusterID;
                    float Cost = FVector::Dist(LocalWP->GetActorLocation(), NeighborWP->GetActorLocation());

                    // 현재 클러스터의 Entrances 맵에 추가
                    TArray<FHPAEntrance>& EntrancesList = CurrentCluster.Entrances.FindOrAdd(NeighborClusterID);
                    // 중복 입구 방지 (선택적) - 동일한 LocalWP, NeighborWP 쌍이 이미 있는지 확인
                     bool bAlreadyExists = false;
                     for(const auto& ExistingEntrance : EntrancesList) {
                         if(ExistingEntrance.LocalWaypoint == LocalWP && ExistingEntrance.NeighborWaypoint == NeighborWP) {
                             bAlreadyExists = true;
                             break;
                         }
                     }
                     if (!bAlreadyExists) {
                         EntrancesList.Emplace(LocalWP, NeighborWP, NeighborClusterID, Cost);
                          // UE_LOG(LogTemp, Verbose, TEXT("Found Entrance: Cluster %d (%s) -> Cluster %d (%s), Cost: %.f"),
                          //        CurrentClusterID, *LocalWP->GetName(), NeighborClusterID, *NeighborWP->GetName(), Cost);
                     }
                }
            }
        }
    }

    // 3. 클러스터 중심 계산
    for (auto& Pair : AbstractGraph.Clusters) {
        Pair.Value.CalculateCenter();
    }

    AbstractGraph.bIsBuilt = true;
    UE_LOG(LogTemp, Log, TEXT("HPA Hierarchy build complete. %d Clusters found."), AbstractGraph.Clusters.Num());

    // 디버그 드로잉 (옵션)
    DrawDebugHPA();
}

TArray<AUKWayPoint*> UUKHPAManager::FindPath(const FVector& StartLocation, const FVector& EndLocation)
{
    // 0. 계층 구조 빌드 확인
    if (!AbstractGraph.bIsBuilt) {
        UE_LOG(LogTemp, Warning, TEXT("HPA Hierarchy not built. Building now..."));
        BuildHierarchy(true);
        if (!AbstractGraph.bIsBuilt) {
             UE_LOG(LogTemp, Error, TEXT("Failed to build HPA Hierarchy. Cannot find path."));
             return {};
        }
    }
     if (AllWaypoints.IsEmpty()) {
         UE_LOG(LogTemp, Warning, TEXT("No waypoints registered. Cannot find path."));
         return {};
     }


    // 1. 시작/끝점 근처 웨이포인트 찾기
    AUKWayPoint* StartWP = FindNearestWaypoint(StartLocation);
    AUKWayPoint* EndWP = FindNearestWaypoint(EndLocation);

    if (!StartWP || !EndWP) {
        UE_LOG(LogTemp, Warning, TEXT("Could not find nearest waypoints for start or end location."));
        return {};
    }

    int32 StartClusterID = StartWP->ClusterID;
    int32 EndClusterID = EndWP->ClusterID;

    UE_LOG(LogTemp, Log, TEXT("FindPath HPA: Start Loc -> WP %s (Cluster %d), End Loc -> WP %s (Cluster %d)"),
           *StartWP->GetName(), StartClusterID, *EndWP->GetName(), EndClusterID);

    if (StartClusterID == INVALID_CLUSTER_ID_HPA || EndClusterID == INVALID_CLUSTER_ID_HPA) {
         UE_LOG(LogTemp, Warning, TEXT("Start or End Waypoint has invalid ClusterID."));
         return {};
    }

    // 2. 같은 클러스터 내 경로 탐색
    if (StartClusterID == EndClusterID) {
        UE_LOG(LogTemp, Log, TEXT("Start and End in the same Cluster (%d). Performing Low-Level search only."), StartClusterID);
        TArray<int32> PathIndices;
        if (FindPath_LowLevel(StartWP, EndWP, StartClusterID, PathIndices)) {
            // 시작점 -> 첫 WP, 마지막 WP -> 끝점 연결은 시각화 부분에서 처리
            return ConvertIndicesToWaypoints(PathIndices);
        } else {
            UE_LOG(LogTemp, Warning, TEXT("Low-Level pathfinding failed within Cluster %d."), StartClusterID);
            return {};
        }
    }
    // 3. 다른 클러스터 간 경로 탐색
    else {
        UE_LOG(LogTemp, Log, TEXT("Start and End in different Clusters. Performing High-Level search..."));
        TArray<int32> ClusterPathIDs;
        if (FindPath_HighLevel(StartClusterID, EndClusterID, ClusterPathIDs)) {
            UE_LOG(LogTemp, Log, TEXT("High-Level path found: %d Clusters."), ClusterPathIDs.Num());
            // 경로 스티칭
            return StitchPath(StartLocation, EndLocation, StartWP, EndWP, ClusterPathIDs);
        } else {
            UE_LOG(LogTemp, Warning, TEXT("High-Level pathfinding failed between Cluster %d and %d."), StartClusterID, EndClusterID);
            return {};
        }
    }
}

AUKWayPoint* UUKHPAManager::FindNearestWaypoint(const FVector& Location, int32 PreferredClusterID) const
{
    AUKWayPoint* NearestWaypoint = nullptr;
    float MinDistSq = TNumericLimits<float>::Max();
    bool bFoundInPreferred = false;

    // 1. 선호 클러스터 내에서 먼저 탐색 (PreferredClusterID가 유효할 경우)
    if (PreferredClusterID != INVALID_CLUSTER_ID_HPA && AbstractGraph.Clusters.Contains(PreferredClusterID)) {
        const FHPACluster& PreferredCluster = AbstractGraph.Clusters[PreferredClusterID];
        for (const auto& WeakWP : PreferredCluster.Waypoints) {
            if (AUKWayPoint* WP = WeakWP.Get()) {
                float DistSq = FVector::DistSquared(Location, WP->GetActorLocation());
                if (DistSq < MinDistSq) {
                    MinDistSq = DistSq;
                    NearestWaypoint = WP;
                    bFoundInPreferred = true;
                }
            }
        }
    }

    // 2. 선호 클러스터에서 못 찾았거나, 선호 ID가 없었으면 전체 웨이포인트 탐색
    if (!bFoundInPreferred) {
        MinDistSq = TNumericLimits<float>::Max(); // 최소 거리 초기화
        for (const auto& WeakWP : AllWaypoints) {
            if (AUKWayPoint* WP = WeakWP.Get()) {
                float DistSq = FVector::DistSquared(Location, WP->GetActorLocation());
                if (DistSq < MinDistSq) {
                    MinDistSq = DistSq;
                    NearestWaypoint = WP;
                }
            }
        }
    }

    return NearestWaypoint;
}

int32 UUKHPAManager::GetClusterIDFromLocation(const FVector& Location) const
{
    AUKWayPoint* NearestWP = FindNearestWaypoint(Location);
    return NearestWP ? NearestWP->ClusterID : INVALID_CLUSTER_ID_HPA;
}


bool UUKHPAManager::FindPath_LowLevel(AUKWayPoint* StartWP, AUKWayPoint* EndWP, int32 ClusterID, TArray<int32>& OutPathIndices)
{
    OutPathIndices.Empty();
    if (!StartWP || !EndWP || ClusterID == INVALID_CLUSTER_ID_HPA || !AbstractGraph.Clusters.Contains(ClusterID)) {
        return false;
    }
     if (StartWP->ClusterID != ClusterID || EndWP->ClusterID != ClusterID) {
         UE_LOG(LogTemp, Warning, TEXT("FindPath_LowLevel: StartWP(%d) or EndWP(%d) not in target Cluster(%d)."), StartWP->ClusterID, EndWP->ClusterID, ClusterID);
         return false;
     }

    // A* 그래프 및 필터 생성
    FWayPointGraph_HPA_Internal Graph(AllWaypoints, WaypointToIndexMap, ClusterID);
    FWayPointFilter_HPA_Internal Filter(Graph);

    // 시작/끝 노드 인덱스 찾기
    const int32* StartIndexPtr = WaypointToIndexMap.Find(StartWP);
    const int32* EndIndexPtr = WaypointToIndexMap.Find(EndWP);

    if (!StartIndexPtr || !EndIndexPtr) {
        UE_LOG(LogTemp, Error, TEXT("FindPath_LowLevel: Could not find index for StartWP or EndWP."));
        return false;
    }
    int32 StartIndex = *StartIndexPtr;
    int32 EndIndex = *EndIndexPtr;

    // A* 탐색 실행
    FGraphAStar<FWayPointGraph_HPA_Internal> Pathfinder(Graph);
    FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> StartNode(StartIndex);
    FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> EndNode(EndIndex);

    EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, Filter, OutPathIndices);

    if (Result == EGraphAStarResult::SearchSuccess) {
        OutPathIndices.Insert(StartIndex, 0); // 시작 노드 인덱스 추가
        return true;
    }
    // else if (Result == EGraphAStarResult::GoalUnreachable) { ... } // 로그 출력 등
    // else { ... }

    return false;
}

bool UUKHPAManager::FindPath_HighLevel(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath)
{
    OutClusterPath.Empty();
    if (StartClusterID == INVALID_CLUSTER_ID_HPA || EndClusterID == INVALID_CLUSTER_ID_HPA ||
        !AbstractGraph.Clusters.Contains(StartClusterID) || !AbstractGraph.Clusters.Contains(EndClusterID)) {
        return false;
    }

    // A* 그래프 및 필터 생성
    FClusterGraph_HPA Graph(&AbstractGraph);
    FClusterFilter_HPA Filter(Graph, &AbstractGraph);

    // A* 탐색 실행
    FGraphAStar<FClusterGraph_HPA> Pathfinder(Graph);
    FGraphAStarDefaultNode<FClusterGraph_HPA> StartNode(StartClusterID);
    FGraphAStarDefaultNode<FClusterGraph_HPA> EndNode(EndClusterID);

    TArray<int32> PathClusterIDs_Internal; // A* 결과 (시작 노드 미포함)
    EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, Filter, PathClusterIDs_Internal);

    if (Result == EGraphAStarResult::SearchSuccess) {
        OutClusterPath.Add(StartClusterID);
        OutClusterPath.Append(PathClusterIDs_Internal);
        return true;
    }
    // else if (Result == EGraphAStarResult::GoalUnreachable) { ... }
    // else { ... }
    return false;
}

TArray<AUKWayPoint*> UUKHPAManager::StitchPath(
    const FVector& StartLocation, const FVector& EndLocation,
    AUKWayPoint* StartWP, AUKWayPoint* EndWP,
    const TArray<int32>& ClusterPath) // ClusterPath: [StartCluster, C1, C2, ..., EndCluster]
{
    TArray<AUKWayPoint*> FinalPath;
    if (!StartWP || !EndWP || ClusterPath.Num() < 2) { return FinalPath; }

    AUKWayPoint* CurrentOriginWP = StartWP; // 현재 세그먼트 탐색 시작 웨이포인트

    // 클러스터 경로 순회 (마지막 클러스터 전까지)
    for (int32 i = 0; i < ClusterPath.Num() - 1; ++i) {
        int32 CurrentClusterID = ClusterPath[i];
        int32 NextClusterID = ClusterPath[i + 1];

        UE_LOG(LogTemp, Log, TEXT("Stitching: %d -> %d"), CurrentClusterID, NextClusterID);

        FHPAEntrance BestEntrance;
        TArray<int32> BestPathIndices_Internal;
        // 현재 클러스터(CurrentClusterID) 내에서, CurrentOriginWP 부터 시작하여
        // 다음 클러스터(NextClusterID)로 가는 가장 비용이 적은 입구와 경로 찾기
        if (!FindBestEntranceToNeighbor(CurrentOriginWP, NextClusterID, BestEntrance, BestPathIndices_Internal)) {
            UE_LOG(LogTemp, Error, TEXT("StitchPath: Failed to find path segment within Cluster %d towards Cluster %d"), CurrentClusterID, NextClusterID);
            return {}; // 경로 찾기 실패
        }

        // 찾은 경로 세그먼트를 최종 경로에 추가 (중복 제외)
        TArray<AUKWayPoint*> PathSegment = ConvertIndicesToWaypoints(BestPathIndices_Internal);
        if (!PathSegment.IsEmpty()) {
            int32 StartIdx = (FinalPath.IsEmpty()) ? 0 : 1; // 첫 세그먼트 아니면 시작점 제외
            for (int32 k = StartIdx; k < PathSegment.Num(); ++k) {
                FinalPath.Add(PathSegment[k]);
            }
        } else {
             UE_LOG(LogTemp, Error, TEXT("StitchPath: Empty path segment found for Cluster %d -> %d."), CurrentClusterID, NextClusterID);
            return {}; // 비정상 경로
        }

        // 다음 루프를 위해 현재 웨이포인트 업데이트 (다음 클러스터의 진입점으로)
        CurrentOriginWP = BestEntrance.NeighborWaypoint.Get();
        if (!CurrentOriginWP) { // 다음 시작점이 유효하지 않으면 실패
             UE_LOG(LogTemp, Error, TEXT("StitchPath: Invalid next origin waypoint after cluster %d."), CurrentClusterID);
             return {};
        }
        UE_LOG(LogTemp, Log, TEXT("Stitching: Added segment in %d, next start is %s in %d"), CurrentClusterID, *CurrentOriginWP->GetName(), NextClusterID);
    }

    // 마지막 클러스터 내부 경로 탐색 (CurrentOriginWP -> EndWP)
    int32 LastClusterID = ClusterPath.Last();
    UE_LOG(LogTemp, Log, TEXT("Stitching: Final segment in Cluster %d from %s to %s"), LastClusterID, *CurrentOriginWP->GetName(), *EndWP->GetName());

    TArray<int32> LastPathIndices;
    if (FindPath_LowLevel(CurrentOriginWP, EndWP, LastClusterID, LastPathIndices)) {
        TArray<AUKWayPoint*> LastPathSegment = ConvertIndicesToWaypoints(LastPathIndices);
        if (!LastPathSegment.IsEmpty()) {
            int32 StartIdx = (FinalPath.IsEmpty() && LastPathSegment[0] == StartWP) ? 0 : 1; // 첫 세그먼트 아니면 시작점 제외
            if(FinalPath.Num() > 0 && FinalPath.Last() == LastPathSegment[0]) StartIdx = 1; // 더 확실한 중복 체크

            for (int32 k = StartIdx; k < LastPathSegment.Num(); ++k) {
                FinalPath.Add(LastPathSegment[k]);
            }
             UE_LOG(LogTemp, Log, TEXT("Stitching: Added final segment. Total Path Length: %d waypoints."), FinalPath.Num());
        }
         else {
             // 마지막 경로가 비어있는 경우 (예: 시작과 끝 WP가 같음)
              if (CurrentOriginWP == EndWP && FinalPath.Last() != EndWP) { // 마지막 WP가 아직 추가 안됐으면 추가
                  FinalPath.Add(EndWP);
              } else {
                  UE_LOG(LogTemp, Warning, TEXT("StitchPath: Empty final path segment found for Cluster %d."), LastClusterID);
                  // 경로가 아예 없는게 아니라면 여기서 실패 처리할 필요는 없을 수 있음
              }
         }

    } else {
        UE_LOG(LogTemp, Error, TEXT("StitchPath: Failed to find final path segment within Cluster %d."), LastClusterID);
        return {}; // 최종 경로 찾기 실패
    }

    return FinalPath;
}


TArray<AUKWayPoint*> UUKHPAManager::ConvertIndicesToWaypoints(const TArray<int32>& Indices) const
{
    TArray<AUKWayPoint*> Waypoints;
    Waypoints.Reserve(Indices.Num());
    for (int32 Index : Indices) {
        if (AllWaypoints.IsValidIndex(Index)) {
            AUKWayPoint* WP = AllWaypoints[Index].Get();
            if (WP) {
                Waypoints.Add(WP);
            } else { UE_LOG(LogTemp, Warning, TEXT("ConvertIndicesToWaypoints: Null waypoint ptr at index %d."), Index); }
        } else { UE_LOG(LogTemp, Warning, TEXT("ConvertIndicesToWaypoints: Invalid index %d."), Index); }
    }
    return Waypoints;
}

// 두 클러스터 사이의 입구 중 하나 반환 (간단 버전)
bool UUKHPAManager::FindEntranceBetweenClusters(int32 FromClusterID, int32 ToClusterID, FHPAEntrance& OutEntrance) const
{
    const FHPACluster* FromCluster = AbstractGraph.Clusters.Find(FromClusterID);
    if (!FromCluster) return false;

    const TArray<FHPAEntrance>* Entrances = FromCluster->Entrances.Find(ToClusterID);
    if (Entrances && !Entrances->IsEmpty()) {
        // TODO: 더 나은 선택 로직? (예: 거리 기반)
        OutEntrance = (*Entrances)[0]; // 일단 첫 번째 입구 반환
        return true;
    }
    return false;
}

// 현재 WP에서 시작하여 이웃 클러스터로 가는 최적 입구 및 경로 찾기
bool UUKHPAManager::FindBestEntranceToNeighbor(AUKWayPoint* CurrentOriginWP, int32 NeighborClusterID, FHPAEntrance& OutBestEntrance, TArray<int32>& OutBestPathIndices)
{
     if (!CurrentOriginWP || NeighborClusterID == INVALID_CLUSTER_ID_HPA) return false;

     int32 CurrentClusterID = CurrentOriginWP->ClusterID;
     const FHPACluster* CurrentClusterData = AbstractGraph.Clusters.Find(CurrentClusterID);
     if (!CurrentClusterData) return false;

     const TArray<FHPAEntrance>* EntrancesToNext = CurrentClusterData->Entrances.Find(NeighborClusterID);
     if (!EntrancesToNext || EntrancesToNext->IsEmpty()) return false;

     float MinPathCost = TNumericLimits<float>::Max();
     bool bFoundPath = false;

     // 모든 출구 후보에 대해 LowLevel 경로 탐색
     for (const FHPAEntrance& CandidateEntrance : *EntrancesToNext) {
         AUKWayPoint* ExitCandidateWP = CandidateEntrance.LocalWaypoint.Get();
         if (!ExitCandidateWP) continue;

         TArray<int32> PathIndices_Candidate;
         // LowLevel 탐색: CurrentOriginWP -> ExitCandidateWP (모두 CurrentClusterID 내)
         if (FindPath_LowLevel(CurrentOriginWP, ExitCandidateWP, CurrentClusterID, PathIndices_Candidate)) {
             // 경로 비용 계산 (간단: 거리 합)
             float CurrentPathCost = 0.f;
             if (PathIndices_Candidate.Num() > 1) {
                 for (int k = 0; k < PathIndices_Candidate.Num() - 1; ++k) {
                     AUKWayPoint* P1 = AllWaypoints[PathIndices_Candidate[k]].Get();
                     AUKWayPoint* P2 = AllWaypoints[PathIndices_Candidate[k + 1]].Get();
                     if (P1 && P2) CurrentPathCost += FVector::Dist(P1->GetActorLocation(), P2->GetActorLocation());
                 }
             } else if (CurrentOriginWP == ExitCandidateWP) { // 시작점과 출구가 같으면 비용 0
                 CurrentPathCost = 0.f;
             }


             // 최적 경로 업데이트
             if (CurrentPathCost < MinPathCost) {
                 MinPathCost = CurrentPathCost;
                 OutBestPathIndices = PathIndices_Candidate;
                 OutBestEntrance = CandidateEntrance; // 가장 좋았던 입구 정보 저장
                 bFoundPath = true;
             }
         }
     }

     return bFoundPath; // 하나라도 경로를 찾았으면 true
}


void UUKHPAManager::DrawDebugHPA(float Duration) const
{
#if ENABLE_DRAW_DEBUG
    UWorld* World = WorldPtr.Get();
    if (!World || !AbstractGraph.bIsBuilt) return;

    TArray<FColor> ClusterColors = { FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Cyan, FColor::Magenta, FColor::Orange, FColor::Purple, FColor::Turquoise, FColor::Silver };
    int32 ColorIndex = 0;

    for (const auto& ClusterPair : AbstractGraph.Clusters) {
        const FHPACluster& Cluster = ClusterPair.Value;
        FColor CurrentColor = ClusterColors[ColorIndex % ClusterColors.Num()];
        ColorIndex++;

        // 클러스터 내 웨이포인트 및 연결 그리기
        for (const auto& WeakWP : Cluster.Waypoints) {
            AUKWayPoint* WP = WeakWP.Get();
            if (!WP) continue;

            DrawDebugSphere(World, WP->GetActorLocation(), 15.f, 8, CurrentColor, false, Duration, 0, 1.f); // 크기 줄임
            DrawDebugString(World, WP->GetActorLocation() + FVector(0,0,20), FString::Printf(TEXT("C:%d"), Cluster.ClusterID), nullptr, CurrentColor, Duration);

            for (AUKWayPoint* NeighborWP : WP->PathPoints) {
                if (NeighborWP && NeighborWP->ClusterID == Cluster.ClusterID) { // 같은 클러스터 내 연결
                     // 양방향 시 오프셋
                     bool bIsMutual = NeighborWP->PathPoints.Contains(WP);
                     FVector Direction = (NeighborWP->GetActorLocation() - WP->GetActorLocation()).GetSafeNormal();
                     FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
                     float LineOffset = bIsMutual ? 5.0f : 0.0f;

                     DrawDebugLine(World, WP->GetActorLocation() + RightVector * LineOffset, NeighborWP->GetActorLocation() + RightVector * LineOffset, CurrentColor, false, Duration, 0, 1.f);
                }
            }
        }

        // 클러스터 간 입구 그리기 (굵은 흰색 선)
        for (const auto& EntrancePair : Cluster.Entrances) {
            const TArray<FHPAEntrance>& EntrancesList = EntrancePair.Value;
            for (const FHPAEntrance& Entrance : EntrancesList) {
                AUKWayPoint* LocalWP = Entrance.LocalWaypoint.Get();
                AUKWayPoint* NeighborWP = Entrance.NeighborWaypoint.Get();
                if (LocalWP && NeighborWP) {
                    DrawDebugLine(World, LocalWP->GetActorLocation(), NeighborWP->GetActorLocation(), FColor::White, false, Duration, SDPG_Foreground, 5.f); // 굵게, 앞에 보이게
                }
            }
        }
        // 클러스터 중심점 (옵션)
        // DrawDebugSphere(World, Cluster.CenterLocation, 25.f, 8, FColor::Black, false, Duration, 0, 3.f);
    }
#endif
}

// --- HPA용 A* 내부 그래프/필터 구현 ---

// FWayPointGraph_HPA_Internal
int32 FWayPointGraph_HPA_Internal::GetNeighbourCount(FNodeRef NodeRef) const
{
    AUKWayPoint* CurrentWP = GetWaypoint(NodeRef);
    if (!CurrentWP || CurrentWP->ClusterID != CurrentClusterID) return 0; // 현재 클러스터 소속 확인

    int32 Count = 0;
    for (AUKWayPoint* NeighborWP : CurrentWP->PathPoints) {
        // 이웃이 유효하고 같은 클러스터 내에 있어야 함
        if (NeighborWP && NeighborWP->ClusterID == CurrentClusterID) {
            Count++;
        }
    }
    return Count;
}

FWayPointGraph_HPA_Internal::FNodeRef FWayPointGraph_HPA_Internal::GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const
{
     AUKWayPoint* CurrentWP = GetWaypoint(NodeRef);
     if (!CurrentWP || CurrentWP->ClusterID != CurrentClusterID) return INDEX_NONE;

    int32 CurrentValidNeighborIndex = 0;
    for (AUKWayPoint* NeighborWP : CurrentWP->PathPoints) {
        if (NeighborWP && NeighborWP->ClusterID == CurrentClusterID) { // 같은 클러스터 내 이웃만
            if (CurrentValidNeighborIndex == NeighbourIndex) {
                // 해당 이웃의 전체 인덱스 반환
                const int32* IndexPtr = WaypointToIndexMapPtr->Find(NeighborWP);
                return IndexPtr ? *IndexPtr : INDEX_NONE;
            }
            CurrentValidNeighborIndex++;
        }
    }
    return INDEX_NONE;
}

// FWayPointFilter_HPA_Internal
FVector::FReal FWayPointFilter_HPA_Internal::GetHeuristicCost(FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> StartNode, FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> EndNode) const
{
    AUKWayPoint* StartWP = Graph.GetWaypoint(StartNode.NodeRef);
    AUKWayPoint* EndWP = Graph.GetWaypoint(EndNode.NodeRef);
    return (StartWP && EndWP) ? FVector::Dist(StartWP->GetActorLocation(), EndWP->GetActorLocation()) : TNumericLimits<FVector::FReal>::Max();
}

FVector::FReal FWayPointFilter_HPA_Internal::GetTraversalCost(FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> StartNode, FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> EndNode) const
{
    // 실제 이동 비용 (거리)
    return GetHeuristicCost(StartNode, EndNode);
}

bool FWayPointFilter_HPA_Internal::IsTraversalAllowed(FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> NodeA, FGraphAStarDefaultNode<FWayPointGraph_HPA_Internal> NodeB) const
{
    // GetNeighbour에서 이미 같은 클러스터인지 확인했으므로 기본적으로 허용
    // 추가 조건(웨이포인트 비활성화 등)이 있다면 여기서 검사
     AUKWayPoint* WPA = Graph.GetWaypoint(NodeA.NodeRef);
     AUKWayPoint* WPB = Graph.GetWaypoint(NodeB.NodeRef);
    return WPA != nullptr && WPB != nullptr; // 유효한 웨이포인트인지 최종 확인
}

// FClusterGraph_HPA
bool FClusterGraph_HPA::IsValidRef(FNodeRef NodeRef) const {
    return AbstractGraphPtr && AbstractGraphPtr->Clusters.Contains(NodeRef);
}

int32 FClusterGraph_HPA::GetNeighbourCount(FNodeRef NodeRef) const {
    if (!AbstractGraphPtr) return 0;
    const FHPACluster* Cluster = AbstractGraphPtr->Clusters.Find(NodeRef);
    return Cluster ? Cluster->Entrances.Num() : 0; // 각 이웃 클러스터 ID가 하나의 이웃 노드
}

FClusterGraph_HPA::FNodeRef FClusterGraph_HPA::GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const {
     if (!AbstractGraphPtr) return INDEX_NONE;
     const FHPACluster* Cluster = AbstractGraphPtr->Clusters.Find(NodeRef);
     if (!Cluster || NeighbourIndex < 0) return INDEX_NONE;

     // TMap 이터레이터로 NeighbourIndex번째 Key (이웃 클러스터 ID) 반환
     auto It = Cluster->Entrances.CreateConstIterator();
     int32 CurrentIndex = 0;
     while(It) {
         if(CurrentIndex == NeighbourIndex) return It.Key();
         ++It;
         ++CurrentIndex;
     }
     return INDEX_NONE;
}

// FClusterFilter_HPA
FVector::FReal FClusterFilter_HPA::GetHeuristicCost(FGraphAStarDefaultNode<FClusterGraph_HPA> StartNode, FGraphAStarDefaultNode<FClusterGraph_HPA> EndNode) const {
    if (!AbstractGraphPtr) return TNumericLimits<FVector::FReal>::Max();
    const FHPACluster* StartCluster = AbstractGraphPtr->Clusters.Find(StartNode.NodeRef);
    const FHPACluster* EndCluster = AbstractGraphPtr->Clusters.Find(EndNode.NodeRef);
    // 클러스터 중심점 간의 거리 사용
    return (StartCluster && EndCluster) ? FVector::Dist(StartCluster->CenterLocation, EndCluster->CenterLocation) : TNumericLimits<FVector::FReal>::Max();
}

FVector::FReal FClusterFilter_HPA::GetTraversalCost(FGraphAStarDefaultNode<FClusterGraph_HPA> StartNode, FGraphAStarDefaultNode<FClusterGraph_HPA> EndNode) const {
    if (!AbstractGraphPtr) return TNumericLimits<FVector::FReal>::Max();
    const FHPACluster* StartCluster = AbstractGraphPtr->Clusters.Find(StartNode.NodeRef);
    if (!StartCluster) return TNumericLimits<FVector::FReal>::Max();

    // StartCluster에서 EndNode.NodeRef (이웃 클러스터 ID)로 가는 입구 비용 중 최소값 사용 (간단 버전)
    const TArray<FHPAEntrance>* EntrancesToNeighbor = StartCluster->Entrances.Find(EndNode.NodeRef);
    if (!EntrancesToNeighbor || EntrancesToNeighbor->IsEmpty()) {
        return TNumericLimits<FVector::FReal>::Max(); // 실제로는 연결된 이웃이므로 발생하면 안됨
    }

    float MinCost = TNumericLimits<float>::Max();
    for (const FHPAEntrance& Entrance : *EntrancesToNeighbor) {
        MinCost = FMath::Min(MinCost, Entrance.Cost);
    }
    // TODO: 더 정확한 비용 계산 (클러스터 내부 이동 고려 - 전처리 필요)
    return MinCost;
}

bool FClusterFilter_HPA::IsTraversalAllowed(FGraphAStarDefaultNode<FClusterGraph_HPA> NodeA, FGraphAStarDefaultNode<FClusterGraph_HPA> NodeB) const {
    // GetNeighbour에서 유효한 연결만 반환하므로 항상 true
    return AbstractGraphPtr && AbstractGraphPtr->Clusters.Contains(NodeA.NodeRef) && AbstractGraphPtr->Clusters.Contains(NodeB.NodeRef);
}