// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include <GameplayTagContainer.h>
#include "PointOfInterest.generated.h"

/**
 * Represents a point of interest that can be placed in the level. When players enter the POI sphere, their navigation components will "discover" this POI,
 * by adding its POI tag to their list of discovered areas. This list will saved to disk, and when load is called all POIs in the level will automatically be updated. 
 */
UCLASS(Blueprintable)
class NARRATIVENAVIGATOR_API ANavigatorPointOfInterest : public AActor
{
	GENERATED_BODY()
	
public:

	friend class UNarrativeNavigationComponent;

	ANavigatorPointOfInterest();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USphereComponent* POISphere;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UPOINavigationMarker* NavigationMarker;

	/** The tag for this location, generally you'd just have one but more can technically be added */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Point Of Interest")
	FGameplayTagContainer POITags;

	/** The display name for this poi when we display it in the UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Point Of Interest")
	FText POIDisplayName;

	/** The location in the world we'll fast travel the player to when the POI is selected in the world map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Point Of Interest", meta = (MakeEditWidget))
	FTransform FastTravelLocation;

protected:

#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void SetDiscovered(const bool bDiscovered);

};
