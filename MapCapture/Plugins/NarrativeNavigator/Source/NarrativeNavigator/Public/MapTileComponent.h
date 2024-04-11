// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "NavigationMarkerComponent.h"
#include "MapTileComponent.generated.h"

/**
 * World maps have a big problem - resolution. Even if your map is 50000x50000px, which is probably going to overwhelm a lot of
 * machines, in an open world map you won't have to zoom in far before the map becomes blurry and looks terrible. 
 *
 * In order to solve this, we split the map into tiles, which are generated via a utility widget that ships with navigator. 
 * Each tile is a top down shot of the map with U units of width, and we take X by Y shots until we generate an entire grid of shots. 
 * 
 * The next problem is stitching them all together, and only rendering tiles that are actually within the maps bounds. Hang on, we 
 * already have something for that - map markers! The map tiles are essentially just big map markers, and so by making them map markers
 * we take advantage of them only being rendered if they are within the maps bounds as well as other marker features. Nice! 
 */
UCLASS( ClassGroup=(Narrative), Blueprintable, DisplayName = "Map Tile Marker", meta=(BlueprintSpawnableComponent) )
class NARRATIVENAVIGATOR_API UMapTileComponent : public UNavigationMarkerComponent
{
	GENERATED_BODY()

	UMapTileComponent();
	
	virtual bool CanInteract_Implementation(class UNarrativeNavigationComponent* Selector, class APlayerController* SelectorOwner) const;

public:

	//The zoom level at which this map tile streams in. IE 5 = the map needs to be at 5x zoom before the tile streams in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marker Setup")
	float LODFadeInLevel;

};
