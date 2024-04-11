// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include <GameplayTagContainer.h>
#include "NavigatorSaveGame.generated.h"


/**
 * 
 */
UCLASS()
class NARRATIVENAVIGATOR_API UNavigatorSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:

	UPROPERTY()
	TArray<FTransform> CustomMarkerTransforms; 

	UPROPERTY()
	FGameplayTagContainer DiscoveredPOIs;

};
