// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UKTickIntervalTargetComponent.generated.h"

class UKTickIntervalSubsystem;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UETEST_API UUKTickIntervalTargetComponent : public USceneComponent
{
	GENERATED_BODY()

	friend class UKTickIntervalSubsystem;
	
	UPROPERTY(Transient)
	UKTickIntervalSubsystem* TickIntervalSubsystem;
	
public:	
	// Sets default values for this component's properties
	UUKTickIntervalTargetComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

public:
	void SetTickInterval(float IntervalTime);
	void SetTickEnabled(bool Enabled);
	
private:
	TInlineComponentArray<UActorComponent*> Components;

public:
	int32 TargetIndex = INDEX_NONE;
	int32 TickIntervalLayer = INDEX_NONE;
};
