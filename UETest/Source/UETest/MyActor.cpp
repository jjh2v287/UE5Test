// Fill out your copyright notice in the Description page of Project Settings.


#include "MyActor.h"

// Sets default values
AMyActor::AMyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();

	// UBlueprint* ObjectToSpawn = Cast<UBlueprint>(SpawnType);
	//if(ObjectToSpawn)
	{
		FActorSpawnParameters spawnParams;
		FRotator rotator;
		FVector spawnLocation = FVector(100.0f, 0.0f, 0.0f);
		GetWorld()->SpawnActor<AActor>(SpawnType, spawnLocation, rotator, spawnParams);
	}

	ElapsedTime = 0.0f;
	bIsMoving = true;
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsMoving)
	{
		return;
	}

	if (!bIsMoving || TotalTime <= 0.f)
	{
		return;
	}

	ElapsedTime += DeltaTime;
	if (ElapsedTime >= TotalTime)
	{
		ElapsedTime = TotalTime;
		bIsMoving = false;
	}

	float Fraction = ElapsedTime / TotalTime; // Current fraction of the total time elapsed
	FVector CurrentPos = FMath::Lerp(StartPos, EndPos, Fraction);
	float CurrentZ = FMath::Lerp(StartPos.Z, StartPos.Z + MaxHeight, ElapsedTime);
	if(ElapsedTime > TotalTime/2)
		CurrentZ = FMath::Lerp(StartPos.Z + MaxHeight, StartPos.Z, ElapsedTime);
	
	// Adjust the coefficient `a` for the new total time
	float MidTime = TotalTime / 2.0f;
	float a = -4 * MaxHeight / FMath::Pow(MidTime, 2);
	float z = a * FMath::Pow((ElapsedTime - MidTime), 2) + MaxHeight;

	// Assuming the start and end Z positions are the same, adjust for vertical movement
	CurrentPos.Z = StartPos.Z + (z - StartPos.Z) * Fraction;
	CurrentPos.Z = CurrentZ;

	SetActorLocation(CurrentPos);

	if (ElapsedTime >= TotalTime)
	{
		ElapsedTime = 0.0f; // Optionally reset to allow for movement to be restarted.
		bIsMoving = false;
	}
}

