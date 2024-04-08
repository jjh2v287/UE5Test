// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UKTickIntervalFocusComponent.generated.h"

class UKTickIntervalSubsystem;

/** Tick settings to use with calculated distance zone / visibility. */
USTRUCT(BlueprintType)
struct FUKTickIntervalSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="UK Tick Interval", meta=(DisplayPriority=0))
	float IntervalDistance = 0.0f;

	UPROPERTY(EditAnywhere, Category="UK Tick Interval", meta=(DisplayPriority=1))
	float IntervalTime = 0.0f;

	UPROPERTY(EditAnywhere, Category="UK Tick Interval", meta=(DisplayPriority=2))
	bool EnabledVisible = true;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE5TEST_API UUKTickIntervalFocusComponent : public USceneComponent
{
	GENERATED_BODY()

	friend class UKTickIntervalSubsystem;

	UPROPERTY(Transient)
	UKTickIntervalSubsystem* TickIntervalSubsystem;

public:	
	// Sets default values for this component's properties
	UUKTickIntervalFocusComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

public:
	int32 FocusIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category="UK Tick Interval", AdvancedDisplay)
	TArray<FUKTickIntervalSettings> TickIntervalSettings = {
		{4000.0f, 1.0f, false}
		,{3000.0f, 0.75f, true}
		,{2000.0f, 0.5f, true}
		,{1000.0f, 0.25f, true}
	};
};
