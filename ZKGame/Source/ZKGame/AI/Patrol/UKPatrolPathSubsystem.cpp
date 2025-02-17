// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKPatrolPathSubsystem.h"
#include "Components/SplineComponent.h"

UUKPatrolPathSubsystem* UUKPatrolPathSubsystem::Instance = nullptr;

UUKPatrolPathSubsystem* UUKPatrolPathSubsystem::Get()
{
	return Instance;
}

void UUKPatrolPathSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
}

void UUKPatrolPathSubsystem::Deinitialize()
{
	if (Instance == this)
	{
		PatrolPathSplines.Empty();
		Instance = nullptr;
	}
	Super::Deinitialize();
}

void UUKPatrolPathSubsystem::RegisterPatrolSpline(AUKPatrolPathSpline* Waypoint)
{
	PatrolPathSplines.Emplace(Waypoint);
}

void UUKPatrolPathSubsystem::UnregisterPatrolSpline(AUKPatrolPathSpline* Waypoint)
{
	PatrolPathSplines.Remove(Waypoint);
}

FPatrolSplineSearchResult UUKPatrolPathSubsystem::FindClosePatrolPath(FVector Location)
{
	FPatrolSplineSearchResult Result;
	if (PatrolPathSplines.Num() == 0)
	{
		return Result;
	}

	for (AUKPatrolPathSpline* Spline : PatrolPathSplines)
	{
		if (!IsValid(Spline))
		{
			continue;
		}

		if (Spline->SplineComponent)
		{
			
		}

		// Find the closest location in Spline
		FVector PointLocation = Spline->SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
            
		// calculation of distance from current location
		float CurrentDistance = FVector::Distance(Location, PointLocation);
            
		// if you find a closer point update results
		if (CurrentDistance < Result.Distance)
		{
			Result.SplineComponent = Spline->SplineComponent;
			Result.Location = PointLocation;
			Result.Distance = CurrentDistance;
		}
	}
        
	return Result;
}

FPatrolSplineSearchResult UUKPatrolPathSubsystem::FindRandomPatrolPath(const FVector Location)
{
	FPatrolSplineSearchResult Result;
	if (PatrolPathSplines.Num() == 0)
	{
		return Result;
	}

	int32 RandomIndex = FMath::RandRange(0, PatrolPathSplines.Num()-1);
	if (const AUKPatrolPathSpline* Spline = PatrolPathSplines[RandomIndex])
	{
		if (!IsValid(Spline))
		{
			return Result;
		}

		if (!Spline->SplineComponent)
		{
			return Result;
		}

		// Find the closest location in Spline
		const FVector PointLocation = Spline->SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
            
		// calculation of distance from current location
		const float CurrentDistance = FVector::Distance(Location, PointLocation);
            
		// if you find a closer point update results
		if (CurrentDistance < Result.Distance)
		{
			Result.SplineComponent = Spline->SplineComponent;
			Result.Location = PointLocation;
			Result.Distance = CurrentDistance;
		}
	}
        
	return Result;
}
