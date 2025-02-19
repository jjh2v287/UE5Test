// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UKPatrolPathSpline.h"
#include "Components/SplineComponent.h"
#include "UKPatrolPathManager.generated.h"

USTRUCT(BlueprintType)
struct ZKGAME_API FPatrolSplineSearchResult
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool Success = false;

	UPROPERTY(BlueprintReadOnly)
	USplineComponent* SplineComponent = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FVector CloseLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector EndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float Distance = MAX_FLT;

	void Reset()
	{
		Success = false;
		SplineComponent = nullptr;
		CloseLocation = FVector::ZeroVector;
		StartLocation = FVector::ZeroVector;
		EndLocation = FVector::ZeroVector;
		Distance = MAX_FLT;
	}
};

/**
 * 
 */
UCLASS()
class ZKGAME_API UUKPatrolPathManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static UUKPatrolPathManager* Get();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterPatrolSpline(AUKPatrolPathSpline* Waypoint);
	void UnregisterPatrolSpline(AUKPatrolPathSpline* Waypoint);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindClosePatrolPath(FVector Location);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindRandomPatrolPath(const FVector Location);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindRandomPatrolPathToTest(FName SplineName, const FVector Location, const int32 StartIndex, const int32 EndIndex);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindPatrolPathWithBoundsByNameAndLocation(const FVector Location, const FName SplineName, const int32 StartPoint, const int32 EndPoint);

private:
	static UUKPatrolPathManager* Instance;

	UPROPERTY(Transient)
	TMap<FName, AUKPatrolPathSpline*> PatrolPathSplines;
};
