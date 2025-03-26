#include "UKHPADebugActor.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKHPADebugActor)

AUKHPADebugActor::AUKHPADebugActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUKHPADebugActor::BeginPlay()
{
    Super::BeginPlay();
}

void AUKHPADebugActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    // 디버그 시각화
    if (UUKHPAManager::Get())
    {
        if (bDrawClusters)
        {
            UUKHPAManager::Get()->DebugDrawClusters();
        }
        
        if (bDrawGateways)
        {
            UUKHPAManager::Get()->DebugDrawGateways();
        }
        
        if (bDrawAbstractGraph)
        {
            UUKHPAManager::Get()->DebugDrawAbstractGraph();
        }
        
        if (bDrawTestPath && !CurrentPath.IsEmpty())
        {
            DrawTestPath();
        }
    }
#endif
}

void AUKHPADebugActor::BuildHPA(float ClusterSize)
{
    if (UUKHPAManager::Get())
    {
        // 웨이포인트 등록
        UUKNavigationManager::Get()->AllRegisterWaypoint();
        
        // HPA* 구축
        UUKHPAManager::Get()->BuildClusters(ClusterSize);
    }
}

void AUKHPADebugActor::TestHPAPath()
{
    CurrentPath.Empty();
    
    if (!StartActor || !EndActor || !UUKHPAManager::Get() || !UUKNavigationManager::Get())
    {
        return;
    }
    
    // 시작점과 끝점에서 가장 가까운 웨이포인트 찾기
    AUKWaypoint* StartWaypoint = UUKNavigationManager::Get()->FindNearestWaypoint(StartActor->GetActorLocation());
    AUKWaypoint* EndWaypoint = UUKNavigationManager::Get()->FindNearestWaypoint(EndActor->GetActorLocation());
    
    if (!StartWaypoint || !EndWaypoint)
    {
        return;
    }
    
    // HPA* 경로 찾기
    CurrentPath = UUKHPAManager::Get()->FindHierarchicalPath(StartWaypoint, EndWaypoint);
}

void AUKHPADebugActor::DrawTestPath()
{
    UWorld* World = GetWorld();
    if (!World || !StartActor || !EndActor || CurrentPath.IsEmpty())
    {
        return;
    }
    
    // 시작점과 끝점 표시
    DrawDebugSphere(
        World,
        StartActor->GetActorLocation(),
        DebugSphereRadius,
        16,
        TestPathColor,
        false,
        -1.0f,
        0,
        DebugThickness
    );
    
    DrawDebugSphere(
        World,
        EndActor->GetActorLocation(),
        DebugSphereRadius,
        16,
        TestPathColor,
        false,
        -1.0f,
        0,
        DebugThickness
    );
    
    // 시작점에서 첫 웨이포인트까지
    DrawDebugDirectionalArrow(
        World,
        StartActor->GetActorLocation(),
        CurrentPath[0]->GetActorLocation(),
        DebugArrowSize,
        TestPathColor,
        false,
        -1.0f,
        0,
        DebugThickness
    );
    
    // 웨이포인트 간 경로 표시
    for (int32 i = 0; i < CurrentPath.Num() - 1; ++i)
    {
        // 게이트웨이인 경우 다른 색상으로 표시
        FColor LineColor = TestPathColor;
        
        if (UUKHPAManager::Get() && 
           (UUKHPAManager::Get()->IsGateway(CurrentPath[i]) || UUKHPAManager::Get()->IsGateway(CurrentPath[i + 1])))
        {
            LineColor = FColor::Green;  // 게이트웨이 경로는 녹색으로 표시
        }
        
        DrawDebugDirectionalArrow(
            World,
            CurrentPath[i]->GetActorLocation(),
            CurrentPath[i + 1]->GetActorLocation(),
            DebugArrowSize,
            LineColor,
            false,
            -1.0f,
            0,
            DebugThickness
        );
        
        // 웨이포인트 표시 (게이트웨이는 다른 색상으로)
        FColor SphereColor = TestPathColor;
        
        if (UUKHPAManager::Get() && UUKHPAManager::Get()->IsGateway(CurrentPath[i]))
        {
            SphereColor = FColor::Green;
        }
        
        DrawDebugSphere(
            World,
            CurrentPath[i]->GetActorLocation(),
            DebugSphereRadius * 0.5f,
            8,
            SphereColor,
            false,
            -1.0f,
            0,
            DebugThickness
        );
    }
    
    // 마지막 웨이포인트 표시
    if (CurrentPath.Num() > 0)
    {
        FColor SphereColor = TestPathColor;
        
        if (UUKHPAManager::Get() && UUKHPAManager::Get()->IsGateway(CurrentPath.Last()))
        {
            SphereColor = FColor::Green;
        }
        
        DrawDebugSphere(
            World,
            CurrentPath.Last()->GetActorLocation(),
            DebugSphereRadius * 0.5f,
            8,
            SphereColor,
            false,
            -1.0f,
            0,
            DebugThickness
        );
        
        // 마지막 웨이포인트에서 목적지까지
        DrawDebugDirectionalArrow(
            World,
            CurrentPath.Last()->GetActorLocation(),
            EndActor->GetActorLocation(),
            DebugArrowSize,
            TestPathColor,
            false,
            -1.0f,
            0,
            DebugThickness
        );
    }
}