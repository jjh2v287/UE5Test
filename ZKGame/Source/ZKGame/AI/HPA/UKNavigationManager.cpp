// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNavigationManager.h"
#include "UKWayPoint.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/TextRenderComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKNavigationManager)

UUKNavigationManager* UUKNavigationManager::Instance = nullptr;

void UUKNavigationManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
	// Consider automatic build at the start of the game
	// BuildHierarchy();
}

void UUKNavigationManager::Deinitialize()
{
	AllWaypoints.Empty();
	WaypointToIndexMap.Empty();
	AbstractGraph.Clear();
	Instance = nullptr;
	Super::Deinitialize();
}

FWayPointHandle UUKNavigationManager::RegisterWaypoint(AUKWayPoint* WayPoint)
{
	if (!IsValid(WayPoint))
	{
		return FWayPointHandle::Invalid;
		
	}

	if (WaypointToIndexMap.Contains(WayPoint))
	{
		return WayPoint->GetWayPointHandle();
		
	}

	//----------- HPA -----------
	AllWaypoints.Add(WayPoint);
	WaypointToIndexMap.Add(WayPoint, AllWaypoints.Num() - 1);
	// Invalidation of the hierarchy when adding WeightPoints
	AbstractGraph.bIsBuilt = false;

	//----------- HashGrid -----------
	if (RuntimeWayPoints.Contains(WayPoint->GetWayPointHandle()))
	{
		return WayPoint->GetWayPointHandle();
	}

    // Create a new handle
    FWayPointHandle NewHandle = GenerateNewHandle();
    if (!NewHandle.IsValid())
    {
        return FWayPointHandle::Invalid;
    }

    // Creating and adding to the runtime data to the map
    FWayPointRuntimeData& RuntimeData = RuntimeWayPoints.Emplace(NewHandle, FWayPointRuntimeData(WayPoint, NewHandle));

    // Boundary box and transform calculation/storage
    RuntimeData.Bounds = CalculateWayPointBounds(WayPoint);
	RuntimeData.Transform = WayPoint->GetActorTransform();


    // Add to the Hash grid
    FWayPointHashGridEntryData* GridEntryData = RuntimeData.SpatialEntryData.GetMutablePtr<FWayPointHashGridEntryData>();
    if (GridEntryData)
    {
        GridEntryData->CellLoc = WaypointGrid.Add(NewHandle, RuntimeData.Bounds);
    }
    else
    {
        // Registration failure processing: removal from the map, etc.
        RuntimeWayPoints.Remove(NewHandle);
        return FWayPointHandle::Invalid;
    }

    return NewHandle;
}

bool UUKNavigationManager::UnregisterWaypoint(const AUKWayPoint* WayPoint)
{
	if (!IsValid(WayPoint))
	{
		return false;
	}

	//----------- HPA -----------
	int32* IndexPtr = WaypointToIndexMap.Find(WayPoint);
	if (IndexPtr)
	{
		int32 IndexToRemove = *IndexPtr;
		
		WaypointToIndexMap.Remove(WayPoint);

		if (AllWaypoints.IsValidIndex(IndexToRemove))
		{
			if (IndexToRemove < AllWaypoints.Num() - 1)
			{
				AllWaypoints.Swap(IndexToRemove, AllWaypoints.Num() - 1);
				AUKWayPoint* SwappedWayPoint = AllWaypoints[IndexToRemove].Get();
				if (SwappedWayPoint)
				{
					WaypointToIndexMap.Remove(SwappedWayPoint);
					WaypointToIndexMap.Add(SwappedWayPoint, IndexToRemove);
				}
			}
			AllWaypoints.Pop(EAllowShrinking::No);
		}

		AbstractGraph.bIsBuilt = false;
	}

	//----------- HashGrid -----------
	FWayPointRuntimeData RuntimeData;
	if (RuntimeWayPoints.RemoveAndCopyValue(WayPoint->GetWayPointHandle(), RuntimeData))
	{
		const FWayPointHashGridEntryData* GridEntryData = RuntimeData.SpatialEntryData.GetPtr<FWayPointHashGridEntryData>();
		if (GridEntryData)
		{
			WaypointGrid.Remove(WayPoint->GetWayPointHandle(), GridEntryData->CellLoc);
		}
		else
		{
		}
		return true;
	}

	return false;
}


void UUKNavigationManager::AllRegisterWaypoint()
{
	// 1. Existing information clear
	AllWaypoints.Empty();
	WaypointToIndexMap.Empty();
	AbstractGraph.Clear();

	// 2. Find all AUKWayPoint actors in the world
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundWaypoints;
	UGameplayStatics::GetAllActorsOfClass(World, AUKWayPoint::StaticClass(), FoundWaypoints);

	// 3. Register the found actor
	AllWaypoints.Reserve(FoundWaypoints.Num());
	for (AActor* Actor : FoundWaypoints)
	{
		if (AUKWayPoint* Waypoint = Cast<AUKWayPoint>(Actor))
		{
			//----------- HPA -----------
			if (IsValid(Waypoint) && !WaypointToIndexMap.Contains(Waypoint))
			{
				AllWaypoints.Add(Waypoint);
				WaypointToIndexMap.Add(Waypoint, AllWaypoints.Num() - 1);
			}

			//----------- HashGrid -----------
			if (RuntimeWayPoints.Contains(Waypoint->GetWayPointHandle()))
			{
				continue;
			}

			// Create a new handle
			FWayPointHandle NewHandle = GenerateNewHandle();
			if (!NewHandle.IsValid())
			{
				continue;
			}

			// Creating and adding to the runtime data to the map
			FWayPointRuntimeData& RuntimeData = RuntimeWayPoints.Emplace(NewHandle, FWayPointRuntimeData(Waypoint, NewHandle));

			// Boundary box and transform calculation/storage
			RuntimeData.Bounds = CalculateWayPointBounds(Waypoint);
			RuntimeData.Transform = Waypoint->GetActorTransform();


			// Add to grid
			FWayPointHashGridEntryData* GridEntryData = RuntimeData.SpatialEntryData.GetMutablePtr<FWayPointHashGridEntryData>();
			if (GridEntryData)
			{
				GridEntryData->CellLoc = WaypointGrid.Add(NewHandle, RuntimeData.Bounds);
			}
			else
			{
				// Registration failure processing: removal from the map, etc.
				RuntimeWayPoints.Remove(NewHandle);
				continue;
			}

			Waypoint->SetWayPointHandle(NewHandle);
		}
	}

	// 4. Hierarchical structure build (optional -decision to build right here)
	BuildHierarchy(true);
}


void UUKNavigationManager::BuildHierarchy(bool bForceRebuild)
{
	if (AbstractGraph.bIsBuilt && !bForceRebuild)
	{
		UE_LOG(LogTemp, Log, TEXT("HPA Hierarchy already built. Skipping."));
		return;
	}

	AbstractGraph.Clear();

	// 1. Cluster creation and way point assignment
	for (int32 i = 0; i < AllWaypoints.Num(); ++i)
	{
		AUKWayPoint* WayPoint = AllWaypoints[i].Get();
		if (!WayPoint)
		{
			continue;
		}
		
		if (WayPoint->ClusterID == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Waypoint %s has invalid ClusterID. Skipping."), *WayPoint->GetName());
			continue;
		}

		FHPACluster& Cluster = AbstractGraph.Clusters.FindOrAdd(WayPoint->ClusterID);
		if (Cluster.ClusterID == INDEX_NONE)
		{
			// If the newly added ID setting
			Cluster.ClusterID = WayPoint->ClusterID;
		}
		Cluster.Waypoints.Add(WayPoint);
	}

	// 2. Identification of Entrance between clusters
	for (auto& ClusterPair : AbstractGraph.Clusters)
	{
		const int32 CurrentClusterID = ClusterPair.Key;
		FHPACluster& CurrentCluster = ClusterPair.Value;

		for (const auto& WeakWayPoint : CurrentCluster.Waypoints)
		{
			AUKWayPoint* LocalWayPoint = WeakWayPoint.Get();
			if (!LocalWayPoint)
			{
				continue;
			}

			for (AUKWayPoint* NeighborWayPoint : LocalWayPoint->PathPoints)
			{
				if (!NeighborWayPoint)
				{
					continue;
				}

				// Make sure your neighbors are valid and belong to other clusters
				if (NeighborWayPoint->ClusterID != INDEX_NONE && NeighborWayPoint->ClusterID != CurrentClusterID)
				{
					int32 NeighborClusterID = NeighborWayPoint->ClusterID;
					float Cost = FVector::Dist(LocalWayPoint->GetActorLocation(), NeighborWayPoint->GetActorLocation());

					// Add to the current cluster's Entrances map
					TArray<FHPAEntrance>& EntrancesList = CurrentCluster.Entrances.FindOrAdd(NeighborClusterID);
					// Duplicate entrance prevention (optional) -Checks that the same LocalWayPoint, NeighborWayPoint pairs are already available
					bool bAlreadyExists = false;
					for (const auto& ExistingEntrance : EntrancesList)
					{
						if (ExistingEntrance.LocalWaypoint == LocalWayPoint && ExistingEntrance.NeighborWaypoint == NeighborWayPoint)
						{
							bAlreadyExists = true;
							break;
						}
					}
					
					if (!bAlreadyExists)
					{
						EntrancesList.Emplace(LocalWayPoint, NeighborWayPoint, NeighborClusterID, Cost);
					}
				}
			}
		}
	}

	// 3. Cluster -centered calculation
	for (auto& Pair : AbstractGraph.Clusters)
	{
		Pair.Value.CalculateCenter();
	}

	AbstractGraph.bIsBuilt = true;
}

TArray<AUKWayPoint*> UUKNavigationManager::FindPath(const FVector& StartLocation, const FVector& EndLocation)
{
	// 0. Check the hierarchy structure build
	if (!AbstractGraph.bIsBuilt)
	{
		BuildHierarchy(true);
		if (!AbstractGraph.bIsBuilt)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to build HPA Hierarchy. Cannot find path."));
			return {};
		}
	}
	
	if (AllWaypoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No waypoints registered. Cannot find path."));
		return {};
	}


	// 1. Find a way point near the start/endpoint
	AUKWayPoint* StartWayPoint = FindNearestWayPointinRange(StartLocation);
	AUKWayPoint* EndWayPoint = FindNearestWayPointinRange(EndLocation);

	if (!StartWayPoint || !EndWayPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find nearest waypoints for start or end location."));
		return {};
	}

	int32 StartClusterID = StartWayPoint->ClusterID;
	int32 EndClusterID = EndWayPoint->ClusterID;
	if (StartClusterID == INDEX_NONE || EndClusterID == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Start or End Waypoint has invalid ClusterID."));
		return {};
	}

	// 2. Search for paths in the same cluster
	if (StartClusterID == EndClusterID)
	{
		TArray<int32> PathIndices;
		if (FindPathLowLevel(StartWayPoint, EndWayPoint, StartClusterID, PathIndices))
		{
			// Starting point-> First WayPoint, last WayPoint-> end point connection is processed in the visualization part
			return ConvertIndicesToWaypoints(PathIndices);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Low-Level pathfinding failed within Cluster %d."), StartClusterID);
			return {};
		}
	}
	// 3. Search for path between other clusters
	else
	{
		TArray<int32> ClusterPathIDs;
		if (FindPathHighLevel(StartClusterID, EndClusterID, ClusterPathIDs))
		{
			// Path Stitching
			return StitchPath(StartLocation, EndLocation, StartWayPoint, EndWayPoint, ClusterPathIDs);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("High-Level pathfinding failed between Cluster %d and %d."), StartClusterID, EndClusterID);
			return {};
		}
	}
}

AUKWayPoint* UUKNavigationManager::FindNearestWaypoint(const FVector& Location, int32 PreferredClusterID) const
{
	AUKWayPoint* NearestWaypoint = nullptr;
	float MinDistSq = TNumericLimits<float>::Max();
	bool bFoundInPreferred = false;

	// 1. Search first within the preferred cluster (if the PreferredClusterID is valid)
	if (PreferredClusterID != INDEX_NONE && AbstractGraph.Clusters.Contains(PreferredClusterID))
	{
		const FHPACluster& PreferredCluster = AbstractGraph.Clusters[PreferredClusterID];
		for (const auto& WeakWayPoint : PreferredCluster.Waypoints)
		{
			if (AUKWayPoint* WayPoint = WeakWayPoint.Get())
			{
				float DistSq = FVector::DistSquared(Location, WayPoint->GetActorLocation());
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					NearestWaypoint = WayPoint;
					bFoundInPreferred = true;
				}
			}
		}
	}

	// 2. If you haven't found it in your preferred cluster, or if you didn't have a preferred ID,
	if (!bFoundInPreferred)
	{
		MinDistSq = TNumericLimits<float>::Max();
		for (const auto& WeakWayPoint : AllWaypoints)
		{
			if (AUKWayPoint* WayPoint = WeakWayPoint.Get())
			{
				float DistSq = FVector::DistSquared(Location, WayPoint->GetActorLocation());
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					NearestWaypoint = WayPoint;
				}
			}
		}
	}

	return NearestWaypoint;
}

AUKWayPoint* UUKNavigationManager::FindNearestWayPointinRange(const FVector& Location, const float Range /*= 1000.0f*/) const
{
	AUKWayPoint* NearestWaypoint = nullptr;

	FVector SearchExtent(Range);
	FBox QueryBox = FBox(Location - SearchExtent, Location + SearchExtent);

	// Grid query
	TArray<FWayPointHandle> FoundHandlesInGrid;
	WaypointGrid.QuerySmall(QueryBox, FoundHandlesInGrid);

	// Grid results verification (further filtering, such as the existence and activation of runtime data)
	float MinRange = Range;
	for (const FWayPointHandle& Handle : FoundHandlesInGrid)
	{
		const FWayPointRuntimeData* RuntimeData = RuntimeWayPoints.Find(Handle);
		if (RuntimeData && RuntimeData->WayPointObject.IsValid())
		{
			const float Dist = FVector::Dist2D(Location, RuntimeData->Transform.GetLocation());
			if (Dist < MinRange)
			{
				MinRange = Dist;
				NearestWaypoint = RuntimeData->WayPointObject.Get();
			}
		}
	}

	return NearestWaypoint;
}

void UUKNavigationManager::FindWayPoints(const FVector Location, const float Range, TArray<FWayPointHandle>& OutWayPointHandles) const
{
	OutWayPointHandles.Reset();

	FVector SearchExtent(Range);
	FBox QueryBox = FBox(Location - SearchExtent, Location + SearchExtent);

	// Grid query
	TArray<FWayPointHandle> FoundHandlesInGrid;
	WaypointGrid.QuerySmall(QueryBox, FoundHandlesInGrid);

	// Grid results verification (further filtering, such as the existence and activation of runtime data)
	for (const FWayPointHandle& Handle : FoundHandlesInGrid)
	{
		const FWayPointRuntimeData* RuntimeData = RuntimeWayPoints.Find(Handle);
		if (RuntimeData && RuntimeData->WayPointObject.IsValid())
		{
			// The final confirmation of the overlap between the query box and the actual border box (because the grid is based on a cell)
			if (RuntimeData->Bounds.Intersect(QueryBox))
			{
				OutWayPointHandles.Add(Handle);
			}
		}
	}
}

int32 UUKNavigationManager::GetClusterIDFromLocation(const FVector& Location) const
{
	AUKWayPoint* NearestWayPoint = FindNearestWaypoint(Location);
	return NearestWayPoint ? NearestWayPoint->ClusterID : INDEX_NONE;
}

bool UUKNavigationManager::FindPathLowLevel(AUKWayPoint* StartWayPoint, AUKWayPoint* EndWayPoint, int32 ClusterID, TArray<int32>& OutPathIndices)
{
	OutPathIndices.Empty();
	if (!StartWayPoint || !EndWayPoint || ClusterID == INDEX_NONE || !AbstractGraph.Clusters.Contains(ClusterID))
	{
		return false;
	}
	
	if (StartWayPoint->ClusterID != ClusterID || EndWayPoint->ClusterID != ClusterID)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPath_LowLevel: StartWayPoint(%d) or EndWayPoint(%d) not in target Cluster(%d)."), StartWayPoint->ClusterID, EndWayPoint->ClusterID, ClusterID);
		return false;
	}

	// A* Graph and filter creation
	FWayPointAStarGraph Graph(AllWaypoints, WaypointToIndexMap, ClusterID);
	FWayPointFilter Filter(Graph);

	// Find Start/End Node Index
	const int32* StartIndexPtr = WaypointToIndexMap.Find(StartWayPoint);
	const int32* EndIndexPtr = WaypointToIndexMap.Find(EndWayPoint);

	if (!StartIndexPtr || !EndIndexPtr)
	{
		UE_LOG(LogTemp, Error, TEXT("FindPath_LowLevel: Could not find index for StartWayPoint or EndWayPoint."));
		return false;
	}
	int32 StartIndex = *StartIndexPtr;
	int32 EndIndex = *EndIndexPtr;

	// A* Search execution
	FGraphAStar<FWayPointAStarGraph> Pathfinder(Graph);
	FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode(StartIndex);
	FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode(EndIndex);

	EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, Filter, OutPathIndices);

	if (Result == EGraphAStarResult::SearchSuccess)
	{
		// Start node index added
		OutPathIndices.Insert(StartIndex, 0);
		return true;
	}

	return false;
}

bool UUKNavigationManager::FindPathHighLevel(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath)
{
	OutClusterPath.Empty();
	if (StartClusterID == INDEX_NONE || EndClusterID == INDEX_NONE || !AbstractGraph.Clusters.Contains(StartClusterID) || !AbstractGraph.Clusters.Contains(EndClusterID))
	{
		return false;
	}

	// A* Graph and filter creation
	FClusterAStarGraph Graph(&AbstractGraph);
	FClusterFilter Filter(Graph, &AbstractGraph);

	// A* Search execution
	FGraphAStar<FClusterAStarGraph> Pathfinder(Graph);
	FGraphAStarDefaultNode<FClusterAStarGraph> StartNode(StartClusterID);
	FGraphAStarDefaultNode<FClusterAStarGraph> EndNode(EndClusterID);

	// A* Results (non -starting node)
	TArray<int32> PathClusterIDs;
	EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, Filter, PathClusterIDs);

	if (Result == EGraphAStarResult::SearchSuccess)
	{
		OutClusterPath.Add(StartClusterID);
		OutClusterPath.Append(PathClusterIDs);
		return true;
	}
	
	return false;
}

TArray<AUKWayPoint*> UUKNavigationManager::StitchPath(const FVector& StartLocation, const FVector& EndLocation, AUKWayPoint* StartWayPoint, AUKWayPoint* EndWayPoint, const TArray<int32>& ClusterPath)
{
	TArray<AUKWayPoint*> FinalPath;
	if (!StartWayPoint || !EndWayPoint || ClusterPath.Num() < 2)
	{
		return FinalPath;
	}

	// Current segment exploration start Wei Point
	AUKWayPoint* CurrentOriginWayPoint = StartWayPoint;

	// Cluster path circuit (before the last cluster)
	for (int32 i = 0; i < ClusterPath.Num() - 1; ++i)
	{
		int32 CurrentClusterID = ClusterPath[i];
		int32 NextClusterID = ClusterPath[i + 1];


		FHPAEntrance BestEntrance;
		TArray<int32> BestPathIndices;
		// Within the current currency (currentClusterId), starting with a current WayPoint and finding the most expensive entrance and path to the next cluster (NextClusterId)
		if (!FindBestEntranceToNeighbor(CurrentOriginWayPoint, NextClusterID, BestEntrance, BestPathIndices))
		{
			UE_LOG(LogTemp, Error, TEXT("StitchPath: Failed to find path segment within Cluster %d towards Cluster %d"), CurrentClusterID, NextClusterID);
			return {};
		}

		// Add found path segments to final path (excluding duplicates)
		TArray<AUKWayPoint*> PathSegment = ConvertIndicesToWaypoints(BestPathIndices);
		if (!PathSegment.IsEmpty())
		{
			int32 StartIdx = 0; //(FinalPath.IsEmpty()) ? 0 : 1;
			for (int32 k = StartIdx; k < PathSegment.Num(); ++k)
			{
				FinalPath.Add(PathSegment[k]);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("StitchPath: Empty path segment found for Cluster %d -> %d."), CurrentClusterID, NextClusterID);
			return {};
		}

		// Update current waypoint for next loop (as entry point to next cluster)
		CurrentOriginWayPoint = BestEntrance.NeighborWaypoint.Get();
		if (!CurrentOriginWayPoint)
		{
			UE_LOG(LogTemp, Error, TEXT("StitchPath: Invalid next origin waypoint after cluster %d."), CurrentClusterID);
			return {};
		}
	}

	// Last cluster internal path search (CurrentOriginWayPoint -> EndWayPoint)
	int32 LastClusterID = ClusterPath.Last();
	TArray<int32> LastPathIndices;
	if (FindPathLowLevel(CurrentOriginWayPoint, EndWayPoint, LastClusterID, LastPathIndices))
	{
		TArray<AUKWayPoint*> LastPathSegment = ConvertIndicesToWaypoints(LastPathIndices);
		if (!LastPathSegment.IsEmpty())
		{
			int32 StartIdx = 0; //(FinalPath.IsEmpty() && LastPathSegment[0] == StartWayPoint) ? 0 : 1;
			if (FinalPath.Num() > 0 && FinalPath.Last() == LastPathSegment[0])
			{
				StartIdx = 1;
			}

			for (int32 k = StartIdx; k < LastPathSegment.Num(); ++k)
			{
				FinalPath.Add(LastPathSegment[k]);
			}
		}
		else
		{
			// If the last path is empty (e.g. start and end WayPoint are the same)
			if (CurrentOriginWayPoint == EndWayPoint && FinalPath.Last() != EndWayPoint)
			{
				FinalPath.Add(EndWayPoint);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("StitchPath: Empty final path segment found for Cluster %d."), LastClusterID);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StitchPath: Failed to find final path segment within Cluster %d."), LastClusterID);
		return {}; 
	}

	return FinalPath;
}


TArray<AUKWayPoint*> UUKNavigationManager::ConvertIndicesToWaypoints(const TArray<int32>& Indices) const
{
	TArray<AUKWayPoint*> Waypoints;
	Waypoints.Reserve(Indices.Num());
	for (int32 Index : Indices)
	{
		if (AllWaypoints.IsValidIndex(Index))
		{
			AUKWayPoint* WayPoint = AllWaypoints[Index].Get();
			if (WayPoint)
			{
				Waypoints.Add(WayPoint);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ConvertIndicesToWaypoints: Null waypoint ptr at index %d."), Index);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ConvertIndicesToWaypoints: Invalid index %d."), Index);
		}
	}
	return Waypoints;
}

bool UUKNavigationManager::FindBestEntranceToNeighbor(AUKWayPoint* CurrentOriginWayPoint, int32 NeighborClusterID, FHPAEntrance& OutBestEntrance, TArray<int32>& OutBestPathIndices)
{
	// Finding the optimal entrance and path to a neighboring cluster starting from the current WayPoint.
	if (!CurrentOriginWayPoint || NeighborClusterID == INDEX_NONE)
	{
		return false;
	}

	int32 CurrentClusterID = CurrentOriginWayPoint->ClusterID;
	const FHPACluster* CurrentClusterData = AbstractGraph.Clusters.Find(CurrentClusterID);
	if (!CurrentClusterData)
	{
		return false;
	}

	const TArray<FHPAEntrance>* EntrancesToNext = CurrentClusterData->Entrances.Find(NeighborClusterID);
	if (!EntrancesToNext || EntrancesToNext->IsEmpty())
	{
		return false;
	}

	float MinPathCost = TNumericLimits<float>::Max();
	bool bFoundPath = false;

	// LowLevel path search for all exit candidates
	for (const FHPAEntrance& CandidateEntrance : *EntrancesToNext)
	{
		AUKWayPoint* ExitCandidateWayPoint = CandidateEntrance.LocalWaypoint.Get();
		if (!ExitCandidateWayPoint)
		{
			continue;
		}

		TArray<int32> PathIndicesCandidate;
		// LowLevel Navigation: CurrentOriginWayPoint -> ExitCandidateWayPoint (all within CurrentClusterID)
		if (FindPathLowLevel(CurrentOriginWayPoint, ExitCandidateWayPoint, CurrentClusterID, PathIndicesCandidate))
		{
			// Calculating path cost (simple: sum of distances)
			float CurrentPathCost = 0.f;
			if (PathIndicesCandidate.Num() > 1)
			{
				for (int k = 0; k < PathIndicesCandidate.Num() - 1; ++k)
				{
					AUKWayPoint* P1 = AllWaypoints[PathIndicesCandidate[k]].Get();
					AUKWayPoint* P2 = AllWaypoints[PathIndicesCandidate[k + 1]].Get();
					if (P1 && P2)
					{
						CurrentPathCost += FVector::Dist(P1->GetActorLocation(), P2->GetActorLocation());
					}
				}
			}
			else if (CurrentOriginWayPoint == ExitCandidateWayPoint)
			{
				// Cost 0 if starting point and exit are the same and Cost in initial build
				// CurrentPathCost = 0.f;
				CurrentPathCost = CandidateEntrance.Cost;
			}

			float NeighborCost = 0.f;
			AUKWayPoint* NeighborWayPoint = CandidateEntrance.NeighborWaypoint.Get();
			if (NeighborWayPoint)
			{
				NeighborCost = FVector::Dist2D(NeighborWayPoint->GetActorLocation(), CurrentOriginWayPoint->GetActorLocation());
			}

			float TotalEstimatedCost = CurrentPathCost + NeighborCost * 0.8f;

			// Optimal route update
			if (TotalEstimatedCost < MinPathCost)
			{
				MinPathCost = CurrentPathCost;
				OutBestPathIndices = PathIndicesCandidate;
				// Save the best entrance information
				OutBestEntrance = CandidateEntrance;
				bFoundPath = true;
			}
		}
	}

	// true if at least one path was found
	return bFoundPath;
}


void UUKNavigationManager::DrawDebugHPA(float Duration) const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World || !AbstractGraph.bIsBuilt)
	{
		return;
	}

	TArray<FColor> ClusterColors = {
		FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Cyan, FColor::Magenta, FColor::Orange,
		FColor::Purple, FColor::Turquoise, FColor::Silver
	};
	int32 ColorIndex = 0;

	for (const auto& ClusterPair : AbstractGraph.Clusters)
	{
		const FHPACluster& Cluster = ClusterPair.Value;
		FColor CurrentColor = ClusterColors[ColorIndex % ClusterColors.Num()];
		ColorIndex++;

		for (const auto& WeakWayPoint : Cluster.Waypoints)
		{
			AUKWayPoint* WayPoint = WeakWayPoint.Get();
			if (!WayPoint)
			{
				continue;
			}

			DrawDebugBox(World, WayPoint->GetActorLocation(), FVector(15.f,15.f,15.f), CurrentColor, false, Duration, SDPG_Foreground, 1.f);
			FString DebugText = FString::Printf(TEXT("C:%d"), Cluster.ClusterID);
			DrawDebugString(World, WayPoint->GetActorLocation() + FVector(0, 0, 30), DebugText, nullptr, CurrentColor, Duration, false, 2);
			if (UTextRenderComponent* TextComp = WayPoint->GetComponentByClass<UTextRenderComponent>())
			{
				TextComp->SetText(FText::FromString(DebugText));
				TextComp->TextRenderColor = CurrentColor;
			}
			
			for (AUKWayPoint* NeighborWayPoint : WayPoint->PathPoints)
			{
				if (NeighborWayPoint && NeighborWayPoint->ClusterID == Cluster.ClusterID)
				{
					bool bIsMutual = NeighborWayPoint->PathPoints.Contains(WayPoint);
					FVector Direction = (NeighborWayPoint->GetActorLocation() - WayPoint->GetActorLocation()).GetSafeNormal();
					FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
					float LineOffset = bIsMutual ? 10.0f : 0.0f;

					DrawDebugLine(World, WayPoint->GetActorLocation() + RightVector * LineOffset, NeighborWayPoint->GetActorLocation() + RightVector * LineOffset, CurrentColor, false, Duration, SDPG_Foreground, 1.f);
				}
			}
		}

		for (const auto& EntrancePair : Cluster.Entrances)
		{
			const TArray<FHPAEntrance>& EntrancesList = EntrancePair.Value;
			for (const FHPAEntrance& Entrance : EntrancesList)
			{
				AUKWayPoint* LocalWayPoint = Entrance.LocalWaypoint.Get();
				AUKWayPoint* NeighborWayPoint = Entrance.NeighborWaypoint.Get();
				if (LocalWayPoint && NeighborWayPoint)
				{
					FVector Direction = (NeighborWayPoint->GetActorLocation() - LocalWayPoint->GetActorLocation()).GetSafeNormal();
					FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
					float LineOffset = 10.0f;
					FVector StartLocation = NeighborWayPoint->GetActorLocation() + RightVector * LineOffset;
					FVector EndLocation = LocalWayPoint->GetActorLocation() + RightVector * LineOffset;
					DrawDebugLine(World, StartLocation, EndLocation, FColor::White, false, Duration, SDPG_Foreground, 1.f);
				}
			}
		}
		
		DrawDebugBox(World, Cluster.CenterLocation, Cluster.Expansion, CurrentColor, false, Duration, SDPG_Foreground, 1.f);
	}

	// Hash Grid
	const TSet<FWayPointHashGrid2D::FCell>& AllCells = WaypointGrid.GetCells();
	for (auto It(AllCells.CreateConstIterator()); It; ++It)
	{
		FBox CellBounds = WaypointGrid.CalcCellBounds(FWayPointHashGrid2D::FCellLocation(It->X, It->Y, It->Level));
		DrawDebugBox(World, CellBounds.GetCenter(), CellBounds.GetExtent(), GColorList.GetFColorByIndex(It->Level), false, Duration, SDPG_Foreground);
	}
#endif
}

FWayPointHandle UUKNavigationManager::GenerateNewHandle()
{
	// Simple incremental example (mimicking FSmartObjectHandleFactory::CreateHandleForDynamicObject)
	// In practice, path hashing is recommended, like FSmartObjectHandleFactory::CreateHandleForComponent
	return FWayPointHandle(NextHandleID++);
}

FBox UUKNavigationManager::CalculateWayPointBounds(AUKWayPoint* WayPoint) const
{
	if (!IsValid(WayPoint))
	{
		return FBox(ForceInit);
	}

	const FVector Location = WayPoint->GetActorLocation();
	const FVector Extent(50.0f);
	return FBox(Location - Extent, Location + Extent);
}
