// Fill out your copyright notice in the Description page of Project Settings.


#include "UKHPAManager.h"

#include "UKWayPoint.h"


// 정적 멤버 변수 초기화
UUKHPAManager* UUKHPAManager::Instance = nullptr;

void UUKHPAManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
}

void UUKHPAManager::Deinitialize()
{
	PathGraph.Waypoints.Empty();
	Instance = nullptr;
	Super::Deinitialize();
}

void UUKHPAManager::RegisterWaypoint(AUKWayPoint* Waypoint)
{
	if (IsValid(Waypoint) && !PathGraph.Waypoints.Contains(Waypoint))
	{
		PathGraph.Waypoints.Add(Waypoint);
	}
}

void UUKHPAManager::UnregisterWaypoint(AUKWayPoint* Waypoint)
{
	if (IsValid(Waypoint) && PathGraph.Waypoints.Contains(Waypoint))
	{
		PathGraph.Waypoints.Remove(Waypoint);
	}
}