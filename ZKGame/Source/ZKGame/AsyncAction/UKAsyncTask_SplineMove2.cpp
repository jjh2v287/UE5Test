// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAsyncTask_SplineMove2.h"

#include "AI/Patrol/UKPatrolPathManager.h"
#include "Engine/World.h"

UUKAsyncTask_SplineMove2::UUKAsyncTask_SplineMove2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	StartIndex(0),
	EndIndex(0)
{
}

UUKAsyncTask_SplineMove2* UUKAsyncTask_SplineMove2::SplineMove2(AActor* Owner, const FName SplineName, const int32 StartIndex, const int32 EndIndex, const float TripTime)
{
	UUKAsyncTask_SplineMove2* AsyncNode = NewObject<UUKAsyncTask_SplineMove2>();

	AsyncNode->Owner = Owner;
	AsyncNode->SplineName = SplineName;
	AsyncNode->StartIndex = StartIndex;
	AsyncNode->EndIndex = EndIndex;
	AsyncNode->TripTime = TripTime;
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
	
	if (!UUKPatrolPathManager::Get())
	{
		FinishTask(false);
		return;
	}

	PatrolSplineSearchResult = UUKPatrolPathManager::Get()->FindPatrolPathWithBoundsByNameAndLocation(FVector::ZeroVector, SplineName, StartIndex, EndIndex);
	if (!PatrolSplineSearchResult.Success)
	{
		FinishTask(false);
		return;
	}

	const FVector SplineStartLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(StartIndex, ESplineCoordinateSpace::World);
	CurrentDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineStartLocation, ESplineCoordinateSpace::World);

	const FVector SplineGoalLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(EndIndex, ESplineCoordinateSpace::World);
	GoalDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineGoalLocation, ESplineCoordinateSpace::World);

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector CloseLocation = PatrolSplineSearchResult.SplineComponent->FindLocationClosestToWorldLocation(OwnerLocation, ESplineCoordinateSpace::World);
	const float OwnerAndSplineDistance = FVector::Distance(OwnerLocation, CloseLocation);
	const float DistanceFromSctorToSpline = PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(Owner->GetActorLocation(), ESplineCoordinateSpace::World);
	const float SplineLength = PatrolSplineSearchResult.SplineComponent->GetSplineLength();
	const float NewMaxWalkSpeed = ((SplineLength - DistanceFromSctorToSpline) + OwnerAndSplineDistance) / TripTime;
	PreMaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
	CharacterMovement->MaxWalkSpeed = NewMaxWalkSpeed;
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
	if (CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed = PreMaxWalkSpeed;
	}
	OnMoveFinish.Clear();
	OnMoveFail.Clear();
	Super::Cancel();
}

void UUKAsyncTask_SplineMove2::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector ActorLocation = Owner->GetActorLocation();
	float NearestDistance = PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(ActorLocation, ESplineCoordinateSpace::World);
	CurrentDistance = NearestDistance + (CharacterMovement->MaxWalkSpeed * DeltaTime);

	// Check if you reached the end of the spline
	if (CurrentDistance >= GoalDistance)
	{
		CurrentDistance = GoalDistance;
	}
	
	const float Dist2D = FVector::Dist2D(PatrolSplineSearchResult.EndLocation, ActorLocation);
	if (Dist2D < 1.0f)
	{
		FinishTask(true);
		return;
	}
	
	const float SplineKey = PatrolSplineSearchResult.SplineComponent->GetInputKeyValueAtDistanceAlongSpline(CurrentDistance);
	const FVector NextSplineLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(SplineKey, ESplineCoordinateSpace::World);
	const FVector AddInputVector = NextSplineLocation - ActorLocation;
	CharacterMovement->AddInputVector(AddInputVector);
}
