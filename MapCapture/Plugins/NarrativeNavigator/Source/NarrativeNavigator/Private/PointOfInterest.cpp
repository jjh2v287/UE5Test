// Copyright Narrative Tools 2022. 


#include "PointOfInterest.h"
#include "NarrativeNavigationComponent.h"
#include "POINavigationMarker.h"
#include <Components/SphereComponent.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>

#define LOCTEXT_NAMESPACE "PointOfInterest"

ANavigatorPointOfInterest::ANavigatorPointOfInterest()
{
	
	POISphere = CreateDefaultSubobject<USphereComponent>("POISphere");
	POISphere->SetSphereRadius(500.f);
	POISphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	POISphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(POISphere);

	POIDisplayName = LOCTEXT("POIDisplayName", "Point of Interest");

	NavigationMarker = CreateDefaultSubobject<UPOINavigationMarker>("NavigationMarker");
	NavigationMarker->DefaultMarkerSettings.LocationDisplayName = POIDisplayName;
	
}

#if WITH_EDITOR
void ANavigatorPointOfInterest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ANavigatorPointOfInterest, POIDisplayName))
	{
		if (NavigationMarker)
		{
			NavigationMarker->DefaultMarkerSettings.LocationDisplayName = POIDisplayName;
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif 

void ANavigatorPointOfInterest::BeginPlay()
{
	Super::BeginPlay();

	//On beginplay we won't be discovered, when player loads they will iterate us and set discovery 
	SetDiscovered(false);
}

void ANavigatorPointOfInterest::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (HasAuthority())
	{
		if (APawn* Pawn = Cast<APawn>(OtherActor))
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				if (UNarrativeNavigationComponent* NavComp = Cast<UNarrativeNavigationComponent>(PC->GetComponentByClass(UNarrativeNavigationComponent::StaticClass())))
				{
					NavComp->HandleEnterPOI(this);
				}
			}
		}
	}
}

void ANavigatorPointOfInterest::SetDiscovered(const bool bDiscovered)
{
	if (NavigationMarker)
	{
		NavigationMarker->SetDiscovered(bDiscovered);
	}
}

#undef LOCTEXT_NAMESPACE 