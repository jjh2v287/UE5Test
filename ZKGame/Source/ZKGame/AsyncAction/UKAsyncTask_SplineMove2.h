// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "UKAsyncTask_Base.h"
#include "AI/Patrol/UKPatrolPathManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UKAsyncTask_SplineMove2.generated.h"

class UWorld;
class ACharacter;

UCLASS(BlueprintType, Blueprintable, meta = (ExposedAsyncProxy = AsyncAction))
class UUKAsyncTask_SplineMove2 : public UUKAsyncTask_Base
{
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveFinishDelegate);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveFailDelegate);

	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FMoveFinishDelegate OnMoveFinish;

	UPROPERTY(BlueprintAssignable)
	FMoveFailDelegate OnMoveFail;

	/**
	 * Create a latent UUKAsyncTask_RotateCharacter Node
	 * @param Owner					Owner Actor
	 * @param SplineName			SplineName
	 * @param StartIndex			Start Point Index
	 * @param EndIndex				End Point Index
	 * @return This UUKAsyncTask_SplineMove blueprint node
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "SplineMove2", Category = "UK|AsyncTask"))
	static UUKAsyncTask_SplineMove2* SplineMove2(AActor* Owner, const FName SplineName, const int32 StartIndex, const int32 EndIndex, const float TripTime);

	virtual void Activate() override;

	UFUNCTION(BlueprintCallable)
	void FinishTask(const bool bSucceeded = true);

protected:
	virtual void Cancel() override;
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(Transient)
	AActor* Owner = nullptr;
	
	UPROPERTY(Transient)
	FName SplineName;

	UPROPERTY(Transient)
	int32 StartIndex;
	
	UPROPERTY(Transient)
    int32 EndIndex;

	UPROPERTY(Transient)
	float TripTime = 0.0f;

	UPROPERTY(Transient)
	FPatrolSplineSearchResult PatrolSplineSearchResult;

	UPROPERTY(Transient)
	float CurrentDistance = 0.0f;

	UPROPERTY(Transient)
	float GoalDistance = 0.0f;

	UPROPERTY(Transient)
	float PreMaxWalkSpeed = 0.0f;
	
	UPROPERTY(Transient)
	UCharacterMovementComponent* CharacterMovement = nullptr;
};
