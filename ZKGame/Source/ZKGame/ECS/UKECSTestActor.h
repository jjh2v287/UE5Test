// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UKECSSystemBase.h"
#include "GameFramework/Actor.h"
#include "UKECSEntity.h"
#include "StructUtils/InstancedStruct.h"
#include "UKECSTestActor.generated.h"

UCLASS()
class ZKGAME_API AUKECSTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AUKECSTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	FECSEntityHandle Entity;
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly ,Category = "ECS")
	TArray<TSubclassOf<UUKECSSystemBase>> ECSSystemBaseClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ECS", meta = (BaseStruct = "/Script/ZKGame.UKECSComponentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> ECSComponentInstances;
};
