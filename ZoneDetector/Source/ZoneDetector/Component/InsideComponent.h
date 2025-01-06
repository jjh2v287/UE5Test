// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "InsideComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZONEDETECTOR_API UInsideComponent : public UBoxComponent
{
	GENERATED_BODY()
public:    
	UInsideComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Wall Distance")
	float GetDistanceToWalls(const FVector& Location) const;
	
	UFUNCTION(BlueprintCallable, Category = "Wall Distance")
	bool IsLocationInside(const FVector& Location) const;

protected:
	void InitializeWallPlanes();
	static float GetDistanceToPlane(const FPlane& Plane, const FVector& Location);

	UPROPERTY(Transient)
	TArray<FPlane> CachedWallPlanes;
};