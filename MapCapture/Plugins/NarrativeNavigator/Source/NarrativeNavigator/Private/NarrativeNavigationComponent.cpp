// Copyright Narrative Tools 2022. 


#include "NarrativeNavigationComponent.h"
#include "NavigationMarkerComponent.h"
#include "MapTileBounds.h"
#include "NavigatorSaveGame.h"
#include <EngineUtils.h>
#include <Kismet/GameplayStatics.h>
#include "CustomWaypoint.h"
#include "PointOfInterest.h"
#include <Net/UnrealNetwork.h>

// Sets default values for this component's properties
UNarrativeNavigationComponent::UNarrativeNavigationComponent()
{
	SetIsReplicatedByDefault(true);
	MapWidth = 5000.f;
	MapOrigin = FVector2D::ZeroVector;
	MaxCustomWaypoints = 5;
}


void UNarrativeNavigationComponent::BeginPlay()
{
	Super::BeginPlay();

	//Try pull our navdata from the maptilebounds should one exist 
	for (TActorIterator<AMapTileBounds> It(GetWorld()); It; ++It)
	{
		MapTileBounds = *It;

		if (MapTileBounds)
		{
			MapWidth = MapTileBounds->MapWidth;
			MapOrigin = FVector2D(MapTileBounds->GetActorLocation());

			break;
		}
	}
}

void UNarrativeNavigationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UNarrativeNavigationComponent, DiscoveredPOIs, COND_OwnerOnly);
}

bool UNarrativeNavigationComponent::AddMarker(class UNavigationMarkerComponent* NavMarker)
{
	if (NavMarker)
	{
		Markers.Add(NavMarker);
		OnMarkerAdded.Broadcast(NavMarker);
		return true;
	}

	return false;
}

bool UNarrativeNavigationComponent::RemoveMarker(class UNavigationMarkerComponent* NavMarker)
{
	if (NavMarker)
	{
		Markers.Remove(NavMarker);
		OnMarkerRemoved.Broadcast(NavMarker);
		return true;
	}
	return false;
}

void UNarrativeNavigationComponent::ServerSelectMarker_Implementation(class UNavigationMarkerComponent* Marker)
{
	SelectMarker(Marker);
}

void UNarrativeNavigationComponent::SelectMarker(class UNavigationMarkerComponent* Marker)
{
	if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
	{
		if (Marker && Marker->CanInteract(this, OwnerPC))
		{
			if (GetOwnerRole() < ROLE_Authority)
			{
				ServerSelectMarker(Marker);
			}

			Marker->OnSelect(this, OwnerPC);
			Marker->OnSelected.Broadcast(this, OwnerPC);
		}
	}
}

bool UNarrativeNavigationComponent::HasDiscoveredPOI(const FGameplayTagContainer& POITag) const
{
	return DiscoveredPOIs.HasAll(POITag);
}

bool UNarrativeNavigationComponent::HasDiscoveredPOI(class ANavigatorPointOfInterest* POI) const
{
	if (POI)
	{
		return HasDiscoveredPOI(POI->POITags);
	}

	return false;
}

void UNarrativeNavigationComponent::HandleEnterPOI(class ANavigatorPointOfInterest* POI)
{
	if (GetOwnerRole() >= ROLE_Authority && POI)
	{	
		if (!HasDiscoveredPOI(POI))
		{
			DiscoveredPOIs.AppendTags(POI->POITags);
			SetPOIDiscovered(POI);
		}
	}
}

void UNarrativeNavigationComponent::ClientSetPOIDiscovered_Implementation(class ANavigatorPointOfInterest* POI)
{
	if (POI)
	{
		POI->SetDiscovered(true);
		OnPOIDiscovered.Broadcast(POI);
	}
}

void UNarrativeNavigationComponent::SetPOIDiscovered(class ANavigatorPointOfInterest* POI)
{
	if (GetOwnerRole() >= ROLE_Authority && POI)
	{
		POI->SetDiscovered(true);
		OnPOIDiscovered.Broadcast(POI);

		if (GetNetMode() != NM_Standalone)
		{
			ClientSetPOIDiscovered(POI);
		}
	}
}

ACustomWaypoint* UNarrativeNavigationComponent::PlaceCustomWaypoint(const FTransform& Transform)
{
	if (CustomWaypoints.Num() + 1 <= MaxCustomWaypoints)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.bNoFail = true;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = GetOwner();

		if (ACustomWaypoint* Waypoint = GetWorld()->SpawnActor<ACustomWaypoint>(ACustomWaypoint::StaticClass(), Transform, SpawnParams))
		{
			CustomWaypoints.AddUnique(Waypoint);

			return Waypoint;
		}
	}

	return nullptr;
}

void UNarrativeNavigationComponent::RemoveCustomWaypoint(class ACustomWaypoint* Waypoint)
{
	if (Waypoint)
	{
		CustomWaypoints.Remove(Waypoint);
		Waypoint->Destroy();
	}
}

bool UNarrativeNavigationComponent::Save(const FString& SaveName /*= "NarrativeNavigatorSaveData"*/, const int32 Slot /*= 0*/)
{
	if (UNavigatorSaveGame* NavigatorSave = Cast<UNavigatorSaveGame>(UGameplayStatics::CreateSaveGameObject(UNavigatorSaveGame::StaticClass())))
	{
		for (auto& Waypoint : CustomWaypoints)
		{
			if (Waypoint)
			{
				NavigatorSave->CustomMarkerTransforms.Add(Waypoint->GetActorTransform());
			}
		}

		NavigatorSave->DiscoveredPOIs = DiscoveredPOIs;

		if (UGameplayStatics::SaveGameToSlot(NavigatorSave, SaveName, Slot))
		{
			return true;
		}
	}
	return true;
}

bool UNarrativeNavigationComponent::Load(const FString& SaveName /*= "NarrativeNavigatorSaveData"*/, const int32 Slot /*= 0*/)
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveName, 0))
	{
		return false;
	}

	//Destroy our existings 
	for (auto& Waypoint : CustomWaypoints)
	{
		if (Waypoint)
		{
			Waypoint->Destroy();
		}
	}
	CustomWaypoints.Empty();

	if (UNavigatorSaveGame* NavigatorSave = Cast<UNavigatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveName, Slot)))
	{
		for (auto& MarkerTransform : NavigatorSave->CustomMarkerTransforms)
		{
			PlaceCustomWaypoint(MarkerTransform);
		}

		DiscoveredPOIs = NavigatorSave->DiscoveredPOIs;

		for (TActorIterator<ANavigatorPointOfInterest> It(GetWorld()); It; ++It)
		{
			if (It)
			{
				if (ANavigatorPointOfInterest* POI = *It)
				{
					POI->SetDiscovered(HasDiscoveredPOI(POI));
				}
			}
		}
		return true;
	}

	return false;
}
