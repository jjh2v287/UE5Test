// Fill out your copyright notice in the Description page of Project Settings.

#include "UKWaypoint.h"
#include "EngineUtils.h"
#include "UKPathFindingSubsystem.h"

AUKWaypoint::AUKWaypoint(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
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

void AUKWaypoint::Destroyed()
{
    ClearSplinePaths();
    Super::Destroyed();
}

void AUKWaypoint::UpdateSplines()
{
    ClearSplinePaths();
    
    for (AUKWaypoint* Point : PathPoints)
    {
        if (Point)
        {
            CreateSplinePath(Point);
        }
    }
}

void AUKWaypoint::CreateSplinePath(AUKWaypoint* TargetPoint)
{
    if (!TargetPoint || !GetWorld()) return;

    FVector Direction = (TargetPoint->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    // FVector Direction = GetActorForwardVector();
    FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector);
    
    constexpr float LineOffset = 10.0f;
    FVector OffsetStart = GetActorLocation() - RightVector * LineOffset;
    FVector OffsetEnd = TargetPoint->GetActorLocation() - RightVector * LineOffset;

    // 스플라인 액터 스폰
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    
    AUKWaypointSpline* SplinePath = GetWorld()->SpawnActor<AUKWaypointSpline>(
        AUKWaypointSpline::StaticClass(),
        GetActorLocation(),
        GetActorRotation(),
        SpawnParams
    );

    if (SplinePath)
    {
        // 스플라인 초기화
        SplinePath->InitializeSpline(OffsetStart, OffsetEnd);
        
        // 웨이포인트의 자식으로 설정
        SplinePath->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        
        // 배열에 추가
        SplinePaths.Add(SplinePath);
    }
}

void AUKWaypoint::ClearSplinePaths()
{
    for (AUKWaypointSpline* SplinePath : SplinePaths)
    {
        if (SplinePath)
        {
            SplinePath->Destroy();
        }
    }
    SplinePaths.Empty();
}

void AUKWaypoint::UpdateConnectedSplines()
{
    for (int32 i = 0; i < PathPoints.Num(); ++i)
    {
        if (PathPoints[i] && i < SplinePaths.Num() && SplinePaths[i])
        {
            FVector Direction = (PathPoints[i]->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            // FVector Direction = GetActorForwardVector();
            FVector RightVector = FVector::CrossProduct(Direction, FVector::UpVector);
            
            constexpr float LineOffset = 10.0f;
            FVector OffsetStart = GetActorLocation() - RightVector * LineOffset;
            FVector OffsetEnd = PathPoints[i]->GetActorLocation() - RightVector * LineOffset;
            
            SplinePaths[i]->UpdateEndPoints(OffsetStart, OffsetEnd);
        }
    }
}

#if WITH_EDITOR
void AUKWaypoint::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);
    // 자신의 스플라인 업데이트
    UpdateConnectedSplines();

    // 자신을 가리키는 다른 웨이포인트들의 스플라인도 업데이트
    for (AUKWaypoint* OtherPoint : TActorRange<AUKWaypoint>(GetWorld()))
    {
        if (OtherPoint && OtherPoint != this && OtherPoint->PathPoints.Contains(this))
        {
            OtherPoint->UpdateConnectedSplines();
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
                    NextPoint->UpdateSplines();
                    if (NextPoint->MarkPackageDirty())
                    {
                        UE_LOG(LogActor, Log, TEXT("%s PostEditChangeProperty"), *PropertyName.ToString());
                    }
                }
            }
            UpdateSplines();
        }
    }
}
#endif