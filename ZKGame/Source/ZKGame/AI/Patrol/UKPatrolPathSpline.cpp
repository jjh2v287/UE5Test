// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKPatrolPathSpline.h"
#include "UKPatrolPathManager.h"
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
	if (UUKPatrolPathManager::Get())
	{
		UUKPatrolPathManager::Get()->RegisterPatrolSpline(this);
	}
}

void AUKPatrolPathSpline::BeginPlay()
{
	Super::BeginPlay();
}

void AUKPatrolPathSpline::BeginDestroy()
{
	if (UUKPatrolPathManager::Get())
	{
		UUKPatrolPathManager::Get()->UnregisterPatrolSpline(this);
	}
	
	Super::BeginDestroy();
}
