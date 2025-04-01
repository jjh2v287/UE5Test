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
		LastFoundPath.Empty();
		return;
	}

	UUKNavigationManager* NavigationManage = World->GetSubsystem<UUKNavigationManager>();
	if (!NavigationManage)
	{
		LastFoundPath.Empty();
		UE_LOG(LogTemp, Error, TEXT("AUKNavigationDebugActor: Failed to get NavigationManage instance."));
		return;
	}

	// In fact, we must call only once in Beginplay or specific events.
	// Forced re -registration and build (for test)
	NavigationManage->AllRegisterWaypoint();

	FVector StartLoc = GetActorLocation();
	FVector EndLoc = EndActor->GetActorLocation();

	TArray<AUKWayPoint*> RawPath = NavigationManage->FindPath(StartLoc, EndLoc);
	LastFoundPath.Empty(RawPath.Num());
	
	for (AUKWayPoint* WayPoint : RawPath)
	{
		if (WayPoint)
		{
			LastFoundPath.Add(WayPoint);
		}
	}

	if (bDrawHPAStructure)
	{
		NavigationManage->DrawDebugHPA(DebugDrawDuration);

		DrawDebugSphere(World, StartLoc, DebugSphereRadius, 16, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
		DrawDebugSphere(World, EndLoc, DebugSphereRadius, 16, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);

		if (LastFoundPath.Num() > 0)
		{
			// Starting point-> First Way Point
			if (LastFoundPath[0].Get())
			{
				DrawDebugDirectionalArrow(World, StartLoc, LastFoundPath[0]->GetActorLocation(), DebugArrowSize, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
			}

			// Way point route
			for (int32 i = 0; i < LastFoundPath.Num() - 1; ++i)
			{
				AUKWayPoint* CurrentWayPoint = LastFoundPath[i].Get();
				AUKWayPoint* NextWayPoint = LastFoundPath[i + 1].Get();
				if (CurrentWayPoint && NextWayPoint)
				{
					DrawDebugDirectionalArrow(World, CurrentWayPoint->GetActorLocation(), NextWayPoint->GetActorLocation(), DebugArrowSize, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
				}
			}

			// Last Way Point-> Endpoint
			AUKWayPoint* LastWayPoint = LastFoundPath.Last().Get();
			if (LastWayPoint)
			{
				DrawDebugDirectionalArrow(World, LastWayPoint->GetActorLocation(), EndLoc, DebugArrowSize, DebugPathColor, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness);
			}
		}
		else
		{
			// Start when you can't find the path-> end straight (red) mark (option)
			DrawDebugLine(World, StartLoc, EndLoc, FColor::Red, false, DebugDrawDuration, SDPG_Foreground, DebugPathThickness * 0.5f);
		}
	}
}
