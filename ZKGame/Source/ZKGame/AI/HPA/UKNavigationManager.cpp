// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNavigationManager.h"

#include "AIController.h"
#include "UKWayPoint.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Components/TextRenderComponent.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Navigation/PathFollowingComponent.h"

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
	for (TPair<int32, FHPACluster>& ClusterPair : AbstractGraph.Clusters)
	{
		const int32 CurrentClusterID = ClusterPair.Key;
		FHPACluster& CurrentCluster = ClusterPair.Value;

		for (const TWeakObjectPtr<AUKWayPoint>& WeakWayPoint : CurrentCluster.Waypoints)
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
					for (const FHPAEntrance& ExistingEntrance : EntrancesList)
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
	for (TPair<int32, FHPACluster>& Pair : AbstractGraph.Clusters)
	{
		Pair.Value.CalculateCenter();
	}

	AbstractGraph.bIsBuilt = true;
}

TArray<FVector> UUKNavigationManager::FindPath(const FVector& StartLocation, const FVector& EndLocation)
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
		// UE_LOG(LogTemp, Warning, TEXT("No waypoints registered. Cannot find path."));
		return GetPathPointsFromStartToEnd(StartLocation, EndLocation);
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

	TArray<AUKWayPoint*> WayPoints; 
	// 2. Search for paths in the same cluster
	if (StartClusterID == EndClusterID)
	{
		TArray<int32> ClusterPathIDs = {StartClusterID};
		// Path Stitching
		WayPoints =  FindPathInCluster(StartWayPoint, EndWayPoint, ClusterPathIDs);
	}
	// 3. Search for path between other clusters
	else
	{
		TArray<int32> ClusterPathIDs;
		if (FindPathCluster(StartClusterID, EndClusterID, ClusterPathIDs))
		{
			// Path Stitching
			WayPoints = FindPathInCluster(StartWayPoint, EndWayPoint, ClusterPathIDs);
		}
	}

	// 4. NavMesh
	TArray<FVector> FinalWayPoint;
	if (WayPoints.Num() > 0)
	{
		TArray<FVector> NavNodes = GetPathPointsFromStartToEnd(StartLocation, WayPoints[0]->GetActorLocation());
		FinalWayPoint.Append(NavNodes);
	
		for (int32 i = 0; i < WayPoints.Num() - 1; i++)
		{
			const AUKWayPoint* WayPoint = WayPoints[i];
			const AUKWayPoint* NextWayPoint = WayPoints[i + 1];
			NavNodes = GetPathPointsFromStartToEnd(WayPoint->GetActorLocation(), NextWayPoint->GetActorLocation());
			FinalWayPoint.Append(NavNodes);
		}

		NavNodes = GetPathPointsFromStartToEnd(WayPoints.Last()->GetActorLocation(), EndLocation);
		FinalWayPoint.Append(NavNodes);
	}

	// 5. Curve Path
	TArray<FVector> CurvePath = GenerateCentripetalCatmullRomPath(FinalWayPoint, 4);
	
	return CurvePath;
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

void UUKNavigationManager::RequestMove(AActor* MoveOwner, const FVector GoalLocation)
{
	if (!UUKNavigationManager::Get() || !MoveOwner)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(MoveOwner->GetInstigatorController());
	if (!AIController)
	{
		return;
	}

	UPathFollowingComponent* PathFollowing = AIController->GetComponentByClass<UPathFollowingComponent>();
	if (!PathFollowing)
	{
		return;
	}

	FAIMoveRequest MoveReq(GoalLocation);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetRequireNavigableEndLocation(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetReachTestIncludesGoalRadius(true);
	MoveReq.SetAcceptanceRadius(5.0f);
	MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
	
	TArray<FVector> RawPath = UUKNavigationManager::Get()->FindPath(MoveOwner->GetActorLocation(), GoalLocation);
	FNavPathSharedPtr Path = MakeShareable(new FNavigationPath(RawPath, nullptr));
	PathFollowing->RequestMove(MoveReq, Path);
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

TArray<FVector> UUKNavigationManager::GetPathPointsFromStartToEnd(const FVector& StartPoint, const FVector& EndPoint, const float AgentRadius, const float AgentHeight) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return {};
	}
    
	// 내비게이션 시스템 가져오기
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSystem)
	{
		return {};
	}
    
	// 기본 내비게이션 데이터 가져오기
	const ANavigationData* NavData = NavSystem->GetDefaultNavDataInstance();
	if (!NavData)
	{
		return {};
	}

	FSharedConstNavQueryFilter NavQueryFilter = NavData->GetDefaultQueryFilter();
	if (NavQueryFilter->GetImplementation() == nullptr)
	{
		return {};
	}
	
	// 내비게이션 쿼리 필터 설정
	FNavAgentProperties NavAgentProps;
	NavAgentProps.AgentRadius = AgentRadius;
	NavAgentProps.AgentHeight = AgentHeight;
    
	// 경로 찾기 쿼리 생성
	FPathFindingQuery Query;
	Query.StartLocation = StartPoint;
	Query.EndLocation = EndPoint;
	Query.NavData = NavData;
	FSharedNavQueryFilter NavigationFilterCopy = NavQueryFilter->GetCopy();
	// NavigationFilterCopy->SetBacktrackingEnabled(true);
	// Query.QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, World, nullptr);
	Query.QueryFilter = NavigationFilterCopy;
	
	// 경로 계산 수행
	const FPathFindingResult Result = NavSystem->FindPathSync(NavAgentProps, Query);
    
	TArray<FVector> PathPoints;
	// 경로가 성공적으로 계산되었는지 확인
	if (Result.IsSuccessful() && Result.Path.IsValid())
	{
		// 경로 포인트 추출
		const TArray<FNavPathPoint>& NavPathPoints = Result.Path->GetPathPoints();
		for (const FNavPathPoint& Point : NavPathPoints)
		{
			PathPoints.Add(Point.Location);
		}

// #if WITH_EDITOR
// 		if (GIsEditor)
// 		{
// 			Result.Path->DebugDraw(NavData, FColor::Green, nullptr, false, -1.0f);
// 		}
// #endif
	}
    
	return PathPoints;
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

TArray<FVector> UUKNavigationManager::GenerateCentripetalCatmullRomPath(const TArray<FVector>& Points, int32 NumSamplesPerSegment /*= 4*/, float Alpha /*= 0.5f*/)
{
    TArray<FVector> InterpolatedPath;
    const int32 NumPoints = Points.Num();
    
    // 경로점이 2개 미만이면 원본 배열 반환
    if (NumPoints < 2)
    {
        return Points;
    }
    
    // 첫 번째 포인트는 항상 경로에 포함
    InterpolatedPath.Add(Points[0]);
    
    // 파라미터 값을 계산하는 람다 함수
    auto GetT = [](float T, float Alpha, const FVector& P0, const FVector& P1)
    {
        const FVector P1P0 = P1 - P0;
        const float Dot = P1P0 | P1P0; // 내적 연산
        const float Pow = FMath::Pow(Dot, Alpha * 0.5f);
        return Pow + T;
    };
    
    // 각 세그먼트에 대해 보간 수행
    for (int32 i = 0; i < NumPoints - 1; ++i)
    {
        // 4개의 제어점 가져오기 (경계 처리 포함)
        const FVector& P0 = Points[FMath::Max(i - 1, 0)];
        const FVector& P1 = Points[i];
        const FVector& P2 = Points[i + 1];
        const FVector& P3 = Points[FMath::Min(i + 2, NumPoints - 1)];
        
        // 파라미터 값 계산
        const float T0 = 0.0f;
        const float T1 = GetT(T0, Alpha, P0, P1);
        const float T2 = GetT(T1, Alpha, P1, P2);
        const float T3 = GetT(T2, Alpha, P2, P3);
        
        // 파라미터 간격 계산
        const float T1T0 = T1 - T0;
        const float T2T1 = T2 - T1;
        const float T3T2 = T3 - T2;
        const float T2T0 = T2 - T0;
        const float T3T1 = T3 - T1;
        
        // 0에 가까운 값 체크 (분모가 0이 되는 것 방지)
        const bool bIsNearlyZeroT1T0 = FMath::IsNearlyZero(T1T0, UE_KINDA_SMALL_NUMBER);
        const bool bIsNearlyZeroT2T1 = FMath::IsNearlyZero(T2T1, UE_KINDA_SMALL_NUMBER);
        const bool bIsNearlyZeroT3T2 = FMath::IsNearlyZero(T3T2, UE_KINDA_SMALL_NUMBER);
        const bool bIsNearlyZeroT2T0 = FMath::IsNearlyZero(T2T0, UE_KINDA_SMALL_NUMBER);
        const bool bIsNearlyZeroT3T1 = FMath::IsNearlyZero(T3T1, UE_KINDA_SMALL_NUMBER);
        
        // 첫 번째 포인트는 이미 추가되었으므로 다음 포인트부터 시작
        // i == 0인 경우를 제외하고는 세그먼트의 시작점도 추가
        if (i > 0)
        {
            InterpolatedPath.Add(P1);
        }
        
        // 세그먼트 내 보간점 생성
        for (int32 SampleIndex = 1; SampleIndex < NumSamplesPerSegment; ++SampleIndex)
        {
            // 파라메트릭 거리 (0.0 ~ 1.0)
            const float ParametricDistance = static_cast<float>(SampleIndex) / static_cast<float>(NumSamplesPerSegment - 1);
            
            // 현재 T 값 (T1과 T2 사이를 보간)
            const float T = FMath::Lerp(T1, T2, ParametricDistance);
            
            // De Boor 알고리즘을 사용한 보간 계산
            // 1단계 보간
            const FVector A1 = bIsNearlyZeroT1T0 ? P0 : (T1 - T) / T1T0 * P0 + (T - T0) / T1T0 * P1;
            const FVector A2 = bIsNearlyZeroT2T1 ? P1 : (T2 - T) / T2T1 * P1 + (T - T1) / T2T1 * P2;
            const FVector A3 = bIsNearlyZeroT3T2 ? P2 : (T3 - T) / T3T2 * P2 + (T - T2) / T3T2 * P3;
            
            // 2단계 보간
            const FVector B1 = bIsNearlyZeroT2T0 ? A1 : (T2 - T) / T2T0 * A1 + (T - T0) / T2T0 * A2;
            const FVector B2 = bIsNearlyZeroT3T1 ? A2 : (T3 - T) / T3T1 * A2 + (T - T1) / T3T1 * A3;
            
            // 최종 보간점
            const FVector InterpolatedPoint = bIsNearlyZeroT2T1 ? B1 : (T2 - T) / T2T1 * B1 + (T - T1) / T2T1 * B2;
            
            // 결과 배열에 보간된 점 추가
            InterpolatedPath.Add(InterpolatedPoint);
        }
    }
    
    // 마지막 점 추가
    InterpolatedPath.Add(Points[NumPoints - 1]);
    
    return InterpolatedPath;
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

	for (const TPair<int32, FHPACluster>& ClusterPair : AbstractGraph.Clusters)
	{
		const FHPACluster& Cluster = ClusterPair.Value;
		FColor CurrentColor = ClusterColors[ColorIndex % ClusterColors.Num()];
		ColorIndex++;

		for (const TWeakObjectPtr<AUKWayPoint>& WeakWayPoint : Cluster.Waypoints)
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

		for (const TPair<int32, TArray<FHPAEntrance>>& EntrancePair : Cluster.Entrances)
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
	for (const FWayPointHashGrid2D::FCell& Cell : AllCells)
	{
		FBox CellBounds = WaypointGrid.CalcCellBounds(FWayPointHashGrid2D::FCellLocation(Cell.X, Cell.Y, Cell.Level));
		DrawDebugBox(World, CellBounds.GetCenter(), CellBounds.GetExtent(), GColorList.GetFColorByIndex(Cell.Level), false, Duration, SDPG_Foreground);
	}
#endif
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