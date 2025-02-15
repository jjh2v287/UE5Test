// Fill out your copyright notice in the Description page of Project Settings.


#include "UKWaypointSpline.h"


AUKWaypointSpline::AUKWaypointSpline(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
    
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
#if WITH_EDITOR
	SplineComponent->bShouldVisualizeScale = true;
	SplineComponent->ScaleVisualizationWidth = 5.0f;
#endif
	RootComponent = SplineComponent;
}

void AUKWaypointSpline::InitializeSpline(const FVector& Start, const FVector& End)
{
	SplineComponent->ClearSplinePoints(false);
	SplineComponent->AddSplinePoint(Start, ESplineCoordinateSpace::World);
	SplineComponent->AddSplinePoint(End, ESplineCoordinateSpace::World);
}

void AUKWaypointSpline::UpdateEndPoints(const FVector& Start, const FVector& End)
{
	if (SplineComponent->GetNumberOfSplinePoints() >= 2)
	{
		SplineComponent->SetLocationAtSplinePoint(0, Start, ESplineCoordinateSpace::World);
		SplineComponent->SetLocationAtSplinePoint(1, End, ESplineCoordinateSpace::World);
		SplineComponent->UpdateSpline();
	}
}

void AUKWaypointSpline::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowDebugLine)
	{
		DrawDebugSpline();
	}
}

void AUKWaypointSpline::DrawDebugSpline()
{
	if (!SplineComponent || SplineComponent->GetNumberOfSplinePoints() < 2) return;

	const float SplineLength = SplineComponent->GetSplineLength();
	const float SegmentLength = SplineLength / DebugLineSegments;

	for (int32 i = 0; i < DebugLineSegments; ++i)
	{
		const float Distance1 = i * SegmentLength;
		const float Distance2 = (i + 1) * SegmentLength;

		const FVector Location1 = SplineComponent->GetLocationAtDistanceAlongSpline(Distance1, ESplineCoordinateSpace::World);
		const FVector Location2 = SplineComponent->GetLocationAtDistanceAlongSpline(Distance2, ESplineCoordinateSpace::World);

		// 메인 라인
		DrawDebugLine(
			GetWorld(),
			Location1,
			Location2,
			DebugLineColor,
			false,  // persistent lines
			-1.0f,  // lifetime
			0,      // depth priority
			DebugLineThickness
		);

		// 방향 표시 화살표 (선택적)
		if (i % 4 == 0)  // 4개 세그먼트마다 화살표 표시
		{
			const FVector Direction = (Location2 - Location1).GetSafeNormal();
			const FVector ArrowEnd = Location1 + Direction * SegmentLength * 0.5f;
            
			DrawDebugDirectionalArrow(
				GetWorld(),
				Location1,
				ArrowEnd,
				15.0f,  // 화살표 크기
				DebugLineColor,
				false,
				-1.0f,
				0,
				DebugLineThickness
			);
		}
	}
}
