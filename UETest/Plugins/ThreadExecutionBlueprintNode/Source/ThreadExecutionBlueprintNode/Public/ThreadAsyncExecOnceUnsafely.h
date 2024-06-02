// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ThreadAsyncExecBase.h"
#include "ThreadAsyncExecOnceUnsafely.generated.h"

/**
 * 
 */
UCLASS()
class THREADEXECUTIONBLUEPRINTNODE_API UThreadAsyncExecOnceUnsafely : public UThreadAsyncExecBase
{
	GENERATED_BODY()

public:
	virtual void Execution() override;

protected:
	virtual void CompletedExecInGameThread() override;
	
	uint8 bBlockGC:1;
	
public:
	// Node execution pin Begin

	/*Event streams that will be executed in other threads*/
	UPROPERTY(BlueprintAssignable, DisplayName="Execution")
	FSimpleDynamicMuticastDelegate OnExecution;

	/*Execute the Completed execution stream in the main game thread after Execution completes*/
	UPROPERTY(BlueprintAssignable, DisplayName="Completed")
	FSimpleDynamicMuticastDelegate OnCompleted;
	
	// Node execution pin End

	// Blueprint function
	/**
	 * Create a thread and execute it once
	 * Repeated execution will create multiple threaded tasks instead of waiting for the previous threaded task to finish executing
	 * @param bLongTask - If true, a separate thread is created with continuously executing or long execution time tasks.
	 * If false, create tasks in the task graph, with short tasks.
	 * The performance consumption of creating short tasks is the least
	 * @param ThreadName - The name of the created thread. If the default value is None, the created thread is a non-None default.
	 * If it is not the default value None, the name of the created thread will be that value.
	 */
	UFUNCTION(BlueprintCallable, Category="Thread|Once",
		meta=(BlueprintInternalUseOnly=true, AdvancedDisplay="bLongTask,ThreadName"))
	static UThreadAsyncExecOnceUnsafely* CreateThreadExecOnceUnsafely(bool bLongTask=false,FName ThreadName=NAME_None,bool bBlockGC=true);
};
