// Copyright Juan Escudero Vizcaíno 2025 All Rights Reserved.

#include "DynamicAvoidanceComponent.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Navigation/PathFollowingComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4

#include "Engine/HitResult.h"

#endif

/**
* Console variables.
***********************************************************************************/

#if !UE_BUILD_SHIPPING

TAutoConsoleVariable<int32> CVarDrawContextRays(
	TEXT("DynamicAvoidance.DrawContextRays"),
	0,
	TEXT("Draw context steering debug rays?\n")
	TEXT("  0: Off\n")
	TEXT("  1: On\n"),
	ECVF_Default);

TAutoConsoleVariable<int32> CVarDrawNavPath(
	TEXT("DynamicAvoidance.DrawNavPath"),
	0,
	TEXT("Draw nav path?\n")
	TEXT("  0: Off\n")
	TEXT("  1: On\n"),
	ECVF_Default);

#endif // !UE_BUILD_SHIPPING

/**
* Construct a UDynamicAvoidanceComponent.
***********************************************************************************/

UDynamicAvoidanceComponent::UDynamicAvoidanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

/**
* Begin play for this component.
***********************************************************************************/

void UDynamicAvoidanceComponent::BeginPlay()
{
	Super::BeginPlay();

	ContextRayDirections.SetNum(ContextSamples);
	InterestMap.SetNum(ContextSamples);
	DangerMap.SetNum(ContextSamples);

	// Set the context directions based on the number of samples.

	const float angleBetweenRays = 2.0f * PI / ContextSamples;

	for (int32 i = 0; i < ContextSamples; ++i)
	{
		float angle = i * angleBetweenRays;
		ContextRayDirections[i] = FVector(FMath::Cos(angle), FMath::Sin(angle), 0.0f);
	}

	// Calculate min dot product for ramps.

	MinAllowedRampDotProduct = FMath::Cos(FMath::DegreesToRadians(MaxAllowedRampAngle));
}

/**
* Regular update tick.
***********************************************************************************/

void UDynamicAvoidanceComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	// Bind ExecuteContextSteeringAlgorithm to UPathFollowingComponent.PostProcessMove.
	// Done here instead of BeginPlay to avoid timing issues.

	if (!WasBindingSuccessful && GetPathFollowingComponent() != nullptr)
	{
		GetPathFollowingComponent()->PostProcessMove.BindUObject(this, &ThisClass::ExecuteContextSteeringAlgorithm);
		WasBindingSuccessful = true;
	}
}

/**
* Get the owner of this component (cast to APawn).
***********************************************************************************/

APawn* UDynamicAvoidanceComponent::GetPawnOwner()
{
	if (!PawnOwner.IsValid())
	{
		if (APawn* pawn = Cast<APawn>(GetOwner()))
		{
			PawnOwner = pawn;
		}
	}

	return PawnOwner.Get();
}

/**
* Get the controller of the pawn owner.
***********************************************************************************/

AController* UDynamicAvoidanceComponent::GetController()
{
	if (!Controller.IsValid())
	{
		if (GetPawnOwner() != nullptr)
		{
			Controller = GetPawnOwner()->GetController();
		}
	}

	return Controller.Get();
}

/**
* Get the controller path following component.
***********************************************************************************/

UPathFollowingComponent* UDynamicAvoidanceComponent::GetPathFollowingComponent()
{
	if (!PathFollowingComponent.IsValid())
	{
		if (GetController() != nullptr)
		{
			PathFollowingComponent = GetController()->FindComponentByClass<UPathFollowingComponent>();
		}
	}

	return PathFollowingComponent.Get();
}

/**
* Execute the context steering algorithm, reacting to UPathFollowingComponent.PostProcessMove.
***********************************************************************************/

void UDynamicAvoidanceComponent::ExecuteContextSteeringAlgorithm(UPathFollowingComponent* comp, FVector& velocity)
{
	if (!IsEnabled)
	{
		return;
	}

	PathFollowingDirection = velocity.GetSafeNormal2D();

	ResetMaps();
	FillDangerMap();
	FillInterestMap();
	CalculateSteeringDirection();

	// Apply context steering direction.

	velocity = SteeringDirection * velocity.Size();

#if !UE_BUILD_SHIPPING

	DrawContextRays();
	DrawNavPath();

#endif // !UE_BUILD_SHIPPING

}

/**
* Reset the danger and interest maps.
***********************************************************************************/

void UDynamicAvoidanceComponent::ResetMaps()
{
	for (int32 i = 0; i < ContextSamples; ++i)
	{
		InterestMap[i] = 0;
		DangerMap[i] = 0;
	}
}

/**
* Fill the interest map, considering the path following direction.
***********************************************************************************/

void UDynamicAvoidanceComponent::FillInterestMap()
{
	// Seek.

	for (int32 i = 0; i < ContextSamples; ++i)
	{
		FVector contextDirection = GetOrientedContextDirection(i);
		float dotProduct = FVector::DotProduct(contextDirection, PathFollowingDirection);
		InterestMap[i] = FMath::Max(0.0f, dotProduct);

		// Add momentum bias towards previous steering direction.

		if (!PreviousSteeringDirection.IsNearlyZero())
		{
			float previousDot = FVector::DotProduct(contextDirection, PreviousSteeringDirection);
			InterestMap[i] += FMath::Max(0.0f, previousDot) * MomentumBias;
			InterestMap[i] = FMath::Clamp(InterestMap[i], 0.0f, 1.0f);
		}
	}

	// Avoidance.

	for (int32 i = 0; i < ContextSamples; ++i)
	{
		FVector contextDirection = GetOrientedContextDirection(i);

		if (DangerMap[i] > 0.0f &&
			FVector::DotProduct(contextDirection, PathFollowingDirection) > 0.66f)
		{
			int32 leftIndex = GetLeftContextDirectionSiblingIndex(i, 2);
			int32 diagonalLeftIndex = GetLeftContextDirectionSiblingIndex(i, 1);

			int32 rightIndex = GetRightContextDirectionSiblingIndex(i, 2);
			int32 diagonalRightIndex = GetRightContextDirectionSiblingIndex(i, 1);

			// Determine which side has more momentum bias.

			float leftMomentum = InterestMap[leftIndex] + InterestMap[diagonalLeftIndex];
			float rightMomentum = InterestMap[rightIndex] + InterestMap[diagonalRightIndex];

			int32* sideIndex = nullptr;
			int32* diagonalIndex = nullptr;

			if (leftMomentum > rightMomentum)
			{
				// Momentum favors left, so enhance left avoidance.

				sideIndex = &leftIndex;
				diagonalIndex = &diagonalLeftIndex;
			}
			else
			{
				// Enhance right avoidance.

				sideIndex = &rightIndex;
				diagonalIndex = &diagonalRightIndex;
			}

			if (sideIndex != nullptr && diagonalIndex != nullptr)
			{
				InterestMap[*sideIndex] = FMath::Max(InterestMap[*sideIndex], DangerMap[i] * SideAvoidanceMultiplier);
				InterestMap[*diagonalIndex] = FMath::Max(InterestMap[*diagonalIndex], DangerMap[i] * SideAvoidanceMultiplier);
			}

			// If the obstacle is too close, avoid to the opposite direction too.

			if (AvoidBackwards &&
				DangerMap[i] > HighDangerValue)
			{
				int32 oppositeIndex = GetRightContextDirectionSiblingIndex(i, ContextSamples / 2);
				InterestMap[oppositeIndex] = FMath::Max(InterestMap[oppositeIndex], DangerMap[i] * SideAvoidanceMultiplier);
			}
		}
	}
}

/**
* Fill the danger map (obstacles, enemies).
***********************************************************************************/

void UDynamicAvoidanceComponent::FillDangerMap()
{
	if (GetPawnOwner() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%hs] Pawn owner is nullptr!"), __FUNCTION__)
		return;
	}

	for (int32 i = 0; i < ContextSamples; i++)
	{
		FVector contextDirection = GetOrientedContextDirection(i);
		float dangerValue = 0.0f;

		FHitResult outHit;
		FVector startPosition = GetPawnOwner()->GetActorLocation() + (GetSurfaceNormal() * LookAheadVerticalOffset);
		FVector endPosition = startPosition + contextDirection * LookAhead;

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(GetPawnOwner());

		bool hitFound = GetWorld()->LineTraceSingleByChannel(outHit, startPosition, endPosition, DangerRayCollisionChannel, queryParams);

		if (hitFound)
		{
			// This is for ignoring top parts of ramps (for being able to use them).

			float dotProduct = FVector::DotProduct(FVector::UpVector, outHit.ImpactNormal);

			if (dotProduct >= MinAllowedRampDotProduct)
			{
				// If a valid ramp was found, just ignore it.

				dangerValue = 0.0f;
			}
			else
			{
				// The greater the ClearanceMultiplier, the sooner the actor will detect (and react to) a danger.

				float hitDistanceRatio = (outHit.Distance - (LookAhead * ClearanceMultiplier)) / (LookAhead - (LookAhead * ClearanceMultiplier));
				hitDistanceRatio = FMath::Clamp(hitDistanceRatio, 0.0f, 1.0f);
				dangerValue = 1.0f - hitDistanceRatio;

				// Only take this danger into account if it's somewhat similar to the
				// path following direction and isn't near the actor.

				float directionDotProduct = FVector::DotProduct(contextDirection, PathFollowingDirection);

				if (directionDotProduct < 0.33f &&
					dangerValue < HighDangerValue)
				{
					dangerValue = 0.0f;
				}
			}
		}

		DangerMap[i] = FMath::Max(DangerMap[i], dangerValue);
	}
}

/**
* Calculate the steering direction, considering interest and danger maps.
***********************************************************************************/

void UDynamicAvoidanceComponent::CalculateSteeringDirection()
{
	if (GetPawnOwner() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%hs] Pawn owner is nullptr!"), __FUNCTION__)
		return;
	}

	// Reset steering direction from previous frame.

	SteeringDirection = FVector::ZeroVector;

	// Find the lowest danger overall.

	float lowestDanger = 1.0f;

	for (int32 i = 0; i < ContextSamples; ++i)
	{
		if (DangerMap[i] < lowestDanger)
		{
			lowestDanger = DangerMap[i];
		}
	}

	// Mask out dangers above the lowest one.

	for (int32 i = 0; i < ContextSamples; ++i)
	{
		if (DangerMap[i] > lowestDanger)
		{
			// InterestMap[i] = FMath::Clamp(InterestMap[i] - DangerMap[i], 0.0f, 1.0f);
			InterestMap[i] = 0.0f;
		}
	}

	// Compute steering direction.

	for (int32 i = 0; i < ContextSamples; ++i)
	{
		SteeringDirection += ContextRayDirections[i] * InterestMap[i];
	}

	SteeringDirection.Normalize();

	// Temporal smoothing.

	if (!PreviousSteeringDirection.IsNearlyZero())
	{
		SteeringDirection = FMath::Lerp(PreviousSteeringDirection, SteeringDirection, 1.0f - SmoothingFactor);
		SteeringDirection.Normalize();
	}

	PreviousSteeringDirection = SteeringDirection;
}

/**
* Get the normal vector of the surface the owner is on top of.
***********************************************************************************/

FVector UDynamicAvoidanceComponent::GetSurfaceNormal()
{
	FVector result = FVector::UpVector;

	if (GetPawnOwner() != nullptr)
	{
		// Raycast down and get the surface normal.

		FHitResult outHit;
		FVector startPosition = GetPawnOwner()->GetActorLocation();
		FVector endPosition = startPosition + (-GetPawnOwner()->GetActorUpVector() * DistanceToSurfaceBelow);

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(GetPawnOwner());

		if (GetWorld()->LineTraceSingleByChannel(outHit, startPosition, endPosition, SurfaceRayCollisionChannel, queryParams))
		{
			result = outHit.ImpactNormal;
		}
		else
		{
			result = GetPawnOwner()->GetActorUpVector();
		}
	}

	return result;
}

/**
* Returns a context direction oriented using the owner up vector.
***********************************************************************************/

FVector UDynamicAvoidanceComponent::GetOrientedContextDirection(int32 index)
{
	FVector result = FVector::ZeroVector;

	if (ContextRayDirections.IsValidIndex(index))
	{
		// World-aligned.

		result = -FVector::CrossProduct(FVector::CrossProduct(ContextRayDirections[index],GetSurfaceNormal()),GetSurfaceNormal()).GetSafeNormal();
	}

	return result;
}

/**
* Get the forward vector of the owner considering the surface below.
***********************************************************************************/

FVector UDynamicAvoidanceComponent::GetForwardVectorParallelToGround()
{
	FVector result = FVector::ZeroVector;

	if (GetPawnOwner() != nullptr)
	{
		result = FVector::VectorPlaneProject(GetPawnOwner()->GetActorForwardVector(), GetSurfaceNormal()).GetSafeNormal();
	}

	return result;
}

#if !UE_BUILD_SHIPPING

/**
* Debug draw the context rays.
***********************************************************************************/

void UDynamicAvoidanceComponent::DrawContextRays()
{
	if (GetPawnOwner() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%hs] Pawn owner is nullptr!"), __FUNCTION__)
		return;
	}

	// Check if debug draw is enabled.

	static IConsoleVariable* drawContextRays = IConsoleManager::Get().FindConsoleVariable(TEXT("DynamicAvoidance.DrawContextRays"));

	if (drawContextRays == nullptr ||
		drawContextRays->GetInt() == 0)
	{
		return;
	}

	// Draw each ray.

	for (int32 i = 0; i < ContextSamples; ++i)
	{
		FVector contextDirection = GetOrientedContextDirection(i);

		FVector startPosition = GetPawnOwner()->GetActorLocation() + (contextDirection * (LookAhead * ClearanceMultiplier)) + (GetSurfaceNormal() * LookAheadVerticalOffset);
		FVector endPosition = startPosition + (contextDirection * (LookAhead - (LookAhead * ClearanceMultiplier)));

		float dotProduct = FMath::Max(0.0f,FVector::DotProduct(contextDirection, SteeringDirection));
		FColor colorA = dotProduct > 0.5f ? FColor::Orange : FColor::Red;
		FColor colorB = dotProduct > 0.5f ? FColor::Green : FColor::Orange;
		FColor debugColor = LerpColor(colorA, colorB, InterestMap[i]);

		DrawDebugLine(GetWorld(), startPosition, endPosition, debugColor, false, -1.f, 0, DebugLineThickness);
	}

	DrawDebugLine(GetWorld(), GetPawnOwner()->GetActorLocation(), GetPawnOwner()->GetActorLocation() + SteeringDirection * LookAhead, DebugSteeringDirectionColor, false, -1, 0, DebugLineThickness);
}

/**
* Debug draw the navigation path.
***********************************************************************************/

void UDynamicAvoidanceComponent::DrawNavPath()
{
	if (GetPawnOwner() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%hs] Pawn owner is nullptr!"), __FUNCTION__)
		return;
	}

	if (GetPathFollowingComponent() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%hs] Path following component is nullptr!"), __FUNCTION__)
		return;
	}

	// Check if debug draw is enabled.

	static IConsoleVariable* drawNavigationPaths = IConsoleManager::Get().FindConsoleVariable(TEXT("DynamicAvoidance.DrawNavPath"));

	if (drawNavigationPaths == nullptr ||
		drawNavigationPaths->GetInt() == 0)
	{
		return;
	}

	// Draw navigation path.

	if (GetPathFollowingComponent()->GetPath().IsValid())
	{
		FVector previousPathPointLocation = FVector::ZeroVector;

		for (const FNavPathPoint& point : GetPathFollowingComponent()->GetPath()->GetPathPoints())
		{
			if (!previousPathPointLocation.IsZero())
			{
				DrawDebugLine(GetWorld(), previousPathPointLocation, point.Location, DebugNavLineColor, false, -1, 0, DebugLineThickness);
			}

			DrawDebugSphere(GetWorld(), point.Location, DebugNavSphereRadius, 8, DebugNavSphereColor);
			previousPathPointLocation = point.Location;
		}
	}
}

/**
* Lerp color A -> B.
***********************************************************************************/

FColor UDynamicAvoidanceComponent::LerpColor(FColor colorA, FColor colorB, float t) const
{
	float newR = FMath::Lerp(colorA.R, colorB.R, t);
	float newG = FMath::Lerp(colorA.G, colorB.G, t);
	float newB = FMath::Lerp(colorA.B, colorB.B, t);
	return FColor(newR, newG, newB);
}

#endif // !UE_BUILD_SHIPPING
