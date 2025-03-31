// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKHPADefine.h"
#include "UKWayPoint.h"

const FWayPointHandle FWayPointHandle::Invalid = FWayPointHandle(0);

// FHPACluster
void FHPACluster::CalculateCenter()
{
    if (Waypoints.IsEmpty())
    {
        CenterLocation = FVector::ZeroVector; return;
    }
        
    FVector SumPos = FVector::ZeroVector;
    int32 ValidCount = 0;
    for (const auto& WeakWP : Waypoints)
    {
        if (AUKWayPoint* WP = WeakWP.Get())
        {
            SumPos += WP->GetActorLocation();
            ValidCount++;
        }
    }
    CenterLocation = (ValidCount > 0) ? SumPos / ValidCount : FVector::ZeroVector;
}

// FWayPointAStarGraph
int32 FWayPointAStarGraph::GetNeighbourCount(FNodeRef NodeRef) const
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

FWayPointAStarGraph::FNodeRef FWayPointAStarGraph::GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const
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

// FWayPointFilter
FVector::FReal FWayPointFilter::GetHeuristicCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const
{
    AUKWayPoint* StartWP = Graph.GetWaypoint(StartNode.NodeRef);
    AUKWayPoint* EndWP = Graph.GetWaypoint(EndNode.NodeRef);
    return (StartWP && EndWP) ? FVector::Dist(StartWP->GetActorLocation(), EndWP->GetActorLocation()) : TNumericLimits<FVector::FReal>::Max();
}

FVector::FReal FWayPointFilter::GetTraversalCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const
{
    // 실제 이동 비용 (거리)
    return GetHeuristicCost(StartNode, EndNode);
}

bool FWayPointFilter::IsTraversalAllowed(FGraphAStarDefaultNode<FWayPointAStarGraph> NodeA, FGraphAStarDefaultNode<FWayPointAStarGraph> NodeB) const
{
    // GetNeighbour에서 이미 같은 클러스터인지 확인했으므로 기본적으로 허용
    // 추가 조건(웨이포인트 비활성화 등)이 있다면 여기서 검사
     AUKWayPoint* WPA = Graph.GetWaypoint(NodeA.NodeRef);
     AUKWayPoint* WPB = Graph.GetWaypoint(NodeB.NodeRef);
    return WPA != nullptr && WPB != nullptr; // 유효한 웨이포인트인지 최종 확인
}

// FClusterAStarGraph
bool FClusterAStarGraph::IsValidRef(FNodeRef NodeRef) const {
    return AbstractGraphPtr && AbstractGraphPtr->Clusters.Contains(NodeRef);
}

int32 FClusterAStarGraph::GetNeighbourCount(FNodeRef NodeRef) const {
    if (!AbstractGraphPtr) return 0;
    const FHPACluster* Cluster = AbstractGraphPtr->Clusters.Find(NodeRef);
    return Cluster ? Cluster->Entrances.Num() : 0; // 각 이웃 클러스터 ID가 하나의 이웃 노드
}

FClusterAStarGraph::FNodeRef FClusterAStarGraph::GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const {
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

// FClusterFilter
FVector::FReal FClusterFilter::GetHeuristicCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const {
    if (!AbstractGraphPtr) return TNumericLimits<FVector::FReal>::Max();
    const FHPACluster* StartCluster = AbstractGraphPtr->Clusters.Find(StartNode.NodeRef);
    const FHPACluster* EndCluster = AbstractGraphPtr->Clusters.Find(EndNode.NodeRef);
    // 클러스터 중심점 간의 거리 사용
    return (StartCluster && EndCluster) ? FVector::Dist(StartCluster->CenterLocation, EndCluster->CenterLocation) : TNumericLimits<FVector::FReal>::Max();
}

FVector::FReal FClusterFilter::GetTraversalCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const {
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

bool FClusterFilter::IsTraversalAllowed(FGraphAStarDefaultNode<FClusterAStarGraph> NodeA, FGraphAStarDefaultNode<FClusterAStarGraph> NodeB) const {
    // GetNeighbour에서 유효한 연결만 반환하므로 항상 true
    return AbstractGraphPtr && AbstractGraphPtr->Clusters.Contains(NodeA.NodeRef) && AbstractGraphPtr->Clusters.Contains(NodeB.NodeRef);
}