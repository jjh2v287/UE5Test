// Copyright YTSS 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AsyncExecutionBlueprintTypes.h"
#include "ThreadAsyncExecBase.h"
#include "Engine/EngineBaseTypes.h"
#include "ThreadAsyncExecTick.generated.h"

//Tick thread execution behavior
USTRUCT(BlueprintType)
struct FThreadTickExecBehavior
{
	GENERATED_BODY()
	/**
	 * If enabled, it will be executed multiple times in each tick.
	 * If a total time within a tick exceeds DeltaSeconds when executed a counted number of times,
	 * the tick will be terminated and wait for the next tick to start.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Thread|Tick")
	bool bDynamicExecPerTick = false;

	/**
	 * Valid only if bDynamicExecPerTick is false.
	 * This value will cause the thread to execute a fixed number of times per tick
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=1), Category="Thread|Tick")
	int32 StaticExecTimes = 1;

	/**
	 * Valid only if bDynamicExecPerTick is true.
	 * If enabled, the corresponding elapsed time is logged for each execution.
	 * Accordingly, it will determine whether the next execution will delay the total tick time.
	 * If it does, it stops the execution and waits for the next tick.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Thread|Tick")
	bool bPredictNextUsingPreExec = false;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Thread|Tick")
	float PredictedTimeCostFactor = 1.0f;
};


/**
 * 
 */
UCLASS(Abstract, BlueprintType)
class THREADEXECUTIONBLUEPRINTNODE_API UThreadAsyncExecTickBase : public UThreadAsyncExecBase
{
	GENERATED_BODY()

public:
	UThreadAsyncExecTickBase()
		: bIsTickEnabled(true),
		  bIsTickableWhenPaused(false),
		  bIsTicking(false),
		  bSyncTicking(false)
	{
	}

public:
	virtual void Activate() override;

	virtual void Execution() override;
	virtual void BeginDestroy() override;

protected:
	virtual void BeginDestroyButFutureNotReady() override;
	// virtual void CompletedExecInGameThread() override;

	// void OnWorldPreActorTick(UWorld* World, ELevelTick LevelTick, float DeltaTime);
	// void OnWorldPostActorTick(UWorld* World, ELevelTick LevelTick, float DeltaTime);

	virtual void DoExecutionDelegate()PURE_VIRTUAL(DoExecutionDelegate);
	virtual void DoTickEndDelegate()PURE_VIRTUAL(DoTickEndDelegate);

	FThreadTickExecBehavior Behavior;
	FThreadExecTimingPair TimingPair;

	uint8 bIsTickEnabled : 1;
	uint8 bIsTickableWhenPaused : 1;

	FEvent* TickEvent = nullptr;
	FEvent* SyncTickEvent = nullptr;
	float DeltaSeconds = 0.0f;

	uint8 bIsTicking : 1;
	
	uint8 bSyncTicking : 1;

public:
	UFUNCTION(BlueprintPure, Category="Thread|Tick")
	FThreadTickExecBehavior GetBehavior() const { return Behavior; }

	UFUNCTION(BlueprintPure, Category="Thread|Tick")
	FThreadExecTimingPair GetTimingPair() const { return TimingPair; }

	UFUNCTION(BlueprintPure, Category="Thread|Tick")
	bool IsTicking() const { return bIsTicking; }


	void BeginExec(float DeltaTime, ELevelTick TickType, EThreadTickTiming CurrentTickTiming);
	void EndExec(float DeltaTime, ELevelTick TickType, EThreadTickTiming CurrentTickTiming);

public:
	// FTickableGameObject implementation Begin

	// virtual TStatId GetStatId() const override { return TStatId(); } // Need to custom

	UFUNCTION(BlueprintPure, Category="Thread|Tick")
	virtual bool IsTickEnabled() const
	{
		return !IsTemplate() && bool(ExecMask & EThreadAsyncExecTag::DoExecution) && bIsTickEnabled;
	}

	UFUNCTION(BlueprintPure, Category="Thread|Tick")
	virtual bool IsTickableWhenPaused() const
	{
		return !IsTemplate() && bool(ExecMask & EThreadAsyncExecTag::DoExecution) && bIsTickableWhenPaused;
	}

	UFUNCTION(BlueprintCallable, Category="Thread|Tick")
	virtual void SetTickEnabled(bool NewValue)
	{
		bIsTickEnabled = NewValue;
	}

	UFUNCTION(BlueprintCallable, Category="Thread|Tick")
	virtual void SetTickableWhenPaused(bool NewValue)
	{
		bIsTickableWhenPaused = NewValue;
	}

	// virtual void Tick(float DeltaTime) override;

	// virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); } // Need to custom

	// virtual ETickableTickType GetTickableTickType() const override
	// {
	// 	return IsTemplate() ? ETickableTickType::Never : FTickableGameObject::GetTickableTickType();
	// }

	// FTickableGameObject implementation End

public:
	// Blueprint Functions

	/**
	 * Interrupts the next Tick.
	 * if a Tick is currently being executed, it will pop up before the next Tick.
	 */
	UFUNCTION(BlueprintCallable, Category="Thread|Tick", meta=(CompactNodeTitle="BREAK TICK"))
	void BreakNextTick() { ClearExecMask(EThreadAsyncExecTag::DoExecution); }
};

UCLASS()
class UThreadAsyncExecTick : public UThreadAsyncExecTickBase
{
	GENERATED_BODY()

protected:
	virtual void DoExecutionDelegate() override;
	virtual void CompletedExecInGameThread() override;
	virtual void DoTickEndDelegate() override;

public:
	/**
	 * @Param DeltaSeconds - Tick interval.
	 * @Param TickHandle - A reference to the object of the Tick thread. You can operate on the Tick through this reference.
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTick, float, DeltaSeconds, UThreadAsyncExecTick*, TickHandle);

	/**
	 * Per Tick will execute one to many execution streams.
	 * The exact number of executions is customized by the Behavior structure parameter.
	 * Execution flows in other thread.
	 */
	UPROPERTY(BlueprintAssignable, DisplayName="Execution")
	FOnTick OnExecution;

	/**
	 * Execution streams executed at the end of per tick and at the end of all execution streams executed within that tick
	 * Execution flows in other thread.
	 */
	UPROPERTY(BlueprintAssignable, DisplayName="TickEnd")
	FSimpleDynamicMuticastDelegate OnTickEnd;

	/**
	 * Completed event stream executed at the end of Tick execution.
	 * Execution flows in game thread.
	 */
	UPROPERTY(BlueprintAssignable, DisplayName="Completed")
	FSimpleDynamicMuticastDelegate OnCompleted;

	/**
	 * Create threads and execute them at per tick.
	 * Repeated execution this node will create multiple threaded tasks instead of waiting for the previous threaded task to finish executing.
	 * @param bLongTask - If true, a separate thread is created with continuously executing or long execution time tasks.
	 * If false, create tasks in the task graph, with short tasks.
	 * The performance consumption of creating short tasks is the least
	 * @param ThreadName - The name of the created thread. If the default value is None, the created thread is a non-None default.
	 * If it is not the default value None, the name of the created thread will be that value.
	 */
	UFUNCTION
	(
		BlueprintCallable, Category="Thread|Tick",
		meta=
		(
			BlueprintInternalUseOnly=true,
			AdvancedDisplay="bTickEnabled,bTickWhenPaused,bLongTask,ThreadName,TimingPair,Behavior"
		)
	)
	static UThreadAsyncExecTick* CreateThreadExecTick
	(
		bool bTickEnabled = true,
		bool bTickWhenPaused = false,
		bool bLongTask = false,
		FName ThreadName = NAME_None,
		FThreadExecTimingPair TimingPair = FThreadExecTimingPair(),
		FThreadTickExecBehavior Behavior = FThreadTickExecBehavior()
	);
};

UCLASS()
class UThreadAsyncExecTickForLoop : public UThreadAsyncExecTickBase
{
	GENERATED_BODY()

protected:
	virtual void DoExecutionDelegate() override;
	virtual void CompletedExecInGameThread() override;
	virtual void DoTickEndDelegate() override;
	int32 Index;

	int32 FirstIndex;
	int32 LastIndex;

public:
	UFUNCTION(BlueprintPure, Category="Thread|Loop")
	int32 GetCurrentIndex() const { return Index; }

	UFUNCTION(BlueprintPure, Category="Thread|Loop")
	int32 GetFirstIndex() const { return FirstIndex; }

	UFUNCTION(BlueprintPure, Category="Thread|Loop")
	int32 GetLastIndex() const { return LastIndex; }

	/*@Param TickHandle - A reference to the object of the Tick thread. You can operate on the Tick through this reference*/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTickForLoop, int32, Index, float, DeltaSeconds,
	                                               UThreadAsyncExecTickForLoop*, TickHandle);

	/**
	 * Per Tick will execute one to many execution streams.
	 * The exact number of executions is customized by the Behavior structure parameter.
	 * Execution flows in other thread.
	 */
	UPROPERTY(BlueprintAssignable, DisplayName="LoopBody")
	FOnTickForLoop OnExecution;

	/**
	 * Execution streams executed at the end of per tick and at the end of all execution streams executed within that tick
	 * Execution flows in other thread.
	 */
	UPROPERTY(BlueprintAssignable, DisplayName="TickEnd")
	FSimpleDynamicMuticastDelegate OnTickEnd;

	/**
	 * Completed event stream executed at the end of Tick execution.
	 * Execution flows in game thread.
	 */
	UPROPERTY(BlueprintAssignable, DisplayName="Completed")
	FSimpleDynamicMuticastDelegate OnCompleted;

	/**
	 * Create threads and execute them at each tick
	 * Repeated execution will create multiple threaded tasks instead of waiting for the previous threaded task to finish executing
	 * @param bLongTask - If true, a separate thread is created with continuously executing or long execution time tasks.
	 * If false, create tasks in the task graph, with short tasks.
	 * The performance consumption of creating short tasks is the least
	 * @param ThreadName - The name of the created thread. If the default value is None, the created thread is a non-None default.
	 * If it is not the default value None, the name of the created thread will be that value.
	 */
	UFUNCTION
	(
		BlueprintCallable, Category="Thread|Loop",
		meta=
		(
			BlueprintInternalUseOnly=true,
			AdvancedDisplay="bTickEnabled,bTickWhenPaused,bLongTask,ThreadName,TimingPair,Behavior"
		)
	)
	static UThreadAsyncExecTickForLoop* CreateThreadExecTickForLoop
	(
		int32 FirstIndex = 0,
		int32 LastIndex = 0,
		bool bTickEnabled = true,
		bool bTickWhenPaused = false,
		bool bLongTask = false,
		FName ThreadName = NAME_None,
		FThreadExecTimingPair TimingPair = FThreadExecTimingPair(),
		FThreadTickExecBehavior Behavior = FThreadTickExecBehavior()
	);
};
