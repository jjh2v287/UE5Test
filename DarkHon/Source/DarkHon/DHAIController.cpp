// Fill out your copyright notice in the Description page of Project Settings.


#include "DHAIController.h"

#include "NavigationSystem.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/MetaNavMeshPath.h"

ADHAIController::ADHAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ADHAIController::BeginPlay()
{
	Super::BeginPlay();

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		const FVector AgentNavLocation = GetNavAgentLocation();
		const ANavigationData* NavData = NavSys->GetNavDataForProps(GetNavAgentPropertiesRef(), AgentNavLocation);
		if (NavData)
		{
			const FVector StartLocation = GetPawn()->GetActorLocation();
			const FVector GoalLocation = GetPawn()->GetActorLocation() + (GetPawn()->GetActorForwardVector() * 1000.f);
			FPathFindingQuery Query(this, *NavData, AgentNavLocation, GoalLocation);
			FPathFindingResult Result = NavSys->FindPathSync(Query);
			// if (Result.IsSuccessful())
			{
				TArray<FVector> InWaypoints = {StartLocation, GoalLocation};
				TSharedRef<FMetaNavMeshPath> Path = MakeShareable(new FMetaNavMeshPath(InWaypoints, *this));
				// FNavPathSharedPtr
				// FNavigationPath
				// FNavMeshPath
				// FMetaNavMeshPath
				Path->PathCorridor.Add(1);
				Path->PathCorridorCost.Add(1);
				GetPathFollowingComponent()->RequestMove(FAIMoveRequest(GoalLocation), Path);
			}
		}
	}
}

// Called every frame
void ADHAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

