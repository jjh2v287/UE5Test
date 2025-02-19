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

// 이동 완료 이벤트 선언 (Blueprint에서 바인딩 가능)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementFinished, ESplineMoveCompleteType, MoveCompleteType);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZKGAME_API UUKSplineMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUKSplineMovementComponent(const FObjectInitializer& ObjectInitializer);

	/** 스플라인을 따라 이동을 시작합니다.
	 * @param SplineName 이동에 사용할 스플라인 컴포넌트
	 * @param StartIndex 스플라인 전체를 이동하는 데 걸리는 시간 (초)
	 * @param EndIndex 반복 여부 (true이면 끝나면 처음부터 다시 시작)
	 */
	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	void StartSplineMovement(const FName SplineName, const int32 StartIndex, const int32 EndIndex);

	/** 이동을 일시 정지합니다. */
	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	void PauseMovement();

	/** 일시 정지된 이동을 재개합니다. */
	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	void ResumeMovement();

	UFUNCTION(BlueprintCallable, Category = "Spline Movement")
	void Finish(const bool bSucceeded = true);
	
	/** 이동이 완료되었을 때 호출되는 이벤트 (반복하지 않을 경우) */
	UPROPERTY(BlueprintAssignable, Category = "Spline Movement")
	FOnMovementFinished OnMovementFinished;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 이동에 사용할 스플라인 컴포넌트
	UPROPERTY(Transient)
	USplineComponent* Spline;

	UPROPERTY(Transient)
	float CurrentDistance = 0.0f;

	UPROPERTY(Transient)
	float GoalDistance = 0.0f;

	UPROPERTY(Transient)
	float PreMaxWalkSpeed = 0.0f;

	UPROPERTY(Transient)
	float TripTime = 0.0f;

	UPROPERTY(Transient)
	FVector EndLocation = FVector::ZeroVector;
	
	UPROPERTY(Transient)
	UCharacterMovementComponent* CharacterMovement = nullptr;

	// 이동이 일시 정지 중인지 여부
	bool bIsPaused;
};
