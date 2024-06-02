// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "ThreadAsyncExecBase.h"
#include "ThreadAsyncExecTickUnsafely.generated.h"

/*@Param TickHandle - A reference to the object of the Tick thread. You can operate on the Tick through this reference*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTick, float, DeltaSeconds, UThreadAsyncExecTickUnsafely*, TickHandle);

/**
 * 
 */
UCLASS()
class THREADEXECUTIONBLUEPRINTNODE_API UThreadAsyncExecTickUnsafely : public UThreadAsyncExecBase,
                                                                      public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Execution() override;
	virtual void BeginDestroy() override;

protected:
	virtual void BeginDestroyButFutureNotReady() override;

	virtual void CompletedExecInGameThread() override;

public:
	/*Event streams that will be executed per Tick*/
	UPROPERTY(BlueprintAssignable, DisplayName="Tick")
	FOnTick OnTick;
	/*Completed event stream executed at the end of Tick execution*/
	UPROPERTY(BlueprintAssignable, DisplayName="Completed")
	FSimpleDynamicMuticastDelegate OnCompleted;

	uint8 bIsTickEnabled:1;
	uint8 bIsTickableWhenPaused:1;

protected:
	FEvent* TickEvent;
	float DeltaSeconds;

	uint8 bBlockGC:1;

public:
	// FTickableGameObject implementation Begin
	virtual TStatId GetStatId() const override { return TStatId(); } // Need to custom

	UFUNCTION(BlueprintPure, Category="Thread|Tick", meta=(Keywords="Get"))
	virtual bool IsTickable() const override
	{
		return !IsTemplate() && HasExecMask(EThreadAsyncExecTag::DoExecution) && bIsTickEnabled;
	}

	UFUNCTION(BlueprintPure, Category="Thread|Tick", meta=(Keywords="Get"))
	virtual bool IsTickableWhenPaused() const override { return bIsTickableWhenPaused; }

	virtual void Tick(float DeltaTime) override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); } // Need to custom
	virtual ETickableTickType GetTickableTickType() const override
	{
		return IsTemplate() ? ETickableTickType::Never : FTickableGameObject::GetTickableTickType();
	}

	// FTickableGameObject implementation End

	/**
	 * Create threads and execute them at each tick
	 * Repeated execution will create multiple threaded tasks instead of waiting for the previous threaded task to finish executing
	 * @param TickEnabled - Whether to execute Tick event flow per Tick.
	 * If true, the Tick is executed after the thread is created;
	 * if false, the Tick is not executed after the thread is created.
	 * @param TickWhenPaused - Whether to execute Tick when paused
	 * @param bLongTask - If true, a separate thread is created with continuously executing or long execution time tasks.
	 * If false, create tasks in the task graph, with short tasks.
	 * The performance consumption of creating short tasks is the least
	 */
	UFUNCTION(BlueprintCallable, Category="Thread|Tick",
		meta=(BlueprintInternalUseOnly=true, AdvancedDisplay="TickEnabled,TickWhenPaused,bLongTask,ThreadName"))
	static UThreadAsyncExecTickUnsafely* CreateThreadExecTickUnsafely
	(
		bool TickEnabled = true,
		bool TickWhenPaused = false,
		bool bLongTask = false,
		FName ThreadName = NAME_None,
		bool bBlockGC = true
	);

public:
	/**
	 * Interrupts the next Tick.
	 * if a Tick is currently being executed, it will pop up before the next Tick.
	 */
	UFUNCTION(BlueprintCallable, Category="Thread|Tick",
		meta=(CompactNodeTitle="BREAK TICK")
	)
	void BreakNextTick() { ClearExecMask(EThreadAsyncExecTag::DoExecution); }

	/**/
	UFUNCTION(BlueprintCallable, Category="Thread|Tick")
	void SetTickable(bool NewValue) { bIsTickEnabled = NewValue; }
	/**/
	UFUNCTION(BlueprintCallable, Category="Thread|Tick")
	void SetTickableWhenPaused(bool NewValue) { bIsTickableWhenPaused = NewValue; }
};
