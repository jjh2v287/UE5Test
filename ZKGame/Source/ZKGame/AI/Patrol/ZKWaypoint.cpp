// Fill out your copyright notice in the Description page of Project Settings.


#include "ZKWaypoint.h"

#include "DrawDebugHelpers.h"

AZKWaypoint::AZKWaypoint(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    
    // 기본값 설정
    bShowDebugLines = true;
    PreviousPathColor = FColor::Red;
    NextPathColor = FColor::Green;
}

void AZKWaypoint::BeginPlay()
{
    Super::BeginPlay();
    ValidateConnections();
}

void AZKWaypoint::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    if (bShowDebugLines)
    {
        DrawDebugLines();
    }
#endif
}

AZKWaypoint* AZKWaypoint::GetRandomNextWaypoint() const
{
    if (NextPathPoints.Num() == 0)
    {
        return nullptr;
    }
    
    int32 RandomIndex = FMath::RandRange(0, NextPathPoints.Num() - 1);
    return NextPathPoints[RandomIndex];
}

AZKWaypoint* AZKWaypoint::GetRandomPreviousWaypoint() const
{
    if (PreviousPathPoints.Num() == 0)
    {
        return nullptr;
    }
    
    int32 RandomIndex = FMath::RandRange(0, PreviousPathPoints.Num() - 1);
    return PreviousPathPoints[RandomIndex];
}

bool AZKWaypoint::ValidateConnections()
{
    bool bIsValid = true;
    
    // 다음 경로 포인트들의 이전 경로에 현재 포인트가 포함되어 있는지 확인
    for (AZKWaypoint* NextPoint : NextPathPoints)
    {
        if (NextPoint && !NextPoint->PreviousPathPoints.Contains(this))
        {
            bIsValid = false;
            UE_LOG(LogTemp, Warning, TEXT("Waypoint %s is not properly connected with next point %s"),
                *GetName(), *NextPoint->GetName());
        }
    }
    
    // 이전 경로 포인트들의 다음 경로에 현재 포인트가 포함되어 있는지 확인
    for (AZKWaypoint* PrevPoint : PreviousPathPoints)
    {
        if (PrevPoint && !PrevPoint->NextPathPoints.Contains(this))
        {
            bIsValid = false;
            UE_LOG(LogTemp, Warning, TEXT("Waypoint %s is not properly connected with previous point %s"),
                *GetName(), *PrevPoint->GetName());
        }
    }
    
    return bIsValid;
}

#if WITH_EDITOR
void AZKWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        
        // NextPathPoints가 변경되었을 때
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AZKWaypoint, NextPathPoints))
        {
            for (AZKWaypoint* NextPoint : NextPathPoints)
            {
                if (NextPoint && !NextPoint->PreviousPathPoints.Contains(this))
                {
                    NextPoint->PreviousPathPoints.Add(this);
                    NextPoint->MarkPackageDirty();
                }
            }
        }
        // PreviousPathPoints가 변경되었을 때
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(AZKWaypoint, PreviousPathPoints))
        {
            for (AZKWaypoint* PrevPoint : PreviousPathPoints)
            {
                if (PrevPoint && !PrevPoint->NextPathPoints.Contains(this))
                {
                    PrevPoint->NextPathPoints.Add(this);
                    PrevPoint->MarkPackageDirty();
                }
            }
        }
    }
}

void AZKWaypoint::DrawDebugLines()
{
    const FVector StartLocation = GetActorLocation();
    const float LineOffset = 10.0f; // 선 간격 조절값
    
    // 이전 경로 표시 (왼쪽으로 오프셋)
    for (const AZKWaypoint* PrevPoint : PreviousPathPoints)
    {
        if (PrevPoint)
        {
            FVector Direction = (PrevPoint->GetActorLocation() - StartLocation).GetSafeNormal();
            FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector);
            
            FVector OffsetStart = StartLocation - RightVector * LineOffset;
            FVector OffsetEnd = PrevPoint->GetActorLocation() - RightVector * LineOffset;
            
            DrawDebugDirectionalArrow(
                GetWorld(),
                OffsetStart,
                OffsetEnd,
                100.0f,
                PreviousPathColor,
                false,
                -1.0f,
                0,
                2.0f
            );
        }
    }
    
    // 다음 경로 표시 (오른쪽으로 오프셋)
    for (const AZKWaypoint* NextPoint : NextPathPoints)
    {
        if (NextPoint)
        {
            FVector Direction = (NextPoint->GetActorLocation() - StartLocation).GetSafeNormal();
            FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector);
            
            FVector OffsetStart = StartLocation + RightVector * -LineOffset;
            FVector OffsetEnd = NextPoint->GetActorLocation() + RightVector * -LineOffset;
            
            DrawDebugDirectionalArrow(
                GetWorld(),
                OffsetStart,
                OffsetEnd,
                100.0f,
                NextPathColor,
                false,
                -1.0f,
                0,
                2.0f
            );
        }
    }
}
#endif