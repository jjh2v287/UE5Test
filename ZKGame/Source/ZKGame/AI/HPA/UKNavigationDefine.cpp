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

int32 FWayPointAStarGraph::GetNeighbourCount(FNodeRef NodeRef) const
{
	AUKWayPoint* CurrentWayPoint = GetWaypoint(NodeRef);
	if (!CurrentWayPoint || CurrentWayPoint->ClusterID != CurrentClusterID)
	{
		return 0;
	}

	int32 Count = 0;
	for (AUKWayPoint* NeighborWayPoint : CurrentWayPoint->PathPoints)
	{
		// Neighbors must be in the same cluster
		if (NeighborWayPoint && NeighborWayPoint->ClusterID == CurrentClusterID)
		{
			Count++;
		}
	}
	return Count;
}

FWayPointAStarGraph::FNodeRef FWayPointAStarGraph::GetNeighbour(const FNodeRef& NodeRef, int32 NeighbourIndex) const
{
	AUKWayPoint* CurrentWayPoint = GetWaypoint(NodeRef);
	if (!CurrentWayPoint || CurrentWayPoint->ClusterID != CurrentClusterID)
	{
		return INDEX_NONE;
	}

	int32 CurrentValidNeighborIndex = 0;
	for (AUKWayPoint* NeighborWayPoint : CurrentWayPoint->PathPoints)
	{
		if (NeighborWayPoint && NeighborWayPoint->ClusterID == CurrentClusterID)
		{
			// Only in the same cluster
			if (CurrentValidNeighborIndex == NeighbourIndex)
			{
				// Return to the entire index of the neighborhood
				const int32* IndexPtr = WaypointToIndexMapPtr->Find(NeighborWayPoint);
				return IndexPtr ? *IndexPtr : INDEX_NONE;
			}
			CurrentValidNeighborIndex++;
		}
	}
	
	return INDEX_NONE;
}

FVector::FReal FWayPointFilter::GetHeuristicCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const
{
	AUKWayPoint* StartWayPoint = Graph.GetWaypoint(StartNode.NodeRef);
	AUKWayPoint* EndWayPoint = Graph.GetWaypoint(EndNode.NodeRef);
	
	return (StartWayPoint && EndWayPoint) ? FVector::Dist(StartWayPoint->GetActorLocation(), EndWayPoint->GetActorLocation()) : TNumericLimits<FVector::FReal>::Max();
}

FVector::FReal FWayPointFilter::GetTraversalCost(FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode, FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode) const
{
	// Actual movement cost (distance)
	return GetHeuristicCost(StartNode, EndNode);
}

bool FWayPointFilter::IsTraversalAllowed(FGraphAStarDefaultNode<FWayPointAStarGraph> NodeA, FGraphAStarDefaultNode<FWayPointAStarGraph> NodeB) const
{
	AUKWayPoint* WayPointA = Graph.GetWaypoint(NodeA.NodeRef);
	AUKWayPoint* WayPointB = Graph.GetWaypoint(NodeB.NodeRef);

	// Check the final whether it is a valid way point
	return WayPointA != nullptr && WayPointB != nullptr;
}

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
