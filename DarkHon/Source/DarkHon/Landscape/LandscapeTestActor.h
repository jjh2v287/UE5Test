// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LandscapeTestActor.generated.h"

UCLASS()
class DARKHON_API ALandscapeTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALandscapeTestActor();

	virtual void OnConstruction(const FTransform& Transform) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stream")
	FGuid PersistentId; // 에디터 배치 시 미리 지정 가능, 런타임 스폰 시 NewGuid()
};
