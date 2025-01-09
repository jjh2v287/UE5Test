// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UKAudioVolume.generated.h"

class UBoxComponent;

/**
 * 
 */
UCLASS(Blueprintable, hidecategories = (Attachment, Collision, Volume))
class UKGAME_API AUKAudioVolume : public AAudioVolume
{
	GENERATED_BODY()

	AUKAudioVolume();
public:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Wall Distance")
	FVector GetBoxExtent() const;
	
	UFUNCTION(BlueprintCallable, Category = "Wall Distance")
	float GetDistanceToWalls(const FVector& Location) const;
	
	UFUNCTION(BlueprintCallable, Category = "Wall Distance")
	bool IsLocationInside(const FVector& Location) const;

protected:
	void InitializeWallPlanes();
	static float GetDistanceToPlane(const FPlane& Plane, const FVector& Location);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(Transient)
	TArray<FPlane> CachedWallPlanes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess="true"))
	UBoxComponent* BoxComp;
};
