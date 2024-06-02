// Copyright 2023, Cubeap Electronics, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "Async/TaskGraphInterfaces.h"
#include "Engine/Engine.h"
#include "Engine/CancellableAsyncAction.h"
#include "BackgroundTaskBP.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBackgroundLoopPin, int32, Index);

UCLASS()
class BASICASYNC_API UBackgroundTaskBP : public UCancellableAsyncAction
{
	friend class AsyncBackgroundTaskLoop;
	
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FBackgroundLoopPin Body ;
	UPROPERTY(BlueprintAssignable)
	FBackgroundLoopPin Completed;
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UBackgroundTaskBP* BackgroundTask(const UObject* WorldContextObject, bool bAsyncComplete);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", ClampMin = "0", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UBackgroundTaskBP* BackgroundTaskLoop(const UObject* WorldContextObject, bool bAsyncComplete, int32 Count = 0);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UBackgroundTaskBP* BackgroundTaskWhile(const UObject* WorldContextObject, bool bAsyncComplete);
	
	UFUNCTION(BlueprintCallable, Category="AsyncAction")
	void FullCancel();

	UFUNCTION(BlueprintCallable, Category="AsyncAction")
	void Wait();

	UFUNCTION(BlueprintCallable, Category="AsyncAction")
	static void StopAndWait(TArray<UBackgroundTaskBP*> Tasks);

protected:
	virtual void Activate() override;
	virtual void Cancel() override;

	virtual UWorld* GetWorld() const override
	{
		return WorldContextObject.IsValid() ? WorldContextObject.Get() : nullptr;
	}
	
private:
	
	TWeakObjectPtr<UWorld> WorldContextObject = nullptr;
	
	ENamedThreads::Type Thread = ENamedThreads::GameThread;
	
	void Restart();
	void GameThread();
	void Stop();
	void RunTask();
	
	void (UBackgroundTaskBP::*Finalize) () {&UBackgroundTaskBP::GameThread};
	int32 Count = 0;
	
	volatile bool bStop = false;
};

///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///

class AsyncBackgroundTaskLoop : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<AsyncBackgroundTaskLoop>;

public:
	
	AsyncBackgroundTaskLoop(TWeakObjectPtr<UBackgroundTaskBP> Task);

	~AsyncBackgroundTaskLoop();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(AsyncBackgroundTaskLoop, STATGROUP_ThreadPoolAsyncTasks);
	}
	
	void DoWork();

private:
	
	bool CheckValid() const;
	
	TWeakObjectPtr<UBackgroundTaskBP> NodeObject = nullptr;
};



