// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoroutineHandle.generated.h"

/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class BLUEPRINTPRO_API UCoroutineHandle : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Pauses a previously set Coroutine.
	 *
	 * @param InHandle The handle of the Coroutine to pause.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Pause Coroutine"), Category = "BlueprintPro|Coroutine")
	void PauseCoroutine();

	/**
	 * Unpauses a previously set Coroutine
	 *
	 * @param InHandle The handle of the Coroutine to unpause.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "UnPause Coroutine"), Category = "BlueprintPro|Coroutine")
	void UnPauseCoroutine();


	/** Explicitly clear handle */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Invalidate"), Category = "BlueprintPro|Coroutine")
	void Invalidate();


	/**
	 * Gets the current rate (time between activations) for the specified Coroutine.
	 *
	 * @param InHandle The handle of the Coroutine to return the rate of.
	 * @return The current rate or -1.f if Coroutine does not exist.
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Progress"), Category = "BlueprintPro|Coroutine")
	float GetCoroutineProgress()
	{
		return Progress;
	}

	void SetProgress(float Value);


	/**
	* Returns true if the specified Coroutine exists and is paused
	*
	* @param InHandle The handle of the Coroutine to check for being paused.
	* @return true if the Coroutine exists and is paused, false otherwise.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Is Paused"), Category = "BlueprintPro|Coroutine")
	bool IsCoroutinePaused();


	/**
	* Returns true if the specified Coroutine exists
	*
	* @param InHandle The handle of the Coroutine to check for existence.
	* @return true if the Coroutine exists, false otherwise.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Is Exists"), Category = "BlueprintPro|Coroutine")
	bool IsCoroutineExists();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Reference Object"), Category = "BlueprintPro|Coroutine")
	UObject* GetReferenceObject()
	{
		return RefenceObject;
	}

	void SetReferenceObject(UObject* Object, UObject* AsyncNode)
	{
		RefenceObject = Object;
		AsyncCoroutineAdvance = AsyncNode;
	}

public: //c++ internal
	void Reset();

	bool IsTickable()
	{
		return bTickable;
	}

	void EnableTick(bool bIsEnable)
	{
		bTickable = bIsEnable;
	}

private:
	UPROPERTY()
	UObject* RefenceObject = nullptr;

	UPROPERTY()
	UObject* AsyncCoroutineAdvance = nullptr;

	bool bTickable = false;

	bool bPaused = false;

	float Progress = 0.0f;
};
