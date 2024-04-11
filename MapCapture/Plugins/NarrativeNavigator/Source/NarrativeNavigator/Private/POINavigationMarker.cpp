// Copyright Narrative Tools 2022. 


#include "POINavigationMarker.h"
#include "PointOfInterest.h"
#include "NarrativeNavigationComponent.h"

UPOINavigationMarker::UPOINavigationMarker()
{
	DefaultMarkerActionText = NSLOCTEXT("POINavigationMarker", "FastTravelText", "Fast Travel");
}

void UPOINavigationMarker::SetDiscovered(const bool bDiscovered)
{
	FLinearColor IconColor = DefaultMarkerSettings.IconTint;

	IconColor.A = bDiscovered ? 1.f : 0.5f;

	SetIconTint(IconColor);
}

bool UPOINavigationMarker::CanInteract_Implementation(class UNarrativeNavigationComponent* Selector, class APlayerController* SelectorOwner) const
{
	if (ANavigatorPointOfInterest* POIOwner = Cast<ANavigatorPointOfInterest>(GetOwner()))
	{
		if (Selector)
		{
			return Selector->HasDiscoveredPOI(POIOwner->POITags);
		}
	}

	return false; 
}
