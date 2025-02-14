// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UKWaypoint.generated.h"

UCLASS()
class ZKGAME_API AUKWaypoint : public AActor
{
	GENERATED_BODY()
public:
	AUKWaypoint(const FObjectInitializer& ObjectInitializer);

	// 연결된 웨이포인트들의 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint")
	TArray<AUKWaypoint*> PathPoints;
};