// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "UKAsyncTask_Base.h"
#include "AI/Patrol/UKPatrolPathManager.h"
#include "UKAsyncTask_SplineMove.generated.h"

class UWorld;
class ACharacter;

UCLASS(BlueprintType, Blueprintable, meta = (ExposedAsyncProxy = AsyncAction))
class UUKAsyncTask_SplineMove : public UUKAsyncTask_Base
{
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveFinishDelegate);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveFailDelegate);

	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FMoveFinishDelegate OnMoveFinish;

	UPROPERTY(BlueprintAssignable)
	FMoveFailDelegate OnMoveFail;

public:
	/**
	 * Create a latent UUKAsyncTask_RotateCharacter Node
	 * @param Owner					Owner Actor
	 * @param SplineName			SplineName
	 * @param StartIndex			Start Point Index
	 * @param EndIndex				End Point Index
	 * @return This UUKAsyncTask_SplineMove blueprint node
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "SplineMove", Category = "UK|AsyncTask"))
	static UUKAsyncTask_SplineMove* SplineMove(AActor* Owner, TSubclassOf<class UUKAsyncTask_SplineMove> Class, const FName SplineName, const int32 StartIndex, const int32 EndIndex);

public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable)
	void FinishTask(const bool bSucceeded = true);

protected:
	virtual void Cancel() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnActivate();

	UFUNCTION(BlueprintImplementableEvent)
	void OnTick(const float DeltaTime);

	UFUNCTION(BlueprintPure)
	AActor* GetOwner() const;
	
protected:
	UPROPERTY(Transient)
	AActor* Owner = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	FName SplineName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	int32 StartIndex;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
    int32 EndIndex;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	FPatrolSplineSearchResult PatrolSplineSearchResult;
};
