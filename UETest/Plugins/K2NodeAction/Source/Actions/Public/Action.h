// (c) 2021 Sergio Lena. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "AIController.h"
#include "UObject/NoExportTypes.h"
#include "Action.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ACTIONS_API UAction : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	

	/**
	 * Execute this  Action.
	 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void Initialize();

	/**
	 * Abort this  Action.
	 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void Abort();

	virtual UWorld* GetWorld() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		return GetStatID();
	}

protected:

	/**
	 * Gracefully finish this AI Action.
	 * @param FinishMessage		The message with which this Action has ended.
	 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void Finish(FName FinishMessage);

	/**
	 * Called when the Action should start executing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	void OnExecute();

	/**
	 * Called when the Action should start executing.
	 */
	virtual void OnExecute_Implementation() {}

	/**
	 * Called on tick.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	void OnUpdate();

	/**
	 * Called on tick.
	 */
	virtual void OnUpdate_Implementation() {}

	/**
	 * Called when the Action should finish executing
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	void OnStopExecute(bool bAborted = false);

	/**
	 * Called when the Action should finish executing
	 */
	virtual void OnStopExecute_Implementation(bool bAborted = false) {}

private:


	/**
	 * Whether this Action is currently executing. 
	 */
	UPROPERTY()
	bool bExecuting = false;

	/**
	 * Get the pawn controlled by the owner.
	 */


	bool bQueuedExecution = false;

	
};
