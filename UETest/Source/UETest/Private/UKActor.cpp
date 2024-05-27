// Fill out your copyright notice in the Description page of Project Settings.


#include "UKActor.h"

// Sets default values
AUKActor::AUKActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUKActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUKActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

