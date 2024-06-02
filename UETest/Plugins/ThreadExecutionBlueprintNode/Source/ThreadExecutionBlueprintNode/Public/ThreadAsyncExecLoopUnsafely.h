// Copyright YTSS 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AsyncExecutionBlueprintTypes.h"
#include "ThreadAsyncExecBase.h"
#include "Async/Async.h"
#include "ThreadAsyncExecLoopUnsafely.generated.h"

/*@param LoopHandle - A reference to the object of the Loop thread. You can operate on this Loop through this reference*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExecLoopBody, class UThreadAsyncExecLoopUnsafely*, LoopHandle);

/**
 * 
 */
UCLASS()
class THREADEXECUTIONBLUEPRINTNODE_API UThreadAsyncExecLoopUnsafely : public UThreadAsyncExecBase

{
	GENERATED_BODY()

public:
	virtual void Execution() override;

protected:
	virtual void CompletedExecInGameThread() override;

public:
	// Node execution pin Begin

	/*Event streams executed in a loop in other threads*/
	UPROPERTY(BlueprintAssignable, DisplayName="LoopBody")
	FOnExecLoopBody OnLoopBody;

	/*Completed execution stream executed at the end of LoopBody execution*/
	UPROPERTY(BlueprintAssignable, DisplayName="Completed")
	FSimpleDynamicMuticastDelegate OnCompleted;

	// Node execution pin End

protected:
	double Interval;
	uint8 bBlockGC:1;

public:
	// Blueprint function
	/**
	 * Create other threads and execute a loop
	 * Repeated execution will create multiple threaded tasks instead of waiting for the previous threaded task to finish executing
	 * @param Interval - The interval wait time between loops. This is done to avoid thread blocking.
	 * If the value is 0 should also be fine (no test out what the problem is for now)
	 * @param bLongTask - If true, a separate thread is created with continuously executing or long execution time tasks.
	 * If false, create tasks in the task graph, with short tasks.
	 * The performance consumption of creating short tasks is the least
	 */
	UFUNCTION(BlueprintCallable, Category="Thread|Loop",
		meta=(BlueprintInternalUseOnly=true, AdvancedDisplay="bLongTask,ThreadName"))
	static UThreadAsyncExecLoopUnsafely* CreateThreadExecLoopUnsafely
	(
		float Interval = 0.001f,
		bool bLongTask = false,
		FName ThreadName = NAME_None,
		bool bBlockGC = true
	);

public:
	/**
	 * Interrupt the next cycle.
	 * If the current loop is executing,
	 * it will not jump out of the current loop,
	 * but will jump out before executing the next loop.
	 */
	UFUNCTION(BlueprintCallable, Category="Thread|Loop", meta=(CompactNodeTitle="BREAK LOOP"))
	void BreakNextLoop() { ClearExecMask(EThreadAsyncExecTag::DoExecution); }
};
