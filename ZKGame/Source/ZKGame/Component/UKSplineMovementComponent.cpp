// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKSplineMovementComponent.h"

#include "DrawDebugHelpers.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AI/Patrol/UKPatrolPathManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKSplineMovementComponent)

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

bool UUKSplineMovementComponent::StartSplineMovement(const FName SplineName, const int32 StartIndex, const int32 EndIndex, const float MovingTimeSecond /*= 0.0f*/)
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        Finish(false);
        return false;
    }
    
    if (!UUKPatrolPathManager::Get())
    {
        Finish(false);
        return false;
    }
    
    const FPatrolSplineSearchResult PatrolSplineSearchResult = UUKPatrolPathManager::Get()->FindPatrolPathWithBoundsByNameAndLocation(Owner->GetActorLocation(), SplineName, StartIndex, EndIndex);
    if (!PatrolSplineSearchResult.Success)
    {
   //  	if (const AUKAICharacter* AIOwner = Cast<AUKAICharacter>(Owner))
   //  	{
			// UE_LOG(LogTemp, Error, TEXT("[NPCData] NPC[%s] is connected to an invalid spline[%s]"), *AIOwner->GetMonsterData()->DataRowName.ToString(), *SplineName.ToString());	
   //  	}
        Finish(false);
        return false;
    }
    
    Spline = PatrolSplineSearchResult.SplineComponent;
    if (!Spline)
    {
        Finish(false);
        return false;
    }
    
    CharacterMovement = Cast<UCharacterMovementComponent>(Owner->GetComponentByClass(UCharacterMovementComponent::StaticClass()));
    if (!CharacterMovement)
    {
        Finish(false);
        return false;
    }

    EndLocation = PatrolSplineSearchResult.EndLocation;
    const FVector SplineStartLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(StartIndex, ESplineCoordinateSpace::World);
	StartDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineStartLocation, ESplineCoordinateSpace::World);
	CurrentDistance =  StartDistance;

	
    const FVector SplineGoalLocation = PatrolSplineSearchResult.SplineComponent->GetLocationAtSplineInputKey(EndIndex, ESplineCoordinateSpace::World);
    GoalDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(SplineGoalLocation, ESplineCoordinateSpace::World);

	Direction = 1.0f;
    if ((EndIndex - StartIndex) < 0)
    {
    	Direction = -1.0f;
    }
	
	// Speed calculation over time
	if (!FMath::IsNearlyZero(MovingTimeSecond))
	{
		const FVector OwnerLocation = Owner->GetActorLocation();
		const FVector CloseLocation = PatrolSplineSearchResult.SplineComponent->FindLocationClosestToWorldLocation(OwnerLocation, ESplineCoordinateSpace::World);
		const float OwnerAndSplineDistance = FVector::Distance(OwnerLocation, CloseLocation);
		const float DistanceFromActorToSpline = PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(Owner->GetActorLocation(), ESplineCoordinateSpace::World);
		const float SplineLength = PatrolSplineSearchResult.SplineComponent->GetSplineLength();
		NewMaxWalkSpeed = ((SplineLength - DistanceFromActorToSpline) + OwnerAndSplineDistance) / MovingTimeSecond;
		PreMaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
		CharacterMovement->MaxWalkSpeed = NewMaxWalkSpeed;
	}

    // Tick activation because the movement began
	bIsInfinity = false;
    bIsPaused = false;
	bIsMoving = true;
    SetComponentTickEnabled(true);
	return true;
}

bool UUKSplineMovementComponent::StartSplineInfinityMovement(const FName SplineName, const float MovingTimeSecond)
{
	const AActor* Owner = GetOwner();
    if (!Owner)
    {
        Finish(false);
        return false;
    }
    
    if (!UUKPatrolPathManager::Get())
    {
        Finish(false);
        return false;
    }
    
    const FPatrolSplineSearchResult PatrolSplineSearchResult = UUKPatrolPathManager::Get()->FindClosePatrolPathByName(Owner->GetActorLocation(), SplineName);
    if (!PatrolSplineSearchResult.Success)
    {
        Finish(false);
        return false;
    }
    
    Spline = PatrolSplineSearchResult.SplineComponent;
    if (!Spline)
    {
        Finish(false);
        return false;
    }
    
    CharacterMovement = Cast<UCharacterMovementComponent>(Owner->GetComponentByClass(UCharacterMovementComponent::StaticClass()));
    if (!CharacterMovement)
    {
        Finish(false);
        return false;
    }

    GoalDistance =  0.0f;
    EndLocation = FVector::ZeroVector;
    CurrentDistance =  PatrolSplineSearchResult.SplineComponent->GetDistanceAlongSplineAtLocation(Owner->GetActorLocation(), ESplineCoordinateSpace::World);

	// Speed calculation over time
	if (!FMath::IsNearlyZero(MovingTimeSecond))
	{
		const float SplineLength = PatrolSplineSearchResult.SplineComponent->GetSplineLength();
		NewMaxWalkSpeed = SplineLength / MovingTimeSecond;
		PreMaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
		CharacterMovement->MaxWalkSpeed = NewMaxWalkSpeed;
	}

    // Tick activation because the movement began
	Direction = 1.0f;
	bIsInfinity = true;
    bIsPaused = false;
	bIsMoving = true;
    SetComponentTickEnabled(true);
	return true;
}

void UUKSplineMovementComponent::PauseMovement()
{
    bIsPaused = true;
	if (!FMath::IsNearlyZero(NewMaxWalkSpeed) && CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed = PreMaxWalkSpeed;
	}
}

void UUKSplineMovementComponent::ResumeMovement()
{
    bIsPaused = false;
	if (!FMath::IsNearlyZero(NewMaxWalkSpeed) && CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed = NewMaxWalkSpeed;
	}
}

void UUKSplineMovementComponent::Finish(const bool bSucceeded)
{
	bIsMoving = false;
	SetComponentTickEnabled(false);
	
    if (bSucceeded)
    {
        OnMovementFinished.Broadcast(ESplineMoveCompleteType::Finish);
    }
    else
    {
        OnMovementFinished.Broadcast(ESplineMoveCompleteType::Fail);
    }
	
	//Returns the speed calculation over time
	if (!FMath::IsNearlyZero(NewMaxWalkSpeed) && CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed = PreMaxWalkSpeed;
		NewMaxWalkSpeed = 0.0f;
	}
}

void UUKSplineMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // If the spline is not set or the movement is suspended, it will not be updated.
    if (!Spline || bIsPaused)
    {
        return;
    }

	if (!bIsInfinity)
    {
		const float MaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
		const FVector ActorLocation = GetOwner()->GetActorLocation();
		const float NearestDistance = Spline->GetDistanceAlongSplineAtLocation(ActorLocation, ESplineCoordinateSpace::World);
		const FVector SplineDirection = Spline->GetDirectionAtDistanceAlongSpline(NearestDistance, ESplineCoordinateSpace::World);
		const FVector Spline2DDirection = FVector(SplineDirection.X, SplineDirection.Y, 0.0f) * Direction;
		CurrentDistance = NearestDistance + (MaxWalkSpeed * DeltaTime * Direction);

		// Check if you reached the end of the spline
		if (Direction > 0.0f && CurrentDistance >= GoalDistance)
		{
			CurrentDistance = GoalDistance;
			Finish();
			return;
		}
		else if (Direction < 0.0f && CurrentDistance <= GoalDistance)
		{
			CurrentDistance = GoalDistance;
			Finish();
			return;
		}
	
		if (FVector::Dist2D(EndLocation, ActorLocation) < 1.0f)
		{
			Finish();
			return;
		}
	
		PreDelta2DDirection = CurrentDelta2DDirection;
		const FVector NextSplineLocation = Spline->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
		const FVector DeltaDirection = (NextSplineLocation - ActorLocation);
		FVector NewCurrentDelta2DDirection = FVector(DeltaDirection.X, DeltaDirection.Y, 0.0f);

		float DotProduct = FVector::DotProduct(PreDelta2DDirection, NewCurrentDelta2DDirection);
		// Restrictions between -1.0f and 1.0F (anti -point error prevention)
		DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
		// Angle calculation between the two vectors (radians)
		float AngleDiff = FMath::Acos(DotProduct);
		// Convert the angle to degree
		float AngleDiffDegrees = FMath::RadiansToDegrees(AngleDiff);
    
		// Normalization of the angle difference in the 0 ~ 1 range (0 when it is 180 degrees, 1 when 0 degrees 1)
		float NormalizedAngle = 1.0f - (AngleDiffDegrees / 180.0f);
    
		// Calculation of interpolation speed (linear interpolation)
		float MinInterpSpeed = 1.0f;
		float MaxInterpSpeed = 10.0f;
		float InterpSpeed = FMath::Lerp(MinInterpSpeed, MaxInterpSpeed, NormalizedAngle);
		
		FVector SmoothedDirection = FMath::VInterpTo(PreDelta2DDirection, NewCurrentDelta2DDirection, DeltaTime, InterpSpeed);
		CurrentDelta2DDirection = SmoothedDirection;
		
		const FVector AddInputVector = (Spline2DDirection * MaxWalkSpeed) + SmoothedDirection;
		CharacterMovement->AddInputVector(AddInputVector);

		// UK_LOG(Log,"AngleDiffDegrees = %f", AngleDiffDegrees);
		// DrawDebugDirectionalArrow(GetWorld(), ActorLocation, ActorLocation + (Spline2DDirection * MaxWalkSpeed), 1, FColor::Red);
		// DrawDebugDirectionalArrow(GetWorld(), ActorLocation, ActorLocation + (SmoothedDirection), 1, FColor::Blue);
		// DrawDebugDirectionalArrow(GetWorld(), NextSplineLocation, NextSplineLocation + FVector(0,0,100), 1, FColor::Green);
    }
	else
    {
    	const float SplineLength = Spline->GetSplineLength();
	    if (Spline->IsClosedLoop())
	    {
	    	if (CurrentDistance > SplineLength)
	    	{
	    		CurrentDistance = 0.0f;
	    	}
	    }
	    else
	    {
	    	if ((SplineLength - CurrentDistance) < 1.0f)
	    	{
	    		Direction = -1.0f;
	    	}
		    else if (CurrentDistance < 0.0f)
		    {
		    	Direction = 1.0f;
		    }
	    }
    	
    	const FVector ActorLocation = GetOwner()->GetActorLocation();
    	const float MaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
    	CurrentDistance = CurrentDistance + (MaxWalkSpeed * DeltaTime * Direction);

    	const FVector NextSplineLocation = Spline->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
    	const FVector AddInputVector = ((NextSplineLocation - ActorLocation));
    	CharacterMovement->AddInputVector(AddInputVector);
    }
}