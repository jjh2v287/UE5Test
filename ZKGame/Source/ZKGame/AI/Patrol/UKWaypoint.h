// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKWaypointSpline.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "UKWaypoint.generated.h"

UCLASS()
class ZKGAME_API AUKWaypoint : public AActor
{
	GENERATED_BODY()
public:
	AUKWaypoint(const FObjectInitializer& ObjectInitializer);
    
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	virtual void Tick(float DeltaTime) override;

	// 연결된 웨이포인트들의 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint")
	TArray<AUKWaypoint*> PathPoints;

	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	};

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void DrawDebugLines();
#endif
};