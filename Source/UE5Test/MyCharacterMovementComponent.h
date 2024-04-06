// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class UE5TEST_API UMyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<AActor> Box;

public:
	UMyCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	virtual void UpdateBasedMovement(float DeltaSeconds) override;

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
};
