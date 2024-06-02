// Copyright 2023, Cubeap Electronics, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "Async/Async.h"
#include "TimerManager.h"
#include "TickLoopTask.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoopPin, int32, Index);

UCLASS()
class BASICASYNC_API UTickLoopTask : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable)
	FLoopPin Body;
	UPROPERTY(BlueprintAssignable)
	FLoopPin Completed;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category="AsyncNodes")
	static UTickLoopTask* DelayLoop(const UObject* WorldContextObject, const float Delay = 0.0f, const int32 StartIndex = 0, const int32 EndIndex = 0, const int32 InOneTick = 1);

	UFUNCTION(BlueprintCallable, Category="AsyncAction")
	void ChangeDelay(const float NewDelay);

	UFUNCTION(BlueprintCallable, Category="AsyncAction")
	void ChangeOneTick(const int32 NewTickCount);
	
protected:
	virtual void Activate() override;
	virtual void Cancel() override;

	virtual UWorld* GetWorld() const override
	{
		return WorldContextObject.IsValid() ? WorldContextObject.Get() : nullptr;
	}

private:
	FTimerDelegate TD();
	void (UTickLoopTask::*TimerSet)() {&UTickLoopTask::TimerDelay};
	void TimerTick();
	void TimerDelay();
	TWeakObjectPtr<UWorld> WorldContextObject = nullptr;
	volatile int32 Current;
	int32 End;
	int32 OneTick;
	float Delay;
};
