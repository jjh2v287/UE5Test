// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UKPathTestActor.generated.h"

class AUKWaypoint;
class UUKPathFindingSubsystem;

UCLASS()
class ZKGAME_API AUKPathTestActor : public AActor
{
	GENERATED_BODY()
    
public:    
	AUKPathTestActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
    
public:
	// 시작점과 끝점 액터 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing")
	AActor* EndActor;
    
	// 디버그 드로잉 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
	float DebugSphereRadius = 30.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
	float DebugArrowSize = 30.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
	FColor DebugColor = FColor::Red;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Testing|Debug")
	float DebugThickness = 1.0f;
    
	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}

private:
	void UpdatePathVisualization();
};