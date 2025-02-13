// Fill out your copyright notice in the Description page of Project Settings.


#include "ZKWaypointTestActor.h"
#include "WaypointPathfinder.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AZKWaypointTestActor::AZKWaypointTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AZKWaypointTestActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AZKWaypointTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!InteractionPoint)
		return;

	if (!DestinationWaypoint)
		return;
	
	TArray<AActor*> AllWaypoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AZKWaypoint::StaticClass(), AllWaypoints);

	TArray<AZKWaypoint*> Path = FWaypointPathfinder::FindPathFromInteractionPoint(
		InteractionPoint->GetActorLocation(),
		AllWaypoints,
		DestinationWaypoint
	);

	// 전체 경로 디버그 표시 (상호작용 지점 포함)
	FWaypointPathfinder::DrawDebugInteractionToPath(GetWorld(), InteractionPoint->GetActorLocation(), Path);
}

