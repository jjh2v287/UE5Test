// Fill out your copyright notice in the Description page of Project Settings.


#include "ThreadAsyncExecOnceUnsafely.h"

void UThreadAsyncExecOnceUnsafely::Execution()
{
	if (bBlockGC)
	{
		FGCScopeGuard GCScopeGuard;
		if (OnExecution.IsBound())
			OnExecution.Broadcast();
	}
	else
	{
		if (OnExecution.IsBound())
			OnExecution.Broadcast();
	}
}

void UThreadAsyncExecOnceUnsafely::CompletedExecInGameThread()
{
	if (OnCompleted.IsBound())
	{
		OnCompleted.Broadcast();
	}
}

UThreadAsyncExecOnceUnsafely* UThreadAsyncExecOnceUnsafely::CreateThreadExecOnceUnsafely(bool bLongTask, FName ThreadName,bool bBlockGC)
{
	auto ExecObject = NewObject<UThreadAsyncExecOnceUnsafely>();

	ExecObject->AsyncExecution = bLongTask ? EAsyncExecution::Thread : EAsyncExecution::TaskGraph;
	ExecObject->ThreadName = ThreadName == NAME_None ? "AsyncExecUnsafeOnceThread" : ThreadName;
	ExecObject->bBlockGC=bBlockGC;
	
	return ExecObject;
}
