// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Async_CreateCoroutineAdvance.generated.h"


class UCoroutor;
class UCoroutineHandle;

/**
 * 
 */
UCLASS()
class BLUEPRINTPRO_API UAsync_CreateCoroutineAdvance : public UBlueprintAsyncActionBase, public FTickableGameObject
{
	/**
	 *	multicast:In the case of multicast delegates, any number of entities within your code base can respond to the same event and receive the inputs and use them.
	 *	dynamic:In the case of dynamic delegates, the delegate can be saved / loaded within a Blueprint graph(they're called Events/Event Dispatcher in BP).
	*/
	// Delcear delegate for finished delay
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoroutineDelegate, UCoroutineHandle*, CoroutineHandle);

	/* Changed GENERATED_BODY() to GENERATED_UCLASS_BODY() to create a constructor to reset variables in */
	GENERATED_UCLASS_BODY()

public: // Generate exec out pin

	UPROPERTY(BlueprintAssignable)
	/** Generate Exec Outpin, named Start */
	FCoroutineDelegate Start;
	UPROPERTY(BlueprintAssignable)
	/** Generate Exec Outpin, named Update */
	FCoroutineDelegate Update;
	UPROPERTY(BlueprintAssignable)
	/** Generate Exec Outpin, named End */
	FCoroutineDelegate End;

public: // Generate blueprint node

	/**
	 * Create a latent coroutine node,execution order：Start pin -> Update pin -> End pin
	 * @param WorldContextObject Current Object to use this use async node.
	 * @param Object	Reference Object.
	 * @param Delay, time before begin update
	 * @param Duration, time of update event exection
	 * @return This Async_CreateCoroutine blueprint node
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DefaultToSelf = "Object", DisplayName = "Create Coroutine(Advance)", Keywords = "Async,Coroutine", ScriptName = "SetTimerDelegate"), Category = "BlueprintPro|Coroutine")
	static UAsync_CreateCoroutineAdvance* CreateCoroutineAdvance(const UObject* WorldContextObject, UObject* Object, const float Delay = 0.0f, const float Duration = 1.0f);


private: // Base class interface

	//--->> UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	//~UBlueprintAsyncActionBase interface

	// --->>>Begin FTickableGameObject Interface.
	// Deltatime, game time passed since the last call.
	virtual void Tick(float DeltaTime) override;
	// Return true if object is ready to be ticked, false otherwise.
	virtual TStatId GetStatId() const override;

	// Whether to open tick, return	true if object is ready to be ticked, false otherwise.
	virtual bool IsTickable() const override;
	// If this tickable object can be ticked in the editor
	virtual bool IsTickableInEditor()const override;
	// --->>> End FTickableGameObject Interface.

private:
	UWorld* GetGWorld();

	// Coroutor object for save variable, and exposed to blueprint
	UPROPERTY()
	UObject* ReferObject = nullptr;

	UPROPERTY()
	UCoroutineHandle* CoroutineHandle = nullptr;

	// Delay time for calling the OnUpdate function
	float DelayTime = 0.0f;

	// Total time of tick event execution
	float DurationTime = 1.0f;

	// Elapsed Time of tick event execution
	float ElapsedTime = 0.0f;

	bool bTickable = false;

	bool bCallStartEvent = false;

};
