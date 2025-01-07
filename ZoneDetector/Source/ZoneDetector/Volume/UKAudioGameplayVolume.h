// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AudioGameplayVolume.h"
#include "UKAudioGameplayVolume.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, hidecategories = (Attachment, Collision, Volume))
class ZONEDETECTOR_API AUKAudioGameplayVolume : public AAudioGameplayVolume
{
	GENERATED_BODY()

	AUKAudioGameplayVolume();
public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
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
