// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UKPatrolPathSpline.h"
#include "Components/SplineComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "UKPatrolPathSubsystem.generated.h"

USTRUCT(BlueprintType)
struct ZKGAME_API FPatrolSplineSearchResult
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	USplineComponent* SplineComponent = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float Distance = MAX_FLT;
};

/**
 * 
 */
UCLASS()
class ZKGAME_API UUKPatrolPathSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	static UUKPatrolPathSubsystem* Get();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterPatrolSpline(AUKPatrolPathSpline* Waypoint);
	void UnregisterPatrolSpline(AUKPatrolPathSpline* Waypoint);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindClosePatrolPath(FVector Location);

	UFUNCTION(BlueprintCallable)
	FPatrolSplineSearchResult FindRandomPatrolPath(const FVector Location);

private:
	static UUKPatrolPathSubsystem* Instance;

	UPROPERTY(Transient)
	TArray<AUKPatrolPathSpline*> PatrolPathSplines;
};
