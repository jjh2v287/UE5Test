// (c) 2021 Sergio Lena. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ActionsStatics.generated.h"

/**
 * Helper statics for running AI plans.
 */
UCLASS()
class ACTIONS_API UActionsStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * A function to start an AI plan. Called by a custom K2 node.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="Actions", meta = (BlueprintInternalUseOnly = "true"))
	static UAction * RunAction(TSubclassOf<UAction> Action);
};
