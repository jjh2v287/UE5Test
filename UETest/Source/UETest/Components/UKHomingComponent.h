// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UKHomingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHomingEventDelegate);

UENUM(BlueprintType, Category = "UK Homing")
enum class EUKHomingStopType : uint8
{
	None,
	Time,
	MaxAngle,
};

UENUM(BlueprintType, Category = "UK Homing")
enum class EUKHomingState : uint8
{
	Stop,
	Homing,
};

USTRUCT(BlueprintType)
struct FUKHomingStartPram
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HomingRotateSpeed = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BlackboardValueName = TEXT("Player");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUKHomingStopType HomingStopType = EUKHomingStopType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="HomingStopType == EUKHomingStop::MaxAngle", EditConditionHides))
	float HomingMaxAngle = 360.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="HomingStopType == EUKHomingStop::Time", EditConditionHides))
	float Duration = 1.0f;
};

UCLASS( meta=(BlueprintSpawnableComponent, DisplayName="UK Homing") )
class UETEST_API UUKHomingComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
public:	
	UUKHomingComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="UK Homing")
	void HomingStart(const FUKHomingStartPram HomingStartPram);

	UFUNCTION(BlueprintCallable, Category="UK Homing")
	void HomingStop();

	UFUNCTION(BlueprintPure, Category="UK Homing")
	const bool IsHomingMove() const;

	UFUNCTION(BlueprintPure, Category="UK Homing")
	const bool IsHomingStop() const;

	/**
		 * @param Owner - Rotation value change target.
		 * @param Target - Lookup target.
		 * @param HomingSpeed - 0.0(Owner Rotation) ~ 1.0(Calculation Target Rotation)
		 * @return FVector
		 */
	UFUNCTION(BlueprintPure, Category = "Homing")
	static FVector GetHomingVector(const AActor* Owner, const AActor* Target, const float HomingSpeed);
	
	/**
	 * @param Owner - Rotation value change target.
	 * @param Target - Lookup target.
	 * @param HomingSpeed - 0.0(Owner Rotation) ~ 1.0(Calculation Target Rotation)
	 * @return FRotator
	 */
	UFUNCTION(BlueprintPure, Category = "Homing")
	static FRotator GetHomingRotator(const AActor* Owner, const AActor* Target, const float HomingSpeed);
	
private:
	void HomingUpdate(const float DeltaTime);
	void HomingTargetActorFindProcess(AActor* Owner);
	const float GetCalculateAngleDelta(const float CurrentAngle, const float StartAngle);

public:
	UPROPERTY(BlueprintAssignable, VisibleAnywhere, BlueprintCallable, Category = "UK Homing")
	FOnHomingEventDelegate OnHomingEndEvent;
	
private:
	FUKHomingStartPram HomingStartInfo;
	FTimerHandle HomingEndTimer;
	
	FRotator HomingStartRotator = FRotator::ZeroRotator;
	TWeakObjectPtr<AActor> HomingTargetActor = nullptr;
	EUKHomingState HomingState = EUKHomingState::Stop;
};
