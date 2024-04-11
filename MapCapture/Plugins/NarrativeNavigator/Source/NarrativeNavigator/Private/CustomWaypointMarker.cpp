// Copyright Narrative Tools 2022. 


#include "CustomWaypointMarker.h"
#include "CustomWaypoint.h"
#include "NarrativeNavigationComponent.h"

UCustomWaypointMarker::UCustomWaypointMarker()
{
	DefaultMarkerSettings.LocationDisplayName = NSLOCTEXT("CustomWaypointMarker", "MarkerDisplayName", "Custom Waypoint");
	DefaultMarkerSettings.IconTint = FLinearColor(0.178507f, 0.538802f, 0.859375f,1.000000f);

	bPinToMapEdge = true;

	VisibilityFlags |= StaticCast<int32>(ENavigatorType::Compass);
	VisibilityFlags |= StaticCast<int32>(ENavigatorType::Map);
	VisibilityFlags |= StaticCast<int32>(ENavigatorType::Minimap);
	VisibilityFlags |= StaticCast<int32>(ENavigatorType::Screenspace);
}

void UCustomWaypointMarker::OnSelect_Implementation(class UNarrativeNavigationComponent* Selector, class APlayerController* SelectorOwner)
{
	Super::OnSelect_Implementation(Selector, SelectorOwner);

	if (Selector)
	{
		if (ACustomWaypoint* Waypoint = Cast<ACustomWaypoint>(GetOwner()))
		{
			Selector->RemoveCustomWaypoint(Waypoint);
		}
;	}
}

FText UCustomWaypointMarker::GetMarkerActionText_Implementation(class UNarrativeNavigationComponent* Selector, class APlayerController* SelectorOwner) const
{
	return NSLOCTEXT("CustomWaypointMarker", "MarkerActionText", "Remove");
}
