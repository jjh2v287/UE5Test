// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/EngineTypes.h"
#include "Async_SetTimerByObject.generated.h"

class UWorld;
class UObject;

/**
 *
 */
UCLASS(meta = (HideThen = true))
class BLUEPRINTPRO_API UAsync_SetTimerByObject : public UBlueprintAsyncActionBase
{

	// Delcear delegate
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FObjectTimerHandleDelegate, UObject*, ReferenceObject, FTimerHandle, TimerHandle);

	GENERATED_BODY()

public:

	UAsync_SetTimerByObject();

	~UAsync_SetTimerByObject();

public:

	/**
	 * Set a timer to execute Completed delegate. Setting an existing timer will reset that timer with updated parameters.
	 * @param WorldContextObject		The world context.
	 * @param Object					Reference Object.
	 * @param Time						How long to wait before executing the delegate, in seconds. Setting a timer to <= 0 seconds will clear it if it is set.
	 * @param bLooping					True to keep executing the delegate every Time seconds, false to execute delegate only once.
	 * @param InitialStartDelay			Initial delay passed to the timer manager, in seconds.
	 * @param InitialStartDelayVariance	Use this to add some variance to when the timer starts in lieu of doing a random range on the InitialStartDelay input, in seconds.
	 * @return							The timer handle to pass to other timer functions to manipulate this timer.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DefaultToSelf = "Object", DisplayName = "Set Timer By Object", ScriptName = "SetTimerDelegate", AdvancedDisplay = "InitialStartDelay, InitialStartDelayVariance"), Category = "BlueprintPro|Async")
	static UAsync_SetTimerByObject* SetTimerByObject(const UObject* WorldContextObject, UObject* Object, float Time, bool bLooping, float InitialStartDelay = 0.f, float InitialStartDelayVariance = 0.f);

public: // Generate exec out pin

	/** Generate Exec Outpin, named Then */
	UPROPERTY(BlueprintAssignable)
	FObjectTimerHandleDelegate Then;

	/** Generate Exec Outpin, named Completed */
	UPROPERTY(BlueprintAssignable)
	FObjectTimerHandleDelegate OnEvent;

private: // UBlueprintAsyncActionBase interface

	virtual void Activate() override;
	//~ END UBlueprintAsyncActionBase interface

	/** UFunction for Delegate.BindUFunction; */
	UFUNCTION()
	void OnTimerEvent();

	/** Based on UKismetSystemLibrary::K2_SetTimerDelegate() */
	FTimerHandle SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay = 0.f, float InitialStartDelayVariance = 0.f);

	/** Action for garbage class instance */
	void PreGarbageCollect();

private:
	const UObject* WorldContextObject = nullptr;
	FTimerHandle TimerHandle;

	UWorld* World = nullptr;

	UPROPERTY()
	UObject* ReferObject = nullptr;
};
