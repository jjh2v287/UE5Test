// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKPatrolPathManager.h"
#include "Components/SplineComponent.h"

UUKPatrolPathManager* UUKPatrolPathManager::Instance = nullptr;

UUKPatrolPathManager* UUKPatrolPathManager::Get()
{
	return Instance;
}

void UUKPatrolPathManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
}

void UUKPatrolPathManager::Deinitialize()
{
	if (Instance == this)
	{
		PatrolPathSplines.Empty();
		Instance = nullptr;
	}
	Super::Deinitialize();
}

void UUKPatrolPathManager::RegisterPatrolSpline(AUKPatrolPathSpline* Waypoint)
{
	if (!PatrolPathSplines.Contains(Waypoint->SplineName))
	{
		PatrolPathSplines.Emplace(Waypoint->SplineName, Waypoint);
	}
}

void UUKPatrolPathManager::UnregisterPatrolSpline(AUKPatrolPathSpline* Waypoint)
{
	if (PatrolPathSplines.Contains(Waypoint->SplineName))
	{
		PatrolPathSplines.Remove(Waypoint->SplineName);
	}
}

FPatrolSplineSearchResult UUKPatrolPathManager::FindClosePatrolPath(FVector Location)
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

FPatrolSplineSearchResult UUKPatrolPathManager::FindRandomPatrolPath(const FVector Location)
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

FPatrolSplineSearchResult UUKPatrolPathManager::FindRandomPatrolPathToTest(FName SplineName, const FVector Location, const int32 StartIndex, const int32 EndIndex)
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

FPatrolSplineSearchResult UUKPatrolPathManager::FindPatrolPathWithBoundsByNameAndLocation(const FVector Location, const FName SplineName, const int32 StartPoint, const int32 EndPoint)
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

		int32 MaxSplinePointCount = Spline->SplineComponent->GetNumberOfSplinePoints();
		if (StartPoint >= MaxSplinePointCount || EndPoint >= MaxSplinePointCount)
		{
			return Result;
		}

		const float StartKey = static_cast<float>(StartPoint);
		const float EndKey = static_cast<float>(EndPoint);
    	
		// Find the closest location in Spline
		const FVector PointLocation = Spline->SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
		const FVector StartPointLocation = Spline->SplineComponent->GetLocationAtSplineInputKey(StartKey, ESplineCoordinateSpace::World);
		const FVector EndPointLocation = Spline->SplineComponent->GetLocationAtSplineInputKey(EndKey, ESplineCoordinateSpace::World);
        
		Result.SplineComponent = Spline->SplineComponent;
		Result.CloseLocation = PointLocation;
		Result.StartLocation = StartPointLocation;
		Result.EndLocation = EndPointLocation;
	}

	Result.Success = true;
	return Result;
}