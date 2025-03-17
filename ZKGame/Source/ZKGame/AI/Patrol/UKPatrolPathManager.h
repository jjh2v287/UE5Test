// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UKPatrolPathManager.generated.h"

struct FGameplayTag;
class AUKPatrolPathSpline;

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
}; 

/**
 * 
 */
UCLASS()
class ZKGAME_API UUKPatrolPathManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static UUKPatrolPathManager* Get() { return Instance; }

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterPatrolSpline(AUKPatrolPathSpline* Waypoint);
	void UnregisterPatrolSpline(const AUKPatrolPathSpline* Waypoint);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindClosePatrolPath(const FVector Location);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindRandomPatrolPath(const FVector Location);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindClosePatrolPathByName(const FVector Location, const FName& SplineName);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindPatrolPathWithBoundsByNameAndLocation(const FVector Location, const FName SplineName, const int32 StartPoint, const int32 EndPoint);
	
	UFUNCTION(BlueprintPure)
	FVector GetSplinePointLocation(const FName& SplineName, int32 Index);

private:
	static UUKPatrolPathManager* Instance;

	UPROPERTY(Transient)
	TMap<FName, AUKPatrolPathSpline*> PatrolPathSplines;
};