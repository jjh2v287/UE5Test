// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZKWaypoint.h"
#include "GameFramework/Actor.h"
#include "ZKWaypointTestActor.generated.h"

UCLASS()
class ZKGAME_API AZKWaypointTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AZKWaypointTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	};

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	AActor* InteractionPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	AZKWaypoint* DestinationWaypoint = nullptr;
};
