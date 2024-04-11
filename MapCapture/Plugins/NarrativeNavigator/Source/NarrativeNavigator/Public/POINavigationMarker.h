// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "NavigationMarkerComponent.h"
#include "POINavigationMarker.generated.h"

/**
 * Special Navigation marker component intended for points of interest. Includes some fast travel functionality and colour changing upon discovery 
 */
UCLASS()
class NARRATIVENAVIGATOR_API UPOINavigationMarker : public UNavigationMarkerComponent
{
	GENERATED_BODY()
	
	UPOINavigationMarker();

	virtual bool CanInteract_Implementation(class UNarrativeNavigationComponent* Selector, class APlayerController* SelectorOwner) const override;

public:

	void SetDiscovered(const bool bDiscovered);

};
