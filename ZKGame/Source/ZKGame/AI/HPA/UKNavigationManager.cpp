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
		TArray<int32> ClusterPathIDs = {StartClusterID};
		// Path Stitching
		return FindPathInCluster(StartWayPoint, EndWayPoint, ClusterPathIDs);
	}
	// 3. Search for path between other clusters
	else
	{
		TArray<int32> ClusterPathIDs;
		if (FindPathCluster(StartClusterID, EndClusterID, ClusterPathIDs))
		{
			// Path Stitching
			return FindPathInCluster(StartWayPoint, EndWayPoint, ClusterPathIDs);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("High-Level pathfinding failed between Cluster %d and %d."), StartClusterID, EndClusterID);
			return {};
		}
	}
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

bool UUKNavigationManager::FindPathCluster(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath)
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

TArray<AUKWayPoint*> UUKNavigationManager::FindPathInCluster(AUKWayPoint* StartWayPoint, AUKWayPoint* EndWayPoint, const TArray<int32>& ClusterPath)
{
	TArray<AUKWayPoint*> FinalPath;
	if (!StartWayPoint || !EndWayPoint || ClusterPath.Num() < 1)
	{
		return FinalPath;
	}

	FWayPointGraph PathGraph;
	for (int32 i = 0; i < ClusterPath.Num(); ++i)
	{
		const int32 CurrentClusterID = ClusterPath[i];
		const FHPACluster* CurrentClusterData = AbstractGraph.Clusters.Find(CurrentClusterID);
		if (!CurrentClusterData)
		{
			continue;
		}

		PathGraph.Waypoints.Append(CurrentClusterData->Waypoints);
	}

	// Finding the indices of start and end waypoints in a graph
	int32 StartIndex = PathGraph.Waypoints.IndexOfByKey(StartWayPoint);
	int32 EndIndex = PathGraph.Waypoints.IndexOfByKey(EndWayPoint);
    
	if (StartIndex == INDEX_NONE || EndIndex == INDEX_NONE)
	{
		return FinalPath;
	}

	// A* Create a node
	FGraphAStarDefaultNode<FWayPointGraph> StartNode(StartIndex);
	FGraphAStarDefaultNode<FWayPointGraph> EndNode(EndIndex);

	// Filter settings
	FWayPointFilter QueryFilter;
	QueryFilter.Graph = &PathGraph;

	// A* Running the algorithm
	TArray<int32> OutPath;
	FGraphAStar<FWayPointGraph> Pathfinder(PathGraph);
	EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, QueryFilter, OutPath);

	if (Result == SearchSuccess)
	{
		FinalPath.Emplace(StartWayPoint);
		for (int32 Index : OutPath)
		{
			if (PathGraph.Waypoints.IsValidIndex(Index))
			{
				FinalPath.Add(PathGraph.Waypoints[Index].Get());
			}
		}
	}
	return FinalPath;
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
		
		DrawDebugBox(World, Cluster.CenterLocation, Cluster.Expansion, CurrentColor, false, Duration, SDPG_Foreground, 0.2f);
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