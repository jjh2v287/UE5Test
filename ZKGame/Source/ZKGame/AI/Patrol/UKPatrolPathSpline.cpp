// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKPatrolPathSpline.h"
#include "UKPatrolPathSubsystem.h"
#include "Components/SplineComponent.h"


AUKPatrolPathSpline::AUKPatrolPathSpline(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

void AUKPatrolPathSpline::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (UUKPatrolPathSubsystem::Get())
	{
		UUKPatrolPathSubsystem::Get()->RegisterPatrolSpline(this);
	}
}

void AUKPatrolPathSpline::BeginPlay()
{
	Super::BeginPlay();
}

void AUKPatrolPathSpline::BeginDestroy()
{
	if (UUKPatrolPathSubsystem::Get())
	{
		UUKPatrolPathSubsystem::Get()->UnregisterPatrolSpline(this);
	}
	
	Super::BeginDestroy();
}
