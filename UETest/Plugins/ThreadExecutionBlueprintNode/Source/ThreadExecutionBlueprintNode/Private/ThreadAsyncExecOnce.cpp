// Copyright YTSS 2022. All Rights Reserved.

#include "ThreadAsyncExecOnce.h"

#include "Async/TaskGraphInterfaces.h"

void UThreadAsyncExecOnce::Activate()
{
	Super::Activate();
}

void UThreadAsyncExecOnce::Execution()
{
	SyncExecEvent->Wait();
	if (IsValidChecked(this) && OnExecution.IsBound())
	{
		OnExecution.Broadcast();
	}
	SyncEndEvent->Trigger();
}

void UThreadAsyncExecOnce::BeginDestroy()
{
	Super::BeginDestroy();
	if (SyncExecEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(SyncExecEvent);
		SyncExecEvent = nullptr;
		FPlatformProcess::ReturnSynchEventToPool(SyncEndEvent);
		SyncEndEvent = nullptr;
	}
}

void UThreadAsyncExecOnce::BeginDestroyButFutureNotReady()
{
	if (SyncExecEvent)
	{
		SyncExecEvent->Trigger();
	}
}

void UThreadAsyncExecOnce::CompletedExecInGameThread()
{
	if (OnCompleted.IsBound()) OnCompleted.Broadcast();
}

void UThreadAsyncExecOnce::BeginExec(float DeltaTime, ELevelTick TickType, EThreadTickTiming CurrentTickTiming)
{
	SyncEndEvent->Reset();
	SyncExecEvent->Trigger();
}

void UThreadAsyncExecOnce::EndExec(float DeltaTime, ELevelTick TickType, EThreadTickTiming CurrentTickTiming)
{
	SyncEndEvent->Wait();
}

UThreadAsyncExecOnce* UThreadAsyncExecOnce::CreateThreadExecOnce
(
	bool bExecuteWhenPaused,
	bool bLongTask,
	FName ThreadName,
	FThreadExecTimingPair TimingPair
)
{
	auto NewExec = NewObject<UThreadAsyncExecOnce>();

	NewExec->bIsExecuteWhenPaused = bExecuteWhenPaused;
	NewExec->AsyncExecution = bLongTask ? EAsyncExecution::Thread : EAsyncExecution::TaskGraph;
	NewExec->ThreadName = ThreadName == NAME_None ? "AsyncExecOnceThread" : ThreadName;
	NewExec->TimingPair = TimingPair;

	NewExec->SyncExecEvent = FPlatformProcess::GetSynchEventFromPool();

	NewExec->SyncEndEvent = FPlatformProcess::GetSynchEventFromPool(true);
	NewExec->SyncEndEvent->Trigger();

	return NewExec;
}
