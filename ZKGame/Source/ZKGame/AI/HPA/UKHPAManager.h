// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UKHPAManager.generated.h"

/**
 * 
 */
UCLASS()
class ZKGAME_API UUKHPAManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// 싱글톤 접근자
	static UUKHPAManager* Get(){ return Instance;};
    
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 웨이포인트 관련 함수들
	void RegisterWaypoint(AUKWayPoint* Waypoint);
	void UnregisterWaypoint(AUKWayPoint* Waypoint);

private:
	// 싱글톤 인스턴스
	static UUKHPAManager* Instance;
};
