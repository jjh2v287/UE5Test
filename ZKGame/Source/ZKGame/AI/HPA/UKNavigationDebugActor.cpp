// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKNavigationDebugActor.h"
#include "DrawDebugHelpers.h"
#include "UKNavigationManager.h"
#include "UKWayPoint.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UKNavigationDebugActor)

AUKNavigationDebugActor::AUKNavigationDebugActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AUKNavigationDebugActor::BeginPlay()
{
	Super::BeginPlay();
}

void AUKNavigationDebugActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// For execution testing in the editor or simulation environment
#if WITH_EDITOR
	UpdatePathVisualization();
#endif
}

void AUKNavigationDebugActor::UpdatePathVisualization()
{
	UWorld* World = GetWorld();
	if (!World || !EndActor)
	{
		return;
	}

	UUKNavigationManager* NavigationManage = World->GetSubsystem<UUKNavigationManager>();
	if (!NavigationManage)
	{
		UE_LOG(LogTemp, Error, TEXT("AUKNavigationDebugActor: Failed to get NavigationManage instance."));
		return;
	}

	// In fact, we must call only once in Beginplay or specific events.
	// Forced re -registration and build (for test)
	NavigationManage->AllRegisterWaypoint();

	FVector StartLoc = GetActorLocation();
	FVector EndLoc = EndActor->GetActorLocation();

	TArray<FVector> RawPath = NavigationManage->FindPath(StartLoc, EndLoc);
	
	if (bDrawHPAStructure)
	{
		NavigationManage->DrawDebugHPA(DebugDrawDuration);

		DrawDebugSphere(World, StartLoc, DebugSphereRadius, 16, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
		DrawDebugSphere(World, EndLoc, DebugSphereRadius, 16, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);

		if (RawPath.Num() > 0)
		{
			// Way point route
			// DrawCentripetalCatmullRomSpline(World, RawPath, FColor::Black);
			for (int32 i = 0; i < RawPath.Num() - 1; ++i)
			{
				const FVector Offset = FVector(0.0f, 0.0f, 20.0f);
				const FVector CurrentWayPoint = RawPath[i] + Offset;
				const FVector NextWayPoint = RawPath[i + 1] + Offset;
				DrawDebugDirectionalArrow(World, CurrentWayPoint, NextWayPoint, DebugArrowSize, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
			}
		}
		else
		{
			// Start when you can't find the path-> end straight (red) mark (option)
			DrawDebugLine(World, StartLoc, EndLoc, FColor::Red, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness * 0.5f);
		}
	}
}