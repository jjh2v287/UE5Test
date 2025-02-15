// Fill out your copyright notice in the Description page of Project Settings.

#include "UKWaypoint.h"
#include "UKPathFindingSubsystem.h"

AUKWaypoint::AUKWaypoint(const FObjectInitializer& ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUKWaypoint::BeginPlay()
{
	Super::BeginPlay();
    if (UUKPathFindingSubsystem* PathFindingSubsystem = GetWorld()->GetSubsystem<UUKPathFindingSubsystem>())
    {
        PathFindingSubsystem->RegisterWaypoint(this);
    }
}

void AUKWaypoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_EDITOR
    DrawDebugLines();
#endif
}

#if WITH_EDITOR
void AUKWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        
        // NextPathPoints가 변경되었을 때
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AUKWaypoint, PathPoints))
        {
            for (AUKWaypoint* NextPoint : PathPoints)
            {
                if (NextPoint && !NextPoint->PathPoints.Contains(this))
                {
                    NextPoint->PathPoints.Add(this);
                    if (NextPoint->MarkPackageDirty())
                    {
                        UE_LOG(LogActor, Log, TEXT("%s PostEditChangeProperty"), *PropertyName.ToString());
                    }
                }
            }
        }
    }
}

void AUKWaypoint::DrawDebugLines()
{
    const FVector StartLocation = GetActorLocation();
    
    // 이전 경로 표시 (왼쪽으로 오프셋)
    for (const AUKWaypoint* Point : PathPoints)
    {
        if (Point)
        {
            FVector Direction = (Point->GetActorLocation() - StartLocation).GetSafeNormal();
            FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector);

            constexpr  float LineOffset = 10.0f; // 선 간격 조절값
            FVector OffsetStart = StartLocation - RightVector * LineOffset;
            FVector OffsetEnd = Point->GetActorLocation() - RightVector * LineOffset;
            
            DrawDebugDirectionalArrow(
                GetWorld(),
                OffsetStart,
                OffsetEnd,
                100.0f,
                FColor::Purple,
                false,
                -1.0f,
                0,
                1.0f
            );
        }
    }
}
#endif