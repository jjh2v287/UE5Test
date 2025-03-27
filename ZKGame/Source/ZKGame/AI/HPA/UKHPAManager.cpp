// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKHPAManager.h"
#include "UKWayPoint.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKHPAManager)

UUKHPAManager* UUKHPAManager::Instance = nullptr;

void UUKHPAManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
	// 게임 시작 시 자동 빌드 고려
	// BuildHierarchy();
}

void UUKHPAManager::Deinitialize()
{
	AllWaypoints.Empty();
	WaypointToIndexMap.Empty();
	AbstractGraph.Clear();
	Instance = nullptr;
	Super::Deinitialize();
}

void UUKHPAManager::RegisterWaypoint(AUKWayPoint* Waypoint)
{
	if (IsValid(Waypoint) && !WaypointToIndexMap.Contains(Waypoint))
	{
		AllWaypoints.Add(Waypoint);
		WaypointToIndexMap.Add(Waypoint, AllWaypoints.Num() - 1);
		// 웨이포인트 추가 시 계층 구조 무효화
		AbstractGraph.bIsBuilt = false;
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
			
			// 맵에서 먼저 제거
			WaypointToIndexMap.Remove(Waypoint);

			if (AllWaypoints.IsValidIndex(IndexToRemove))
			{
				// 배열 끝 요소와 Swap 후 Pop (인덱스 재조정)
				if (IndexToRemove < AllWaypoints.Num() - 1)
				{
					AllWaypoints.Swap(IndexToRemove, AllWaypoints.Num() - 1);
					// 이동된 요소의 인덱스 업데이트
					AUKWayPoint* SwappedWP = AllWaypoints[IndexToRemove].Get();
					if (SwappedWP)
					{
						// 기존 맵 항목 제거 후 새로 추가 (혹시 모를 중복 방지)
						WaypointToIndexMap.Remove(SwappedWP);
						WaypointToIndexMap.Add(SwappedWP, IndexToRemove);
					}
				}
				// 마지막 요소 제거 (실제로는 Swap 후 마지막이 된 원래 요소를 제거)
				AllWaypoints.Pop(EAllowShrinking::No);
			}

			// 웨이포인트 제거 시 계층 구조 무효화
			AbstractGraph.bIsBuilt = false;
		}
	}
}


void UUKHPAManager::AllRegisterWaypoint()
{
	// 1. 기존 정보 클리어
	AllWaypoints.Empty();
	WaypointToIndexMap.Empty();
	AbstractGraph.Clear();

	// 2. 월드에서 모든 AUKWayPoint 액터 찾기
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundWaypoints;
	UGameplayStatics::GetAllActorsOfClass(World, AUKWayPoint::StaticClass(), FoundWaypoints);

	// 3. 찾은 액터를 캐스팅하여 등록
	AllWaypoints.Reserve(FoundWaypoints.Num());
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

	// 4. 계층 구조 빌드 (선택적 - 여기서 바로 빌드할지 결정)
	BuildHierarchy(true);
}


void UUKHPAManager::BuildHierarchy(bool bForceRebuild)
{
	if (AbstractGraph.bIsBuilt && !bForceRebuild)
	{
		UE_LOG(LogTemp, Log, TEXT("HPA Hierarchy already built. Skipping."));
		return;
	}

	AbstractGraph.Clear();

	// 1. 클러스터 생성 및 웨이포인트 할당
	for (int32 i = 0; i < AllWaypoints.Num(); ++i)
	{
		AUKWayPoint* WP = AllWaypoints[i].Get();
		if (!WP)
		{
			continue;
		}
		
		if (WP->ClusterID == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Waypoint %s has invalid ClusterID. Skipping."), *WP->GetName());
			continue;
		}

		FHPACluster& Cluster = AbstractGraph.Clusters.FindOrAdd(WP->ClusterID);
		if (Cluster.ClusterID == INDEX_NONE)
		{
			// 새로 추가된 경우 ID 설정
			Cluster.ClusterID = WP->ClusterID;
		}
		Cluster.Waypoints.Add(WP);
	}

	// 2. 클러스터 간 입구(Entrance) 식별
	for (auto& ClusterPair : AbstractGraph.Clusters)
	{
		const int32 CurrentClusterID = ClusterPair.Key;
		FHPACluster& CurrentCluster = ClusterPair.Value;

		for (const auto& WeakWP : CurrentCluster.Waypoints)
		{
			AUKWayPoint* LocalWP = WeakWP.Get();
			if (!LocalWP)
			{
				continue;
			}

			for (AUKWayPoint* NeighborWP : LocalWP->PathPoints)
			{
				// 직접 연결된 이웃 확인
				if (!NeighborWP)
				{
					continue;
				}

				// 이웃이 유효하고 다른 클러스터에 속하는지 확인
				if (NeighborWP->ClusterID != INDEX_NONE && NeighborWP->ClusterID != CurrentClusterID)
				{
					// 입구 발견
					int32 NeighborClusterID = NeighborWP->ClusterID;
					float Cost = FVector::Dist(LocalWP->GetActorLocation(), NeighborWP->GetActorLocation());

					// 현재 클러스터의 Entrances 맵에 추가
					TArray<FHPAEntrance>& EntrancesList = CurrentCluster.Entrances.FindOrAdd(NeighborClusterID);
					// 중복 입구 방지 (선택적) - 동일한 LocalWP, NeighborWP 쌍이 이미 있는지 확인
					bool bAlreadyExists = false;
					for (const auto& ExistingEntrance : EntrancesList)
					{
						if (ExistingEntrance.LocalWaypoint == LocalWP && ExistingEntrance.NeighborWaypoint == NeighborWP)
						{
							bAlreadyExists = true;
							break;
						}
					}
					
					if (!bAlreadyExists)
					{
						EntrancesList.Emplace(LocalWP, NeighborWP, NeighborClusterID, Cost);
					}
				}
			}
		}
	}

	// 3. 클러스터 중심 계산
	for (auto& Pair : AbstractGraph.Clusters)
	{
		Pair.Value.CalculateCenter();
	}

	AbstractGraph.bIsBuilt = true;

	// 디버그 드로잉 (옵션)
	DrawDebugHPA();
}

TArray<AUKWayPoint*> UUKHPAManager::FindPath(const FVector& StartLocation, const FVector& EndLocation)
{
	// 0. 계층 구조 빌드 확인
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


	// 1. 시작/끝점 근처 웨이포인트 찾기
	AUKWayPoint* StartWP = FindNearestWaypoint(StartLocation);
	AUKWayPoint* EndWP = FindNearestWaypoint(EndLocation);

	if (!StartWP || !EndWP)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find nearest waypoints for start or end location."));
		return {};
	}

	int32 StartClusterID = StartWP->ClusterID;
	int32 EndClusterID = EndWP->ClusterID;
	if (StartClusterID == INDEX_NONE || EndClusterID == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Start or End Waypoint has invalid ClusterID."));
		return {};
	}

	// 2. 같은 클러스터 내 경로 탐색
	if (StartClusterID == EndClusterID)
	{
		TArray<int32> PathIndices;
		if (FindPathLowLevel(StartWP, EndWP, StartClusterID, PathIndices))
		{
			// 시작점 -> 첫 WP, 마지막 WP -> 끝점 연결은 시각화 부분에서 처리
			return ConvertIndicesToWaypoints(PathIndices);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Low-Level pathfinding failed within Cluster %d."), StartClusterID);
			return {};
		}
	}
	// 3. 다른 클러스터 간 경로 탐색
	else
	{
		TArray<int32> ClusterPathIDs;
		if (FindPathHighLevel(StartClusterID, EndClusterID, ClusterPathIDs))
		{
			// 경로 스티칭
			return StitchPath(StartLocation, EndLocation, StartWP, EndWP, ClusterPathIDs);
		}
		else
		{
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
	if (PreferredClusterID != INDEX_NONE && AbstractGraph.Clusters.Contains(PreferredClusterID))
	{
		const FHPACluster& PreferredCluster = AbstractGraph.Clusters[PreferredClusterID];
		for (const auto& WeakWP : PreferredCluster.Waypoints)
		{
			if (AUKWayPoint* WP = WeakWP.Get())
			{
				float DistSq = FVector::DistSquared(Location, WP->GetActorLocation());
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					NearestWaypoint = WP;
					bFoundInPreferred = true;
				}
			}
		}
	}

	// 2. 선호 클러스터에서 못 찾았거나, 선호 ID가 없었으면 전체 웨이포인트 탐색
	if (!bFoundInPreferred)
	{
		MinDistSq = TNumericLimits<float>::Max(); // 최소 거리 초기화
		for (const auto& WeakWP : AllWaypoints)
		{
			if (AUKWayPoint* WP = WeakWP.Get())
			{
				float DistSq = FVector::DistSquared(Location, WP->GetActorLocation());
				if (DistSq < MinDistSq)
				{
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
	return NearestWP ? NearestWP->ClusterID : INDEX_NONE;
}


bool UUKHPAManager::FindPathLowLevel(AUKWayPoint* StartWP, AUKWayPoint* EndWP, int32 ClusterID, TArray<int32>& OutPathIndices)
{
	OutPathIndices.Empty();
	if (!StartWP || !EndWP || ClusterID == INDEX_NONE || !AbstractGraph.Clusters.Contains(ClusterID))
	{
		return false;
	}
	
	if (StartWP->ClusterID != ClusterID || EndWP->ClusterID != ClusterID)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPath_LowLevel: StartWP(%d) or EndWP(%d) not in target Cluster(%d)."), StartWP->ClusterID, EndWP->ClusterID, ClusterID);
		return false;
	}

	// A* 그래프 및 필터 생성
	FWayPointAStarGraph Graph(AllWaypoints, WaypointToIndexMap, ClusterID);
	FWayPointFilter Filter(Graph);

	// 시작/끝 노드 인덱스 찾기
	const int32* StartIndexPtr = WaypointToIndexMap.Find(StartWP);
	const int32* EndIndexPtr = WaypointToIndexMap.Find(EndWP);

	if (!StartIndexPtr || !EndIndexPtr)
	{
		UE_LOG(LogTemp, Error, TEXT("FindPath_LowLevel: Could not find index for StartWP or EndWP."));
		return false;
	}
	int32 StartIndex = *StartIndexPtr;
	int32 EndIndex = *EndIndexPtr;

	// A* 탐색 실행
	FGraphAStar<FWayPointAStarGraph> Pathfinder(Graph);
	FGraphAStarDefaultNode<FWayPointAStarGraph> StartNode(StartIndex);
	FGraphAStarDefaultNode<FWayPointAStarGraph> EndNode(EndIndex);

	EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, Filter, OutPathIndices);

	if (Result == EGraphAStarResult::SearchSuccess)
	{
		// 시작 노드 인덱스 추가
		OutPathIndices.Insert(StartIndex, 0);
		return true;
	}

	return false;
}

bool UUKHPAManager::FindPathHighLevel(int32 StartClusterID, int32 EndClusterID, TArray<int32>& OutClusterPath)
{
	OutClusterPath.Empty();
	if (StartClusterID == INDEX_NONE || EndClusterID == INDEX_NONE || !AbstractGraph.Clusters.Contains(StartClusterID) || !AbstractGraph.Clusters.Contains(EndClusterID))
	{
		return false;
	}

	// A* 그래프 및 필터 생성
	FClusterAStarGraph Graph(&AbstractGraph);
	FClusterFilter Filter(Graph, &AbstractGraph);

	// A* 탐색 실행
	FGraphAStar<FClusterAStarGraph> Pathfinder(Graph);
	FGraphAStarDefaultNode<FClusterAStarGraph> StartNode(StartClusterID);
	FGraphAStarDefaultNode<FClusterAStarGraph> EndNode(EndClusterID);

	TArray<int32> PathClusterIDs; // A* 결과 (시작 노드 미포함)
	EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, Filter, PathClusterIDs);

	if (Result == EGraphAStarResult::SearchSuccess)
	{
		OutClusterPath.Add(StartClusterID);
		OutClusterPath.Append(PathClusterIDs);
		return true;
	}
	
	return false;
}

TArray<AUKWayPoint*> UUKHPAManager::StitchPath(const FVector& StartLocation, const FVector& EndLocation, AUKWayPoint* StartWP, AUKWayPoint* EndWP, const TArray<int32>& ClusterPath) // ClusterPath: [StartCluster, C1, C2, ..., EndCluster]
{
	TArray<AUKWayPoint*> FinalPath;
	if (!StartWP || !EndWP || ClusterPath.Num() < 2)
	{
		return FinalPath;
	}

	// 현재 세그먼트 탐색 시작 웨이포인트
	AUKWayPoint* CurrentOriginWP = StartWP;

	// 클러스터 경로 순회 (마지막 클러스터 전까지)
	for (int32 i = 0; i < ClusterPath.Num() - 1; ++i)
	{
		int32 CurrentClusterID = ClusterPath[i];
		int32 NextClusterID = ClusterPath[i + 1];


		FHPAEntrance BestEntrance;
		TArray<int32> BestPathIndices;
		// 현재 클러스터(CurrentClusterID) 내에서, CurrentOriginWP 부터 시작하여
		// 다음 클러스터(NextClusterID)로 가는 가장 비용이 적은 입구와 경로 찾기
		if (!FindBestEntranceToNeighbor(CurrentOriginWP, NextClusterID, BestEntrance, BestPathIndices))
		{
			// 경로 찾기 실패
			UE_LOG(LogTemp, Error, TEXT("StitchPath: Failed to find path segment within Cluster %d towards Cluster %d"), CurrentClusterID, NextClusterID);
			return {};
		}

		// 찾은 경로 세그먼트를 최종 경로에 추가 (중복 제외)
		TArray<AUKWayPoint*> PathSegment = ConvertIndicesToWaypoints(BestPathIndices);
		if (!PathSegment.IsEmpty())
		{
			int32 StartIdx = 0; //(FinalPath.IsEmpty()) ? 0 : 1; // 첫 세그먼트 아니면 시작점 제외
			for (int32 k = StartIdx; k < PathSegment.Num(); ++k)
			{
				FinalPath.Add(PathSegment[k]);
			}
		}
		else
		{
			// 비정상 경로
			UE_LOG(LogTemp, Error, TEXT("StitchPath: Empty path segment found for Cluster %d -> %d."), CurrentClusterID, NextClusterID);
			return {};
		}

		// 다음 루프를 위해 현재 웨이포인트 업데이트 (다음 클러스터의 진입점으로)
		CurrentOriginWP = BestEntrance.NeighborWaypoint.Get();
		if (!CurrentOriginWP)
		{
			// 다음 시작점이 유효하지 않으면 실패
			UE_LOG(LogTemp, Error, TEXT("StitchPath: Invalid next origin waypoint after cluster %d."), CurrentClusterID);
			return {};
		}
	}

	// 마지막 클러스터 내부 경로 탐색 (CurrentOriginWP -> EndWP)
	int32 LastClusterID = ClusterPath.Last();
	TArray<int32> LastPathIndices;
	if (FindPathLowLevel(CurrentOriginWP, EndWP, LastClusterID, LastPathIndices))
	{
		TArray<AUKWayPoint*> LastPathSegment = ConvertIndicesToWaypoints(LastPathIndices);
		if (!LastPathSegment.IsEmpty())
		{
			int32 StartIdx = 0; //(FinalPath.IsEmpty() && LastPathSegment[0] == StartWP) ? 0 : 1; // 첫 세그먼트 아니면 시작점 제외
			if (FinalPath.Num() > 0 && FinalPath.Last() == LastPathSegment[0])
			{
				StartIdx = 1; // 더 확실한 중복 체크
			}

			for (int32 k = StartIdx; k < LastPathSegment.Num(); ++k)
			{
				FinalPath.Add(LastPathSegment[k]);
			}
		}
		else
		{
			// 마지막 경로가 비어있는 경우 (예: 시작과 끝 WP가 같음)
			if (CurrentOriginWP == EndWP && FinalPath.Last() != EndWP)
			{
				// 마지막 WP가 아직 추가 안됐으면 추가
				FinalPath.Add(EndWP);
			}
			else
			{
				// 경로가 아예 없는게 아니라면 여기서 실패 처리할 필요는 없을 수 있음
				UE_LOG(LogTemp, Warning, TEXT("StitchPath: Empty final path segment found for Cluster %d."), LastClusterID);
			}
		}
	}
	else
	{
		// 최종 경로 찾기 실패
		UE_LOG(LogTemp, Error, TEXT("StitchPath: Failed to find final path segment within Cluster %d."), LastClusterID);
		return {}; 
	}

	return FinalPath;
}


TArray<AUKWayPoint*> UUKHPAManager::ConvertIndicesToWaypoints(const TArray<int32>& Indices) const
{
	TArray<AUKWayPoint*> Waypoints;
	Waypoints.Reserve(Indices.Num());
	for (int32 Index : Indices)
	{
		if (AllWaypoints.IsValidIndex(Index))
		{
			AUKWayPoint* WP = AllWaypoints[Index].Get();
			if (WP)
			{
				Waypoints.Add(WP);
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

// 현재 WP에서 시작하여 이웃 클러스터로 가는 최적 입구 및 경로 찾기
bool UUKHPAManager::FindBestEntranceToNeighbor(AUKWayPoint* CurrentOriginWP, int32 NeighborClusterID, FHPAEntrance& OutBestEntrance, TArray<int32>& OutBestPathIndices)
{
	if (!CurrentOriginWP || NeighborClusterID == INDEX_NONE)
	{
		return false;
	}

	int32 CurrentClusterID = CurrentOriginWP->ClusterID;
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

	// 모든 출구 후보에 대해 LowLevel 경로 탐색
	for (const FHPAEntrance& CandidateEntrance : *EntrancesToNext)
	{
		AUKWayPoint* ExitCandidateWP = CandidateEntrance.LocalWaypoint.Get();
		if (!ExitCandidateWP)
		{
			continue;
		}

		TArray<int32> PathIndicesCandidate;
		// LowLevel 탐색: CurrentOriginWP -> ExitCandidateWP (모두 CurrentClusterID 내)
		if (FindPathLowLevel(CurrentOriginWP, ExitCandidateWP, CurrentClusterID, PathIndicesCandidate))
		{
			// 경로 비용 계산 (간단: 거리 합)
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
			else if (CurrentOriginWP == ExitCandidateWP)
			{
				// 시작점과 출구가 같으면 비용 0 및 초기 빌드시 Cost
				// CurrentPathCost = 0.f;
				CurrentPathCost = CandidateEntrance.Cost;
			}


			// 최적 경로 업데이트
			if (CurrentPathCost < MinPathCost)
			{
				MinPathCost = CurrentPathCost;
				OutBestPathIndices = PathIndicesCandidate;
				// 가장 좋았던 입구 정보 저장
				OutBestEntrance = CandidateEntrance;
				bFoundPath = true;
			}
		}
	}

	return bFoundPath; // 하나라도 경로를 찾았으면 true
}


void UUKHPAManager::DrawDebugHPA(float Duration) const
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

		// 클러스터 내 웨이포인트 및 연결 그리기
		for (const auto& WeakWP : Cluster.Waypoints)
		{
			AUKWayPoint* WP = WeakWP.Get();
			if (!WP)
			{
				continue;
			}

			DrawDebugBox(World, WP->GetActorLocation(), FVector(15.f,15.f,15.f), CurrentColor, false, Duration, 0, 1.f);
			DrawDebugString(World, WP->GetActorLocation() + FVector(0, 0, 30), FString::Printf(TEXT("C:%d"), Cluster.ClusterID), nullptr, CurrentColor, Duration, false, 2);

			for (AUKWayPoint* NeighborWP : WP->PathPoints)
			{
				if (NeighborWP && NeighborWP->ClusterID == Cluster.ClusterID)
				{
					// 같은 클러스터 내 연결
					// 양방향 시 오프셋
					bool bIsMutual = NeighborWP->PathPoints.Contains(WP);
					FVector Direction = (NeighborWP->GetActorLocation() - WP->GetActorLocation()).GetSafeNormal();
					FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
					float LineOffset = bIsMutual ? 10.0f : 0.0f;

					DrawDebugLine(World, WP->GetActorLocation() + RightVector * LineOffset, NeighborWP->GetActorLocation() + RightVector * LineOffset, CurrentColor, false, Duration, 0, 1.f);
				}
			}
		}

		// 클러스터 간 입구 그리기 (굵은 흰색 선)
		for (const auto& EntrancePair : Cluster.Entrances)
		{
			const TArray<FHPAEntrance>& EntrancesList = EntrancePair.Value;
			for (const FHPAEntrance& Entrance : EntrancesList)
			{
				AUKWayPoint* LocalWP = Entrance.LocalWaypoint.Get();
				AUKWayPoint* NeighborWP = Entrance.NeighborWaypoint.Get();
				if (LocalWP && NeighborWP)
				{
					FVector Direction = (NeighborWP->GetActorLocation() - LocalWP->GetActorLocation()).GetSafeNormal();
					FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
					float LineOffset = 10.0f;
					FVector StartLocation = NeighborWP->GetActorLocation() + RightVector * LineOffset;
					FVector EndLocation = LocalWP->GetActorLocation() + RightVector * LineOffset;
					DrawDebugLine(World, StartLocation, EndLocation, FColor::White, false, Duration, SDPG_Foreground, 1.f); // 굵게, 앞에 보이게
				}
			}
		}
		// 클러스터 중심점
		// DrawDebugSphere(World, Cluster.CenterLocation, 25.f, 8, FColor::Black, false, Duration, 0, 3.f);
	}
#endif
}
