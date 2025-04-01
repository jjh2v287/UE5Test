// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNavigationDefine.h"
#include "UKWayPoint.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKNavigationDefine)

const FWayPointHandle FWayPointHandle::Invalid = FWayPointHandle(0);

void FHPACluster::CalculateCenter()
{
	if (Waypoints.IsEmpty())
	{
		CenterLocation = FVector::ZeroVector;
		return;
	}

	// Cluster boundary Calculate
	FBox ClusterBounds(EForceInit::ForceInit);
	FVector MinBounds = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector MaxBounds = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	int32 ValidCount = 0;
	for (const TWeakObjectPtr<AUKWayPoint>& WeakWayPoint : Waypoints)
	{
		if (AUKWayPoint* WayPoint = WeakWayPoint.Get())
		{
			FVector WayPointLocation = WayPoint->GetActorLocation();

			MinBounds.X = FMath::Min(MinBounds.X, WayPointLocation.X);
			MinBounds.Y = FMath::Min(MinBounds.Y, WayPointLocation.Y);
			MinBounds.Z = FMath::Min(MinBounds.Z, WayPointLocation.Z);

			MaxBounds.X = FMath::Max(MaxBounds.X, WayPointLocation.X);
			MaxBounds.Y = FMath::Max(MaxBounds.Y, WayPointLocation.Y);
			MaxBounds.Z = FMath::Max(MaxBounds.Z, WayPointLocation.Z);

			ClusterBounds += WayPointLocation;
			ValidCount++;
		}
	}

	Expansion = (ValidCount > 0) ? (MaxBounds - MinBounds) * 0.5f : FVector::ZeroVector;
	ClusterBounds = ClusterBounds.ExpandBy(Expansion);
	CenterLocation = ClusterBounds.GetCenter();
}

#pragma region Test
int32 FWayPointGraph::GetNeighbourCount(FNodeRef NodeRef) const
{
	if (!IsValidRef(NodeRef))
	{
		return 0;
	}
	return Waypoints[NodeRef]->PathPoints.Num();
}

FWayPointGraph::FNodeRef FWayPointGraph::GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const
{
	if (!IsValidRef(NodeRef))
	{
		return INDEX_NONE;
	}
	AUKWayPoint* CurrentWP = Waypoints[NodeRef].Get();
	if (!CurrentWP || !CurrentWP->PathPoints.IsValidIndex(NeighbourIndex))
	{
		return INDEX_NONE;
	}
	AUKWayPoint* NeighborWP = CurrentWP->PathPoints[NeighbourIndex];
	return Waypoints.IndexOfByKey(NeighborWP);
}

FVector::FReal FWayPointFilter::GetHeuristicCost(const FGraphAStarDefaultNode<FWayPointGraph>& StartNode, const FGraphAStarDefaultNode<FWayPointGraph>& EndNode) const
{
	// 실제 거리 기반 휴리스틱 사용
	const AUKWayPoint* StartWP = Graph->Waypoints[StartNode.NodeRef].Get();
	const AUKWayPoint* EndWP = Graph->Waypoints[EndNode.NodeRef].Get();
        
	if (!IsValid(StartWP) || !IsValid(EndWP))
	{
		return MAX_flt;
	}
        
	return FVector::Dist(StartWP->GetActorLocation(), EndWP->GetActorLocation());
}

FVector::FReal FWayPointFilter::GetTraversalCost(const FGraphAStarDefaultNode<FWayPointGraph>& StartNode, const FGraphAStarDefaultNode<FWayPointGraph>& EndNode) const
{ 
	const AUKWayPoint* StartWP = Graph->Waypoints[StartNode.NodeRef].Get();
	const AUKWayPoint* EndWP = Graph->Waypoints[EndNode.NodeRef].Get();
        
	if (!IsValid(StartWP) || !IsValid(EndWP))
	{
		return MAX_flt;
	}
        
	return FVector::Dist(StartWP->GetActorLocation(), EndWP->GetActorLocation());
}

#pragma endregion Test

bool FClusterAStarGraph::IsValidRef(FNodeRef NodeRef) const
{
	return AbstractGraphPtr && AbstractGraphPtr->Clusters.Contains(NodeRef);
}

int32 FClusterAStarGraph::GetNeighbourCount(FNodeRef NodeRef) const
{
	if (!AbstractGraphPtr)
	{
		return 0;
	}
	
	const FHPACluster* Cluster = AbstractGraphPtr->Clusters.Find(NodeRef);

	// Each neighbor cluster ID is one neighbor node
	return Cluster ? Cluster->Entrances.Num() : 0;
}

FClusterAStarGraph::FNodeRef FClusterAStarGraph::GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const
{
	if (!AbstractGraphPtr)
	{
		return INDEX_NONE;
	}
	
	const FHPACluster* Cluster = AbstractGraphPtr->Clusters.Find(NodeRef);
	if (!Cluster || NeighbourIndex < 0)
	{
		return INDEX_NONE;
	}

	TMap<int32, TArray<FHPAEntrance>>::TConstIterator It = Cluster->Entrances.CreateConstIterator();
	int32 CurrentIndex = 0;
	while (It)
	{
		if (CurrentIndex == NeighbourIndex)
		{
			return It.Key();
		}
		
		++It;
		++CurrentIndex;
	}
	return INDEX_NONE;
}

FVector::FReal FClusterFilter::GetHeuristicCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const
{
	if (!AbstractGraphPtr)
	{
		return TNumericLimits<FVector::FReal>::Max();
	}
	
	const FHPACluster* StartCluster = AbstractGraphPtr->Clusters.Find(StartNode.NodeRef);
	const FHPACluster* EndCluster = AbstractGraphPtr->Clusters.Find(EndNode.NodeRef);
	
	// Using distance between cluster center points
	return (StartCluster && EndCluster) ? FVector::Dist(StartCluster->CenterLocation, EndCluster->CenterLocation) : TNumericLimits<FVector::FReal>::Max();
}

FVector::FReal FClusterFilter::GetTraversalCost(FGraphAStarDefaultNode<FClusterAStarGraph> StartNode, FGraphAStarDefaultNode<FClusterAStarGraph> EndNode) const
{
	if (!AbstractGraphPtr)
	{
		return TNumericLimits<FVector::FReal>::Max();
	}

	const FHPACluster* StartCluster = AbstractGraphPtr->Clusters.Find(StartNode.NodeRef);
	if (!StartCluster)
	{
		return TNumericLimits<FVector::FReal>::Max();
	}

	// The minimum value of the entrance cost to the StartCluster to EndNode.NodeRef (neighboring cluster ID) (simple version)
	const TArray<FHPAEntrance>* EntrancesToNeighbor = StartCluster->Entrances.Find(EndNode.NodeRef);
	if (!EntrancesToNeighbor || EntrancesToNeighbor->IsEmpty())
	{
		// It is actually a connected neighbor, so it should not occur
		return TNumericLimits<FVector::FReal>::Max();
	}

	float MinCost = TNumericLimits<float>::Max();
	for (const FHPAEntrance& Entrance : *EntrancesToNeighbor)
	{
		MinCost = FMath::Min(MinCost, Entrance.Cost);
	}
	
	// More accurate cost calculations (considering the inner movement of cluster -pretreatment)
	return MinCost;
}

bool FClusterFilter::IsTraversalAllowed(FGraphAStarDefaultNode<FClusterAStarGraph> NodeA, FGraphAStarDefaultNode<FClusterAStarGraph> NodeB) const
{
	return AbstractGraphPtr && AbstractGraphPtr->Clusters.Contains(NodeA.NodeRef) && AbstractGraphPtr->Clusters. Contains(NodeB.NodeRef);
}
