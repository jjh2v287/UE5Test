// Copyright Kong Studios, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/AudioVolume.h"
#include "UKAudioVolume.generated.h"

class UBoxComponent;

/**
 * 
 */
UCLASS(hidecategories=(Advanced, Attachment, Collision, Volume))
class UKGAME_API AUKAudioVolume : public AAudioVolume
{
	GENERATED_BODY()

	AUKAudioVolume();
public:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "UK AudioVolume")
	FVector GetBoxExtent() const;
	
	UFUNCTION(BlueprintCallable, Category = "UK AudioVolume")
	float GetDistanceToWalls(const FVector& Location) const;
	
	UFUNCTION(BlueprintCallable, Category = "UK AudioVolume")
	bool IsLocationInside(const FVector& Location) const;

protected:
	void InitializeWallPlanes();
	static float GetDistanceToPlane(const FPlane& Plane, const FVector& Location);

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void EditorReplacedActor(AActor* OldActor) override;
#endif // WITH_EDITOR
	
	UPROPERTY(Transient)
	TArray<FPlane> CachedWallPlanes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess="true"))
	UBoxComponent* BoxComp;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> OverlapActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UK AudioVolume")	
	float RainAttenuationDistance = 100.0f;
};
