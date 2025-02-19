// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAsyncTask_SplineMove2.h"
#include "Engine/World.h"

UUKAsyncTask_SplineMove2::UUKAsyncTask_SplineMove2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	StartIndex(0),
	EndIndex(0)
{
}

UUKAsyncTask_SplineMove2* UUKAsyncTask_SplineMove2::SplineMove2(AActor* Owner, const FName SplineName, const int32 StartIndex, const int32 EndIndex)
{
	UUKAsyncTask_SplineMove2* AsyncNode = NewObject<UUKAsyncTask_SplineMove2>();

	AsyncNode->Owner = Owner;
	AsyncNode->SplineName = SplineName;
	AsyncNode->StartIndex = StartIndex;
	AsyncNode->EndIndex = EndIndex;
	AsyncNode->CharacterMovement = Cast<UCharacterMovementComponent>(Owner->GetComponentByClass(UCharacterMovementComponent::StaticClass()));
	
	//  Call to globally register this object with a game instance, it will not be destroyed until SetReadyToDestroy is called
	if (Owner)
	{
		AsyncNode->RegisterWithGameInstance(Owner);
	}
	else
	{
		const UWorld* World = AsyncNode->GetWorld();
		AsyncNode->RegisterWithGameInstance(World);
	}

	// return this node
	return AsyncNode;
}

void UUKAsyncTask_SplineMove2::Activate()
{
	Super::Activate();

	if (!Owner)
	{
		FinishTask(false);
		return;
	}

	if (!CharacterMovement)
	{
		FinishTask(false);
		return;
	}
	
	if (!UUKPatrolPathSubsystem::Get())
	{
		FinishTask(false);
		return;
	}

	PatrolSplineSearchResult = UUKPatrolPathSubsystem::Get()->FindPatrolPathWithBoundsByTagAndLocation(FVector::ZeroVector, SplineName, StartIndex, EndIndex);
	if (!PatrolSplineSearchResult.Success)
	{
		FinishTask(false);
		return;
	}

	const FVector SplineStartLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(StartIndex, ESplineCoordinateSpace::World);
	CurrentDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineStartLocation, ESplineCoordinateSpace::World);

	const FVector SplineGoalLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(EndIndex, ESplineCoordinateSpace::World);
	GoalDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineGoalLocation, ESplineCoordinateSpace::World);
}

void UUKAsyncTask_SplineMove2::FinishTask(const bool bSucceeded)
{
	if (bSucceeded)
	{
		OnMoveFinish.Broadcast();
	}
	else
	{
		OnMoveFail.Broadcast();
	}
	bTickable = false;
	Cancel();
}

void UUKAsyncTask_SplineMove2::Cancel()
{
	OnMoveFinish.Clear();
	OnMoveFail.Clear();
	Super::Cancel();
}

void UUKAsyncTask_SplineMove2::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float CurrentSplineKey = PatrolSplineSearchResult.SplineComponent->GetInputKeyValueAtDistanceAlongSpline(CurrentDistance);
	const FVector CurrentSplineLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(CurrentSplineKey, ESplineCoordinateSpace::World);
	const FVector CurrentActorLocation = Owner->GetActorLocation();
	const float Distance = FVector::Dist2D(CurrentSplineLocation, CurrentActorLocation);
	if (Distance < 10.0f)
	{
		CurrentDistance += CharacterMovement->MaxWalkSpeed * DeltaTime;
	}
	
	const FVector ActorLocation = Owner->GetActorLocation();
	const float SplineKey = PatrolSplineSearchResult.SplineComponent->GetInputKeyValueAtDistanceAlongSpline(CurrentDistance);
	const FVector NextSplineLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(SplineKey, ESplineCoordinateSpace::World);
	const FVector AddInputVector = NextSplineLocation - ActorLocation;
	CharacterMovement->AddInputVector(AddInputVector);

	if (CurrentDistance > GoalDistance)
	{
		CurrentDistance = GoalDistance;
		FinishTask(true);
	}
}
