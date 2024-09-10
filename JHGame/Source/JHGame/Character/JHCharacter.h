// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "JHCharacter.generated.h"

UCLASS()
class JHGAME_API AJHCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AJHCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void LaunchToTargetWithFriction(const FVector TargetLocation);

	float GetLaunchCharacterDistanceZ(float DistanceZ);
	
	FVector CalculateLaunchVelocityWithFriction(const FVector& TargetLocation, const float FrictionCoefficient) const;

	FVector CalculateLaunchVelocity(FVector Start, FVector Target, float LaunchAngle) const;
};
