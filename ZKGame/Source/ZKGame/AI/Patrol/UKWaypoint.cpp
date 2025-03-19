// Fill out your copyright notice in the Description page of Project Settings.

#include "UKWaypoint.h"

#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "UKPathFindingSubsystem.h"

AUKWaypoint::AUKWaypoint(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;

    InvokerComponent = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));
}

void AUKWaypoint::BeginPlay()
{
    Super::BeginPlay();
    if (UUKPathFindingSubsystem* PathFindingSubsystem = GetWorld()->GetSubsystem<UUKPathFindingSubsystem>())
    {
        PathFindingSubsystem->RegisterWaypoint(this);
    }
}

void AUKWaypoint::Destroyed()
{
    Super::Destroyed();
}

void AUKWaypoint::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if WITH_EDITOR
    DrawDebugLines();
#endif
}

#if WITH_EDITOR
void AUKWaypoint::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);
    // 자신을 가리키는 다른 웨이포인트들의 스플라인도 업데이트
    for (AUKWaypoint* OtherPoint : TActorRange<AUKWaypoint>(GetWorld()))
    {
        if (OtherPoint && OtherPoint != this && OtherPoint->PathPoints.Contains(this))
        {
            // OtherPoint->UpdateConnectedSplines();
        }
    }
}

void AUKWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        
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