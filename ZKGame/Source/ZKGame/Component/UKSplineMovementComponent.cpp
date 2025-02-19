// Fill out your copyright notice in the Description page of Project Settings.


#include "UKSplineMovementComponent.h"

#include "AI/Patrol/UKPatrolPathManager.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"

UUKSplineMovementComponent::UUKSplineMovementComponent(const FObjectInitializer& ObjectInitializer)
    :Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    Spline = nullptr;
    bIsPaused = false;
}

void UUKSplineMovementComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UUKSplineMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    OnMovementFinished.Clear();
}

void UUKSplineMovementComponent::StartSplineMovement(const FName SplineName, const int32 StartIndex, const int32 EndIndex)
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        Finish(false);
        return;
    }
    
    if (!UUKPatrolPathManager::Get())
    {
        Finish(false);
        return;
    }
    
    const FPatrolSplineSearchResult PatrolSplineSearchResult = UUKPatrolPathManager::Get()->FindPatrolPathWithBoundsByNameAndLocation(Owner->GetActorLocation(), SplineName, StartIndex, EndIndex);
    if (!PatrolSplineSearchResult.Success)
    {
        Finish(false);
        return;
    }
    
    Spline = PatrolSplineSearchResult.SplineComponent;
    if (!Spline)
    {
        Finish(false);
        return;
    }
    
    CharacterMovement = Cast<UCharacterMovementComponent>(Owner->GetComponentByClass(UCharacterMovementComponent::StaticClass()));
    if (!CharacterMovement)
    {
        Finish(false);
        return;
    }

    EndLocation = PatrolSplineSearchResult.EndLocation;
    const FVector SplineStartLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(StartIndex, ESplineCoordinateSpace::World);
    CurrentDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineStartLocation, ESplineCoordinateSpace::World);

    const FVector SplineGoalLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(EndIndex, ESplineCoordinateSpace::World);
    GoalDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineGoalLocation, ESplineCoordinateSpace::World);

    /*const FVector OwnerLocation = Owner->GetActorLocation();
    const FVector CloseLocation = PatrolSplineSearchResult.SplineComponent->FindLocationClosestToWorldLocation(OwnerLocation, ESplineCoordinateSpace::World);
    const float OwnerAndSplineDistance = FVector::Distance(OwnerLocation, CloseLocation);
    const float DistanceFromActorToSpline = PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(Owner->GetActorLocation(), ESplineCoordinateSpace::World);
    const float SplineLength = PatrolSplineSearchResult.SplineComponent->GetSplineLength();
    const float NewMaxWalkSpeed = ((SplineLength - DistanceFromActorToSpline) + OwnerAndSplineDistance) / TripTime;
    PreMaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
    CharacterMovement->MaxWalkSpeed = NewMaxWalkSpeed;*/

    // 이동이 시작되었으므로 Tick 활성화
    bIsPaused = false;
    SetComponentTickEnabled(true);
}

void UUKSplineMovementComponent::PauseMovement()
{
    bIsPaused = true;
}

void UUKSplineMovementComponent::ResumeMovement()
{
    bIsPaused = false;
}

void UUKSplineMovementComponent::Finish(const bool bSucceeded)
{
    if (bSucceeded)
    {
        OnMovementFinished.Broadcast(ESplineMoveCompleteType::Finish);
    }
    else
    {
        OnMovementFinished.Broadcast(ESplineMoveCompleteType::Fail);
    }
}

void UUKSplineMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 스플라인이 설정되지 않았거나 이동이 일시 정지 중이면 업데이트하지 않음
    if (!Spline || bIsPaused)
    {
        return;
    }

    const FVector ActorLocation = GetOwner()->GetActorLocation();
    float NearestDistance = Spline->GetDistanceAlongSplineAtLocation(ActorLocation, ESplineCoordinateSpace::World);
    CurrentDistance = NearestDistance + (CharacterMovement->MaxWalkSpeed * DeltaTime);

    // Check if you reached the end of the spline
    if (CurrentDistance >= GoalDistance)
    {
        CurrentDistance = GoalDistance;
    }
	
    const float Dist2D = FVector::Dist2D(EndLocation, ActorLocation);
    if (Dist2D < 1.0f)
    {
        Finish();
        SetComponentTickEnabled(false);
        return;
    }
	
    const float SplineKey = Spline->GetInputKeyValueAtDistanceAlongSpline(CurrentDistance);
    const FVector NextSplineLocation = Spline->GetLocationAtSplineInputKey(SplineKey, ESplineCoordinateSpace::World);
    const FVector AddInputVector = NextSplineLocation - ActorLocation;
    CharacterMovement->AddInputVector(AddInputVector);
}
