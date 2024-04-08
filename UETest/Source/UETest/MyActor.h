// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class UETEST_API AMyActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	TSubclassOf<class AActor> SpawnType;

	FVector StartPos = FVector(200.0f, 200.0f, 0.0f);
	FVector EndPos = FVector(400.0f, -200.0f, 0.0f);
	float MaxHeight = 500.0f;
	float ElapsedTime = 0.0f;
	bool bIsMoving = true;
	float TotalTime = 1.0f; // 총 이동 시간 (1초)
	
public:	
	// Sets default values for this actor's properties
	AMyActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
