// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "UKAsyncTask_Base.h"
#include "UKAsyncTask_SplineMove.generated.h"

class UWorld;
class ACharacter;

UCLASS(BlueprintType, Blueprintable, meta = (ExposedAsyncProxy = AsyncAction))
class UUKAsyncTask_SplineMove : public UUKAsyncTask_Base
{
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveFinishDelegate);

	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	// ReSharper disable once CppUEBlueprintCallableFunctionUnused
	FMoveFinishDelegate OnMoveFinish;

public:
	/**
	 * Create a latent UUKAsyncTask_RotateCharacter Node
	 * @param WorldContextObject	Current Object to use this use async node.
	 * @param Actor					Actor to Rotate
	 * @param TargetRotation		Target Rotation to Rotate
	 * @param InterpSpeed			Rotation Interp Speed
	 * @param Tolerance				Tolerance for Target Rotation
	 * @param bInterpConstant		Option to control interpolation speed
	 * @return This UUKAsyncTask_RotateCharacter blueprint node
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Rotate Actor To Target Rotation", Category = "UK|AsyncTask"))
	static UUKAsyncTask_SplineMove* SplineMove(const UObject* WorldContextObject, TSubclassOf<class UUKAsyncTask_SplineMove> Class);

public:
	virtual void Activate() override;

protected:
	virtual void Cancel() override;
	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnActivate();

	UFUNCTION(BlueprintImplementableEvent)
	void OnTick(const float DeltaTime);

protected:
	// Character to Rotate
	UPROPERTY(Transient)
	AActor* Actor = nullptr;
	// Target Rotation to Rotate
	FRotator TargetRotation = FRotator::ZeroRotator;
	// Rotation Interp Speed
	float InterpSpeed = 10.f;
	// Tolerance for Target Rotation
	float Tolerance = 0.0001f;
	// If false, use FMath::RInterpTo. If true, use FMath::RInterpConstantTo.
	bool bInterpConstant = false;
};
