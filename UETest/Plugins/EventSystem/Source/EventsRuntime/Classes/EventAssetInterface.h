// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "UObject/Interface.h"
#include "EventContainer.h"
#include "EventAssetInterface.generated.h"

/** Interface for assets which contain gameplay tags */
UINTERFACE(BlueprintType, MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UEventAssetInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};
class EVENTSRUNTIME_API IEventAssetInterface
{
	GENERATED_IINTERFACE_BODY()

	/**
	 * Get any owned gameplay tags on the asset
	 * 
	 * @param OutTags	[OUT] Set of tags on the asset
	 */
	 UFUNCTION(BlueprintCallable, Category = Events)
	virtual void GetOwnedEvents(FEventContainer& TagContainer) const=0;

	/**
	 * Check if the asset has a gameplay tag that matches against the specified tag (expands to include parents of asset tags)
	 * 
	 * @param TagToCheck	Tag to check for a match
	 * 
	 * @return True if the asset has a gameplay tag that matches, false if not
	 */
	UFUNCTION(BlueprintCallable, Category=Events)
	virtual bool HasMatchingEvent(FEventInfo TagToCheck) const;

	/**
	 * Check if the asset has gameplay tags that matches against all of the specified tags (expands to include parents of asset tags)
	 * 
	 * @param TagContainer			Tag container to check for a match
	 * 
	 * @return True if the asset has matches all of the gameplay tags, will be true if container is empty
	 */
	UFUNCTION(BlueprintCallable, Category=Events)
	virtual bool HasAllMatchingEvents(const FEventContainer& TagContainer) const;

	/**
	 * Check if the asset has gameplay tags that matches against any of the specified tags (expands to include parents of asset tags)
	 * 
	 * @param TagContainer			Tag container to check for a match
	 * 
	 * @return True if the asset has matches any of the gameplay tags, will be false if container is empty
	 */
	UFUNCTION(BlueprintCallable, Category=Events)
	virtual bool HasAnyMatchingEvents(const FEventContainer& TagContainer) const;
};

