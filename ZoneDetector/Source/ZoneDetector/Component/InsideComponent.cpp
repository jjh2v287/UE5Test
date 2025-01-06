#include "InsideComponent.h"

UInsideComponent::UInsideComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInsideComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeWallPlanes();
}

void UInsideComponent::InitializeWallPlanes()
{
	FTransform BoxTransform = GetComponentTransform();
	FVector Forward = BoxTransform.GetUnitAxis(EAxis::X);
	FVector Right = BoxTransform.GetUnitAxis(EAxis::Y);
	FVector Origin = BoxTransform.GetLocation();
	FVector Extent = GetScaledBoxExtent();

	CachedWallPlanes.Empty();
	CachedWallPlanes.Add(FPlane(Origin + Forward * Extent.X, Forward));
	CachedWallPlanes.Add(FPlane(Origin - Forward * Extent.X, -Forward));
	CachedWallPlanes.Add(FPlane(Origin + Right * Extent.Y, Right));
	CachedWallPlanes.Add(FPlane(Origin - Right * Extent.Y, -Right));
}

float UInsideComponent::GetDistanceToPlane(const FPlane& Plane, const FVector& Location)
{
	return FMath::Abs(Plane.PlaneDot(Location));
}

float UInsideComponent::GetDistanceToWalls(const FVector& Location) const
{
	if (!IsLocationInside(Location))
	{
		return -1.0f;
	}

	float MinDistance = MAX_FLT;
	for (const FPlane& Plane : CachedWallPlanes)
	{
		float Distance = GetDistanceToPlane(Plane, Location);
		MinDistance = FMath::Min(MinDistance, Distance);
	}
	return MinDistance;
}

bool UInsideComponent::IsLocationInside(const FVector& Location) const
{
	FVector LocalPoint = GetComponentTransform().InverseTransformPosition(Location);
	FVector Extent = GetScaledBoxExtent();
	return FMath::Abs(LocalPoint.X) <= Extent.X &&
		   FMath::Abs(LocalPoint.Y) <= Extent.Y &&
		   FMath::Abs(LocalPoint.Z) <= Extent.Z;
}