// Copyright Narrative Tools 2022. 


#include "MapTileComponent.h"

UMapTileComponent::UMapTileComponent()
{
	DefaultMarkerSettings.bOverride_bShowActorRotation = true;

	//Map tiles only need to show on the map navigators 
	VisibilityFlags = 0;
	VisibilityFlags |= StaticCast<int32>(ENavigatorType::Map);
	VisibilityFlags |= StaticCast<int32>(ENavigatorType::Minimap);

	//By default, the map tile should never show - the LOD gets set by the utility widget 
	LODFadeInLevel = 0.f;

}

bool UMapTileComponent::CanInteract_Implementation(class UNarrativeNavigationComponent* Selector, class APlayerController* SelectorOwner) const
{
	//Map tiles are static icons that we definitely dont want to interact with in any way - they are purely visual 
	return false;
}
