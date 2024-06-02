// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Async_SetTimerForNextTick.generated.h"

/**
 *
 */
UCLASS(meta = (HideThen = true))
class BLUEPRINTPRO_API UAsync_SetTimerForNextTick : public UBlueprintAsyncActionBase
{
	// Delcear delegate
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectHandleDelegate, UObject*, ReferenceObject);

	GENERATED_BODY()

public:

	UAsync_SetTimerForNextTick();

	~UAsync_SetTimerForNextTick();

public:

	/**
	 * Set a timer to execute Completed delegate. Setting an existing timer will reset that timer with updated parameters.
	 * @param WorldContextObject		The world context.
	 * @param Object		ReferenceObject
	 * @return							The timer handle to pass to other timer functions to manipulate this timer.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DefaultToSelf = "Object", DisplayName = "Set Timer For Next Tick"), Category = "BlueprintPro|Async")
	static UAsync_SetTimerForNextTick* SetTimerForNextTick(const UObject* WorldContextObject, UObject* Object);

public: // Generate exec out pin

	/** Generate Exec Outpin, named Then */
	UPROPERTY(BlueprintAssignable)
	FObjectHandleDelegate Then;

	/** Generate Exec Outpin, named Completed */
	UPROPERTY(BlueprintAssignable)
	FObjectHandleDelegate OnEvent;

private: // UBlueprintAsyncActionBase interface

	virtual void Activate() override;
	//~ END UBlueprintAsyncActionBase interface

	/** UFunction for Delegate.BindUFunction; */
	UFUNCTION()
	void CompletedEvent();

private:
	UPROPERTY()
	const UObject* WorldContextObject = nullptr;

	UPROPERTY()
	UObject* ReferObject = nullptr;

	FTimerHandle TimerHandle;

	UWorld* World = nullptr;
};
