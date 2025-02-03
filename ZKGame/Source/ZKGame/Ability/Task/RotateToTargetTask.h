// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "RotateToTargetTask.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRotateTaskCompletedDelegate);

UCLASS()
class ZKGAME_API URotateToTargetTask : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FRotateTaskCompletedDelegate OnRotationCompleted;

	// 블루프린트에서 호출할 수 있는 정적 함수
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static URotateToTargetTask* RotateActorToTarget(UGameplayAbility* OwningAbility, FRotator TargetRotation, float Duration);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	FRotator StartRotation;
	FRotator TargetRotation;
	float Duration;
	float ElapsedTime;
};