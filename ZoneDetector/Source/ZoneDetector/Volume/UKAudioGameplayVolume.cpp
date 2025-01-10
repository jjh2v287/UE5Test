// Fill out your copyright notice in the Description page of Project Settings.


#include "UKAudioGameplayVolume.h"

#include "Components/BrushComponent.h"
#include "Kismet/GameplayStatics.h"

AUKAudioGameplayVolume::AUKAudioGameplayVolume()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AUKAudioGameplayVolume::BeginPlay()
{
	Super::BeginPlay();
	InitializeWallPlanes();
	SetActorTickEnabled(false);
}

void AUKAudioGameplayVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	float Length = GetDistanceToWalls(Pawn->GetActorLocation());

	UKismetSystemLibrary::PrintString(this, FString::SanitizeFloat(Length), true, true, FLinearColor::Black,0.0f);
}

void AUKAudioGameplayVolume::InitializeWallPlanes()
{
	const FTransform BoxTransform = GetBrushComponent()->GetComponentTransform();
	const FVector Forward = BoxTransform.GetUnitAxis(EAxis::X);
	const FVector Right = BoxTransform.GetUnitAxis(EAxis::Y);
	const FVector Origin = BoxTransform.GetLocation();
	const FVector Extent = GetBrushComponent()->Bounds.GetBox().GetExtent();

	CachedWallPlanes.Empty();
	CachedWallPlanes.Add(FPlane(Origin + Forward * Extent.X, Forward));
	CachedWallPlanes.Add(FPlane(Origin - Forward * Extent.X, -Forward));
	CachedWallPlanes.Add(FPlane(Origin + Right * Extent.Y, Right));
	CachedWallPlanes.Add(FPlane(Origin - Right * Extent.Y, -Right));
}

float AUKAudioGameplayVolume::GetDistanceToPlane(const FPlane& Plane, const FVector& Location)
{
	return FMath::Abs(Plane.PlaneDot(Location));
}

float AUKAudioGameplayVolume::GetDistanceToWalls(const FVector& Location) const
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

bool AUKAudioGameplayVolume::IsLocationInside(const FVector& Location) const
{
	const FTransform BoxTransform = GetBrushComponent()->GetComponentTransform();
	const FVector LocalPoint = BoxTransform.InverseTransformPosition(Location);
	const FVector Extent = GetBrushComponent()->Bounds.GetBox().GetExtent();
	const FVector ActorScale = GetActorScale();
	return FMath::Abs(LocalPoint.X) <= Extent.X / ActorScale.X &&
		   FMath::Abs(LocalPoint.Y) <= Extent.Y / ActorScale.Y &&
		   FMath::Abs(LocalPoint.Z) <= Extent.Z / ActorScale.Z;
}