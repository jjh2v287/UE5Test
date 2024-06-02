// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/EngineTypes.h"
#include "Runtime/Launch/Resources/Version.h"

#if ( ENGINE_MAJOR_VERSION == 5 &&  1< ENGINE_MINOR_VERSION )
#include "Engine/TimerHandle.h"
#endif

#include "Async_SetTimer.generated.h"

class UWorld;

// Declare General Log Category, header file .h
DECLARE_LOG_CATEGORY_EXTERN(LogAsyncSetTimer, Log, All);


/**
 *
 */
UCLASS(meta = (HideThen = true))
class BLUEPRINTPRO_API UAsync_SetTimer : public UBlueprintAsyncActionBase
{
	// Delcear delegate for finished delay
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTimerHandleDelegate, FTimerHandle, TimerHandle);

	GENERATED_BODY()

public:

	UAsync_SetTimer();

	~UAsync_SetTimer();

public:

	/**
	 * Set a timer to execute Completed delegate. Setting an existing timer will reset that timer with updated parameters.
	 * @param WorldContextObject		The world context.
	 * @param Time						How long to wait before executing the delegate, in seconds. Setting a timer to <= 0 seconds will clear it if it is set.
	 * @param bLooping					True to keep executing the delegate every Time seconds, false to execute delegate only once.
	 * @param InitialStartDelay			Initial delay passed to the timer manager, in seconds.
	 * @param InitialStartDelayVariance	Use this to add some variance to when the timer starts in lieu of doing a random range on the InitialStartDelay input, in seconds.
	 * @return							The timer handle to pass to other timer functions to manipulate this timer.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Set Timer", ScriptName = "SetTimerDelegate", AdvancedDisplay = "InitialStartDelay, InitialStartDelayVariance"), Category = "BlueprintPro|Async")
	static UAsync_SetTimer* SetTimer(const UObject* WorldContextObject, float Time, bool bLooping, float InitialStartDelay = 0.f, float InitialStartDelayVariance = 0.f);

public: // Generate exec out pin

	/** Generate Exec Outpin, named Then */
	UPROPERTY(BlueprintAssignable)
	FTimerHandleDelegate Then;

	/** Generate Exec Outpin, named Completed */
	UPROPERTY(BlueprintAssignable)
	FTimerHandleDelegate OnEvent;

private: // UBlueprintAsyncActionBase interface

	virtual void Activate() override;
	//~ END UBlueprintAsyncActionBase interface

	/** UFunction for Delegate.BindUFunction; */
	UFUNCTION()
	void CompletedEvent();

	/** Based on UKismetSystemLibrary::K2_SetTimerDelegate() */
	FTimerHandle SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay = 0.f, float InitialStartDelayVariance = 0.f);

	/** Action for garbage class instance */
	void PreGarbageCollect();

private:
	const UObject* WorldContextObject;
	FTimerHandle TimerHandle;

	UWorld* World;
};
