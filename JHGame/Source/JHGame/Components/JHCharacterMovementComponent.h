// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JHCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class JHGAME_API UJHCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm="InImpulse"), Category="Character Movement: Slime")
	void SlimeAddImpulse(const FVector& InImpulse);
	
protected:
	FVector SlimePendingImpulseToApply = FVector::ZeroVector;
	
	void PhysSlime(float DeltaTime, int32 Iterations);
};
