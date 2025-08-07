// Copyright Juan Escudero Vizcaíno 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DynamicAvoidanceComponent.generated.h"

class UPathFollowingComponent;

/**
* Dynamic Avoidance component, holding the logic for applying context steering to
* the pawn.
***********************************************************************************/

UCLASS(ClassGroup = "DynamicAvoidance", meta = (BlueprintSpawnableComponent))
class DYNAMICAVOIDANCE_API UDynamicAvoidanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// Construct a UDynamicAvoidanceComponent.
	UDynamicAvoidanceComponent();

	// Begin play for this component.
	virtual void BeginPlay() override;

	// Regular update tick.
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

public:

	// Is the context steering algorithm enabled?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool IsEnabled = true;

	// Number of rays.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (UIMin = "8", ClampMin = "8"))
	int32 ContextSamples = 8;

	// The distance to look for dangers.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Avoidance", meta = (Units = "cm", UIMin = "0.0", ClampMin = "0.0"))
	float LookAhead = 100.0f;

	// Clearance multiplier for making the actor react early to dangers. This will be applied to LookAhead. The greater the value, the earlier the actor will react to obstacles.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Avoidance", meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
	float ClearanceMultiplier = 0.25f;

	// The max angle allowed for the owner to use ramps.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Avoidance", meta = (Units = "deg", UIMin = "0.0", ClampMin = "0.0", UIMax = "360.0", ClampMax = "360.0"))
	float MaxAllowedRampAngle = 45.0f;

	// The collision channel of the ray that determines hits with dangers.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Avoidance")
	TEnumAsByte<ECollisionChannel> DangerRayCollisionChannel = ECC_WorldDynamic;

	// Bias towards previous steering direction to prevent oscillation (0 = no bias, 1 = strong bias).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", AdvancedDisplay, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
	float MomentumBias = 0.2f;

	// Smooths steering direction between frames for visual smoothness (0 = no smoothing, 1 = heavy smoothing).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", AdvancedDisplay, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
	float SmoothingFactor = 0.33f;

	// The vertical offset to apply to the look ahead starting point.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", AdvancedDisplay, meta = (Units = "cm"))
	float LookAheadVerticalOffset = 0.0f;

	// Multiplier side danger avoidance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", AdvancedDisplay, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
	float SideAvoidanceMultiplier = 0.85f;

	// Avoid backwards when an obstacle is too close?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", AdvancedDisplay)
	bool AvoidBackwards = true;

	// High danger value (0 - 1).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", AdvancedDisplay, meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
	float HighDangerValue = 0.75f;

	// The distance to look for surfaces below the owner.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Surfaces", meta = (Units = "cm"))
	float DistanceToSurfaceBelow = 100.0f;

	// The collision channel of the ray that determines hits with surfaces the owner is on top of.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Surfaces")
	TEnumAsByte<ECollisionChannel> SurfaceRayCollisionChannel = ECC_WorldDynamic;

	// The current path following direction.
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector PathFollowingDirection = FVector::ZeroVector;

	// Directions for each ray.
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TArray<FVector> ContextRayDirections;

	// Array of interests (where the owner may want to go).
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TArray<float> InterestMap;

	// Array of dangers (obstacles, enemies).
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TArray<float> DangerMap;

	// The previous steering direction..
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector PreviousSteeringDirection = FVector::ZeroVector;

	// The steering direction, calculated considering interest and danger maps.
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FVector SteeringDirection = FVector::ZeroVector;

	// Line thickness for debugging context rays and nav path.
	UPROPERTY(EditAnywhere, Category = "Debug", AdvancedDisplay, meta = (uimin = "1.0", ClampMin = "1.0"))
	float DebugLineThickness = 5.0f;

	// The color for the steering direction debug line.
	UPROPERTY(EditAnywhere, Category = "Debug", AdvancedDisplay)
	FColor DebugSteeringDirectionColor = FColor::Cyan;

	// The color for the nav path debug line.
	UPROPERTY(EditAnywhere, Category = "Debug", AdvancedDisplay)
	FColor DebugNavLineColor = FColor::Purple;

	// The color for the nav path debug spheres.
	UPROPERTY(EditAnywhere, Category = "Debug", AdvancedDisplay)
	FColor DebugNavSphereColor = FColor::Red;

	// The radius for the nav path debug spheres.
	UPROPERTY(EditAnywhere, Category = "Debug", AdvancedDisplay, meta = (uimin = "1.0", ClampMin = "1.0"))
	float DebugNavSphereRadius = 50.0f;

protected:

	// Pointer to the owner (cast to APawn).
	UPROPERTY()
	TWeakObjectPtr<APawn> PawnOwner = nullptr;

	// Pointer to the pawn owner controller.
	UPROPERTY()
	TWeakObjectPtr<AController> Controller = nullptr;

	// Pointer to the controller's path following component.
	UPROPERTY()
	TWeakObjectPtr<UPathFollowingComponent> PathFollowingComponent = nullptr;

	// Was the binding to UPathFollowingComponent.PostProcessMove succesful?
	bool WasBindingSuccessful = false;

	// Cached min dot product value for ramps.
	float MinAllowedRampDotProduct = 0.0f;

	// Get the owner of this component (cast to APawn).
	UFUNCTION(BlueprintCallable, Category = "DynamicAvoidance")
	APawn* GetPawnOwner();

	// Get the controller of the pawn owner.
	UFUNCTION(BlueprintCallable, Category = "DynamicAvoidance")
	AController* GetController();

	// Get the controller path following component.
	UFUNCTION(BlueprintCallable, Category = "DynamicAvoidance")
	UPathFollowingComponent* GetPathFollowingComponent();

	// Execute the context steering algorithm, reacting to UPathFollowingComponent.PostProcessMove.
	UFUNCTION()
	virtual void ExecuteContextSteeringAlgorithm(UPathFollowingComponent* comp, FVector& velocity);

	// Reset the danger and interest maps.
	virtual void ResetMaps();

	// Fill the interest map, considering the path following direction.
	virtual void FillInterestMap();

	// Fill the danger map (obstacles, enemies).
	virtual void FillDangerMap();

	// Calculate the steering direction, considering interest and danger maps.
	virtual void CalculateSteeringDirection();

	// Returns the index of the left sibling direction at a certain distance.
	int32 GetLeftContextDirectionSiblingIndex(int32 index, int32 siblingDistance) const
	{ return (index + ContextSamples - siblingDistance) % ContextSamples; }

	// Returns the index of the right sibling direction at a certain distance.
	int32 GetRightContextDirectionSiblingIndex(int32 index, int32 siblingDistance) const
	{ return (index + siblingDistance) % ContextSamples; }

	// Get the normal vector of the surface the owner is on top of.
	virtual FVector GetSurfaceNormal();

	// Returns a context direction oriented using the owner up vector.
	virtual FVector GetOrientedContextDirection(int32 index);

	// Get the forward vector of the owner considering the surface below.
	virtual FVector GetForwardVectorParallelToGround();

private:

#if !UE_BUILD_SHIPPING

	// Debug draw the context rays.
	void DrawContextRays();

	// Debug draw the navigation path.
	void DrawNavPath();

	// Lerp color A -> B.
	FColor LerpColor(FColor colorA, FColor colorB, float t) const;

#endif // !UE_BUILD_SHIPPING

};
