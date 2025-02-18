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
	if (!PatrolPathSplines.Contains(Waypoint->SplineName))
	{
		PatrolPathSplines.Emplace(Waypoint->SplineName, Waypoint);
	}
}

void UUKPatrolPathSubsystem::UnregisterPatrolSpline(AUKPatrolPathSpline* Waypoint)
{
	if (PatrolPathSplines.Contains(Waypoint->SplineName))
	{
		PatrolPathSplines.Remove(Waypoint->SplineName);
	}
}

FPatrolSplineSearchResult UUKPatrolPathSubsystem::FindClosePatrolPath(FVector Location)
{
	FPatrolSplineSearchResult Result;
	if (PatrolPathSplines.Num() == 0)
	{
		return Result;
	}

	for (const auto Pair : PatrolPathSplines)
	{
		AUKPatrolPathSpline* Spline = Pair.Value;
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
			Result.CloseLocation = PointLocation;
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

	TArray<FName> Keys;
	PatrolPathSplines.GenerateKeyArray(Keys);
	int32 RandomIndex = FMath::RandRange(0, PatrolPathSplines.Num()-1);
	const FName& RandomKey = Keys[RandomIndex];
	
	if (const AUKPatrolPathSpline* Spline = PatrolPathSplines.FindRef(RandomKey))
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
			Result.CloseLocation = PointLocation;
			Result.Distance = CurrentDistance;
		}
	}
        
	return Result;
}

FPatrolSplineSearchResult UUKPatrolPathSubsystem::FindRandomPatrolPathToTest(FName SplineName, const FVector Location, const int32 StartIndex, const int32 EndIndex)
{
	FPatrolSplineSearchResult Result;
	if (PatrolPathSplines.Num() == 0)
	{
		return Result;
	}

	if (const AUKPatrolPathSpline* Spline = PatrolPathSplines.FindRef(SplineName))
	{
		if (!Spline->SplineComponent)
		{
			return Result;
		}

		const FVector CloseLocation = Spline->SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
		const FVector StartLocation = Spline->SplineComponent->GetLocationAtSplineInputKey(StartIndex, ESplineCoordinateSpace::World);
		const FVector EndLocation = Spline->SplineComponent->GetLocationAtSplineInputKey(EndIndex, ESplineCoordinateSpace::World);
		const float CurrentDistance = FVector::Distance(Location, CloseLocation);
            
		Result.SplineComponent = Spline->SplineComponent;
		Result.Distance = CurrentDistance;
		Result.CloseLocation = CloseLocation;
		Result.StartLocation = StartLocation;
		Result.EndLocation = EndLocation;
	}
        
	return Result;
}