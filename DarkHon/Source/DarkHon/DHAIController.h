﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DHAIController.generated.h"

UCLASS()
class DARKHON_API ADHAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADHAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
