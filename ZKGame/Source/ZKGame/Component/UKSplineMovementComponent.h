// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UKSplineMovementComponent.generated.h"

UENUM()
enum ESplineMoveCompleteType : uint8
{
	Finish,
	Fail
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementFinished, ESplineMoveCompleteType, MoveCompleteType);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZKGAME_API UUKSplineMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUKSplineMovementComponent(const FObjectInitializer& ObjectInitializer);

	/** Start moving along the spline.
	 * @param SplineName Spline Name
	 * @param StartIndex Spline key for the starting position
	 * @param EndIndex Mobile completed spline key
	 * @param MovingTimeSecond Moving Time
	 */
	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	bool StartSplineMovement(const FName SplineName, const int32 StartIndex, const int32 EndIndex, const float MovingTimeSecond = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	bool StartSplineInfinityMovement(const FName SplineName, const float MovingTimeSecond = 0.0f);
	
	/** Pause moving. */
	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	void PauseMovement();

	/** Resumes pause movements. */
	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	void ResumeMovement();

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	void Finish(const bool bSucceeded = true);

	/** Events called when the movement is completed */
	UPROPERTY(BlueprintAssignable, Category = "Spline Movement")
	FOnMovementFinished OnMovementFinished;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	bool bIsPaused = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	bool bIsMoving = false;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	UPROPERTY(Transient)
	USplineComponent* Spline;

	UPROPERTY(Transient)
	float CurrentDistance = 0.0f;

	UPROPERTY(Transient)
	float StartDistance = 0.0f;
	
	UPROPERTY(Transient)
	float GoalDistance = 0.0f;

	UPROPERTY(Transient)
	float PreMaxWalkSpeed = 0.0f;

	UPROPERTY(Transient)
	float NewMaxWalkSpeed = 0.0f;

	UPROPERTY(Transient)
	FVector EndLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector PreDelta2DDirection = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector CurrentDelta2DDirection = FVector::ZeroVector;
	
	UPROPERTY(Transient)
	UCharacterMovementComponent* CharacterMovement = nullptr;

	UPROPERTY(Transient)
	bool bIsInfinity = false;

	UPROPERTY(Transient)
	float Direction = 1.0f;
};
