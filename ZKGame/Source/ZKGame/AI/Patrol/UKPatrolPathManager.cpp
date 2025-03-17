// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKPatrolPathManager.h"
#include "UKPatrolPathSpline.h"
#include "Components/SplineComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKPatrolPathManager)

UUKPatrolPathManager* UUKPatrolPathManager::Instance = nullptr;

void UUKPatrolPathManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
}

void UUKPatrolPathManager::Deinitialize()
{
	PatrolPathSplines.Empty();
	Instance = nullptr;
	Super::Deinitialize();
}

auto UUKPatrolPathManager::RegisterPatrolSpline(AUKPatrolPathSpline* Waypoint) -> void
{
	if (!PatrolPathSplines.Contains(Waypoint->SplineName))
	{
		PatrolPathSplines.Emplace(Waypoint->SplineName, Waypoint);
	}
}

void UUKPatrolPathManager::UnregisterPatrolSpline(const AUKPatrolPathSpline* Waypoint)
{
	if (PatrolPathSplines.Contains(Waypoint->SplineName))
	{
		PatrolPathSplines.Remove(Waypoint->SplineName);
	}
}

FPatrolSplineSearchResult UUKPatrolPathManager::FindClosePatrolPath(const FVector Location)
{
	FPatrolSplineSearchResult Result;
	if (PatrolPathSplines.Num() == 0)
	{
		return Result;
	}

	for (const TPair<FName, AUKPatrolPathSpline*>& Pair : PatrolPathSplines)
	{
		const AUKPatrolPathSpline* Spline = Pair.Value;
		if (!IsValid(Spline))
		{
			continue;
		}

		if (!Spline->SplineComponent)
		{
			continue;
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
			Result.Success = true;
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
		Result.Success = true;
	}

	return Result;
}

FPatrolSplineSearchResult UUKPatrolPathManager::FindClosePatrolPathByName(const FVector Location, const FName& SplineName)
{
	FPatrolSplineSearchResult Result;
	if (PatrolPathSplines.Num() == 0)
	{
		return Result;
	}

	TArray<FName> FoundKeys;
	for (const TPair<FName, AUKPatrolPathSpline*>& Pair : PatrolPathSplines)
	{
		FString KeyString = Pair.Key.ToString();
		if (KeyString.StartsWith(SplineName.ToString()))
		{
			FoundKeys.Add(Pair.Key);
		}
	}

	for (const FName& Key : FoundKeys)
	{
		if (const AUKPatrolPathSpline* Spline = PatrolPathSplines.FindRef(Key))
		{
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
				Result.Success = true;
			}
		}
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
		Result.Success = true;
    }

    return Result;
}

FVector UUKPatrolPathManager::GetSplinePointLocation(const FName& SplineName, int32 Index)
{
	FVector OutVector = FVector::ZeroVector; 
	if (!PatrolPathSplines.Contains(SplineName))
	{
		return OutVector;
	}

	AUKPatrolPathSpline* PathSpline = PatrolPathSplines.FindRef(SplineName);
	if (PathSpline->SplineComponent->GetNumberOfSplinePoints() <= Index)
	{
		return OutVector;
	}

	OutVector = PathSpline->SplineComponent->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
	return OutVector;
}
