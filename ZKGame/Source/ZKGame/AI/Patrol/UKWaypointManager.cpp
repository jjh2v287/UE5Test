// Fill out your copyright notice in the Description page of Project Settings.

#include "UKWaypointManager.h"
#include "GraphAStar.h"
#include "Kismet/GameplayStatics.h"

AUKWaypointManager* AUKWaypointManager::SingletonInstance = nullptr;

AUKWaypointManager::AUKWaypointManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUKWaypointManager::BeginPlay()
{
    Super::BeginPlay();
    if (SingletonInstance != nullptr)
    {
        // 이미 인스턴스가 존재하면 중복 생성된 액터는 소멸
        UE_LOG(LogTemp, Warning, TEXT("싱글턴 인스턴스가 이미 존재합니다! 중복 액터를 제거합니다."));
        Destroy();
    }
    else
    {
        SingletonInstance = this;
    }
    
    TArray<AActor*> AllWaypoint;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUKWaypoint::StaticClass(), AllWaypoint);
    AllWaypoints.Append(AllWaypoint);
}

void AUKWaypointManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    SingletonInstance = nullptr;
}

AUKWaypointManager* AUKWaypointManager::Get()
{
    return SingletonInstance;
}

void AUKWaypointManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!StartActor || !EndActor)
    {
        return;
    }

#if WITH_EDITOR
    if (AllWaypoints.IsEmpty())
    {
        TArray<AActor*> AllWaypoint;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUKWaypoint::StaticClass(), AllWaypoint);
        AllWaypoints.Append(AllWaypoint);
    }
#endif
    AUKWaypoint* StartWaypoint= FindNearestWaypoint(StartActor->GetActorLocation());
    AUKWaypoint* EndWaypoint= FindNearestWaypoint(EndActor->GetActorLocation());
    TArray<AUKWaypoint*> FinalWaypoints = {StartWaypoint};
    TArray<AUKWaypoint*> FindWaypoints = FindPath(StartWaypoint, EndWaypoint);

    // 상호작용 지점 표시
    DrawDebugSphere(
        GetWorld(),
        StartActor->GetActorLocation(),
        30.0f,
        16,
        FColor::Blue,
        false,
        0.01f,
        0,
        0.5f
    );

    FinalWaypoints.Append(FindWaypoints);
    // 상호작용 지점에서 첫 번째 웨이포인트까지 선 그리기
    if (FinalWaypoints.Num() > 1)
    {
        for (int32 i=0; i<FinalWaypoints.Num()-1; i++)
        {
            FVector StartVec =  FinalWaypoints[i]->GetActorLocation();
            FVector EndVec =  FinalWaypoints[i+1]->GetActorLocation();
            DrawDebugDirectionalArrow(
                GetWorld(),
                StartVec,
                EndVec,
                30.f,
                FColor::Blue,
                false,
                0.01f,
                0,
                1.0f
            );
        }
    }

    // 상호작용 지점 표시
    DrawDebugSphere(
        GetWorld(),
        EndActor->GetActorLocation(),
        30.0f,
        16,
        FColor::Red,
        false,
        0.01f,
        0,
        0.5f
    );
}

AUKWaypoint* AUKWaypointManager::FindNearestWaypoint(const FVector& Location)
{
    // 입력 검증
    if (AllWaypoints.Num() == 0)
    {
        return nullptr;
    }

    AUKWaypoint* NearestWaypoint = nullptr;
    float NearestDistanceSq = TNumericLimits<float>::Max();

    // 모든 웨이포인트를 순회하며 가장 가까운 지점 찾기
    for (int32 i=0; i<AllWaypoints.Num(); i++)
    {
        AUKWaypoint* Waypoint = Cast<AUKWaypoint>(AllWaypoints[i]);
        if (!IsValid(Waypoint))
        {
            continue;
        }

        // 거리 계산 (제곱 거리 사용으로 sqrt 연산 최소화)
        const float DistanceSq = FVector::DistSquared(Location, Waypoint->GetActorLocation());
        
        // 현재까지의 최단거리보다 가깝고, 웨이포인트의 영향 반경 내에 있는 경우
        if (DistanceSq < NearestDistanceSq)
        {
            NearestDistanceSq = DistanceSq;
            NearestWaypoint = Waypoint;
        }
    }

    return NearestWaypoint;
}

TArray<AUKWaypoint*> AUKWaypointManager::FindPath(AUKWaypoint* Start, AUKWaypoint* End)
{
    // 1. FWaypointGraph 구성
    FWaypointGraph WaypointGraph;
    for (int32 i=0; i<AllWaypoints.Num(); i++)
    {
        WaypointGraph.Waypoints.Emplace(Cast<AUKWaypoint>(AllWaypoints[i]));
    }

    // 2. 시작과 끝 웨이포인트의 인덱스 검색
    int32 StartIndex = WaypointGraph.Waypoints.IndexOfByKey(Start);
    int32 EndIndex   = WaypointGraph.Waypoints.IndexOfByKey(End);
    if (StartIndex == INDEX_NONE || EndIndex == INDEX_NONE)
    {
        // UE_LOG(LogTemp, Warning, TEXT("시작 또는 끝 웨이포인트가 그래프에 존재하지 않습니다."));
        return TArray<AUKWaypoint*>();
    }

    // 3. A* 기본 노드 생성 (FGraphAStarDefaultNode는 GraphAStar.h에 정의되어 있음)
    FGraphAStarDefaultNode<FWaypointGraph> StartNode(StartIndex);
    FGraphAStarDefaultNode<FWaypointGraph> EndNode(EndIndex);

    // 4. 간단한 Query Filter 구현 (휴리스틱 0, 이동 비용 1 등으로 단순화)
    FWaypointFilter QueryFilter;

    // 5. FGraphAStar 객체 생성 (FWaypointGraph를 기반으로)
    FGraphAStar<FWaypointGraph> Pathfinder(WaypointGraph);

    // 6. A* 탐색 실행 (OutPath는 노드 인덱스 배열)
    TArray<int32> OutPath;
    EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, QueryFilter, OutPath);

    // 7. 결과 출력
    if (Result == SearchSuccess)
    {
        // UE_LOG(LogTemp, Log, TEXT("경로 탐색 성공! 경로 길이: %d"), OutPath.Num());
        TArray<AUKWaypoint*> Waypoints;
        for (int32 i : OutPath)
        {
            Waypoints.Emplace(WaypointGraph.Waypoints[i]);
        }
        return Waypoints;
    }
    
    // UE_LOG(LogTemp, Warning, TEXT("경로를 찾지 못했습니다."));
    return TArray<AUKWaypoint*>();
}
