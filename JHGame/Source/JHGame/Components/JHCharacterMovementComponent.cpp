// Fill out your copyright notice in the Description page of Project Settings.


#include "JHCharacterMovementComponent.h"

void UJHCharacterMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	Super::PhysWalking(DeltaTime, Iterations);
	
	Velocity += SlimePendingImpulseToApply;
	SlimePendingImpulseToApply = FVector::ZeroVector;
}

void UJHCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	Super::PhysCustom(DeltaTime, Iterations);

	PhysSlime(DeltaTime, Iterations);
	// If there are no pending impulses to apply, exit the function.
	if (SlimePendingImpulseToApply.IsZero())
	{
		return;
	}
	
	// Cache the current velocity before applying the impulse.
	const FVector VelocityCache = Velocity;

	// Apply braking to the slime impulse vector.
	// Temporarily set the current velocity to the pending impulse to use the Unreal Engine function.
	Velocity = SlimePendingImpulseToApply;
	ApplyVelocityBraking(DeltaTime, 4.f, 1000.f);
	SlimePendingImpulseToApply = Velocity;

	// Restore the cached velocity.
	Velocity = VelocityCache;
	
	// Add the braked impulse to the cached velocity.
	Velocity += SlimePendingImpulseToApply;

	const FVector Delta = Velocity * DeltaTime;
	// Movement
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, CurrentFloor.HitResult);
}

void UJHCharacterMovementComponent::SlimeAddImpulse(const FVector& InImpulse)
{
	SlimePendingImpulseToApply += InImpulse;
}

void UJHCharacterMovementComponent::PhysSlime(float DeltaTime, int32 Iterations)
{
}
