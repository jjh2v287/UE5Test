// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>
#include "NarrativeNavigationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarkerAdded, class UNavigationMarkerComponent*, Marker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarkerRemoved, class UNavigationMarkerComponent*, Marker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDiscoverPointOfInterest, class ANavigatorPointOfInterest*, POI);

/**Represents a point of interest in the world. These are stored in a data table so we can easily manage them */
USTRUCT(BlueprintType)
struct FPointOfInterest : public FTableRowBase
{
	GENERATED_BODY()

	FPointOfInterest()
	{
		DisplayName = NSLOCTEXT("PointOfInterest", "POIDisplayName", "Location Name");
		Radius = 5000.f;
		WorldLocation = FVector::ZeroVector;
	}

	/*The display name of this point of interest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Point of Interest")
	FText DisplayName;

	/*The location in the world the point of interest is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Point of Interest")
	FVector WorldLocation;

	/*The radius with which your player is considered to have reached the point of interest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Point of Interest")
	float Radius;
};

/**
Add this to your PlayerController.

To enable navigation, add this to your controller, and you're away.  */
UCLASS( ClassGroup=(Narrative), DisplayName = "Navigation Component", meta=(BlueprintSpawnableComponent) )
class NARRATIVENAVIGATOR_API UNarrativeNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	friend class UNavigationMarkerComponent;

	// Sets default values for this component's properties
	UNarrativeNavigationComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool AddMarker(class UNavigationMarkerComponent* NavMarker);
	virtual bool RemoveMarker(class UNavigationMarkerComponent* NavMarker);

	UFUNCTION(Server, Reliable)
	void ServerSelectMarker(class UNavigationMarkerComponent* Marker);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void SelectMarker(class UNavigationMarkerComponent* Marker);

	/**
	* Create a save file with all the players custom waypoints in it which can be loaded back in by calling the Load function.
	* 
	@param SaveName the name of the save game. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Saving")
	virtual bool Save(const FString& SaveName = "NarrativeNavigatorSaveData", const int32 Slot = 0);

	/**Load custom waypoints back in from disk
	@param SaveName the name of the save game. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Saving")
	virtual bool Load(const FString& SaveName = "NarrativeNavigatorSaveData", const int32 Slot = 0);

	/**Contains all the POI's we've found */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Point Of Interest")
	FGameplayTagContainer DiscoveredPOIs;

public:

	//Check if we've found the given POI in the world. Can check as many pois as you like since we're using tag container . 
	UFUNCTION(BlueprintPure, Category = "Navigation")
	virtual bool HasDiscoveredPOI(const FGameplayTagContainer& POITag) const;

	virtual bool HasDiscoveredPOI(class ANavigatorPointOfInterest* POI) const;

	virtual void HandleEnterPOI(class ANavigatorPointOfInterest* POI);

	UFUNCTION(Client, Reliable)
	virtual void ClientSetPOIDiscovered(class ANavigatorPointOfInterest* POI);

	virtual void SetPOIDiscovered(class ANavigatorPointOfInterest* POI);

	//Place a custom waypoint in the world at the given transform. bAddToSaveFile will ensure the marker is added to our save file 
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	virtual class ACustomWaypoint* PlaceCustomWaypoint(const FTransform& Transform);

	//Place a custom waypoint in the world at the given transform. bAddToSaveFile will ensure the marker is added to our save file 
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	virtual void RemoveCustomWaypoint(class ACustomWaypoint* Waypoint);

	//Our custom waypoints that we've placed. These are serialized to disk. s
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Map Configuration")
	TArray<class ACustomWaypoint*> CustomWaypoints;

	//How many unreal units wide (cm) is your map image? Enter it here. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Configuration")
	float MapWidth;

	//What position in the world is the center of your map? Enter that position here. (ie where was the camera located that you took your map shot from)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Configuration")
	FVector2D MapOrigin;

	//The max number of custom waypoints we're allowed to place in the world 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Configuration")
	int32 MaxCustomWaypoints;

	//The bounds we've found in the world, should one exist 
	UPROPERTY(BlueprintReadOnly, Category = "Map Configuration")
	class AMapTileBounds* MapTileBounds;

	//All of the markers currently being tracked by this navigation component
	UPROPERTY(BlueprintReadOnly, Category = "Navigation")
	TArray<class UNavigationMarkerComponent*> Markers;

	//Fires when a marker is added to our navigation
	UPROPERTY(BlueprintAssignable, Category = "Navigation")
	FOnMarkerAdded OnMarkerAdded;

	//Fires when a marker is removed from our navigation 
	UPROPERTY(BlueprintAssignable, Category = "Navigation")
	FOnMarkerRemoved OnMarkerRemoved;
	
	//Fires when a point of interest has been discovered, in case you want to add a message to the UI or something similar 
	UPROPERTY(BlueprintAssignable, Category = "Navigation")
	FOnDiscoverPointOfInterest OnPOIDiscovered;

};
