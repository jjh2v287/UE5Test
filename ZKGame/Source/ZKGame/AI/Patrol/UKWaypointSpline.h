// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "UKWaypointSpline.generated.h"

UCLASS()
class ZKGAME_API AUKWaypointSpline : public AActor
{
	GENERATED_BODY()
public:
	AUKWaypointSpline();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spline")
	USplineComponent* SplineComponent;

	// 디버그 라인 관련 속성
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugLine = true;
    
	UPROPERTY(EditAnywhere, Category = "Debug")
	FColor DebugLineColor = FColor::Blue;
    
	UPROPERTY(EditAnywhere, Category = "Debug")
	float DebugLineThickness = 2.0f;
    
	UPROPERTY(EditAnywhere, Category = "Debug")
	int32 DebugLineSegments = 32;

	void InitializeSpline(const FVector& Start, const FVector& End);

	void UpdateEndPoints(const FVector& Start, const FVector& End);
	virtual void Tick(float DeltaTime) override;
    
private:
	void DrawDebugSpline();
};
