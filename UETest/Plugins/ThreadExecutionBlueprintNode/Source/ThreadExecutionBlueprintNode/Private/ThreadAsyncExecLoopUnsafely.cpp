// Copyright YTSS 2022. All Rights Reserved.

#include "ThreadAsyncExecLoopUnsafely.h"

#include "Async/TaskGraphInterfaces.h"

void UThreadAsyncExecLoopUnsafely::Execution()
{
	while (HasExecMask(EThreadAsyncExecTag::DoExecution))
	{
		{
			if (bBlockGC)
			{
				FGCScopeGuard GCScopeGuard;
				if (OnLoopBody.IsBound())
					OnLoopBody.Broadcast(this);
			}
			else
			{
				if (OnLoopBody.IsBound())
					OnLoopBody.Broadcast(this);
			}
		}
		FPlatformProcess::Sleep(Interval);
	}
}

void UThreadAsyncExecLoopUnsafely::CompletedExecInGameThread()
{
	if (OnCompleted.IsBound()) OnCompleted.Broadcast();
}

UThreadAsyncExecLoopUnsafely* UThreadAsyncExecLoopUnsafely::CreateThreadExecLoopUnsafely
(
	float Interval,
	bool bLongTask,
	FName ThreadName,
	bool bBlockGC
)
{
	auto ExecObject = NewObject<UThreadAsyncExecLoopUnsafely>();

	ExecObject->Interval = Interval;
	ExecObject->AsyncExecution = bLongTask ? EAsyncExecution::Thread : EAsyncExecution::TaskGraph;
	ExecObject->ThreadName = ThreadName == NAME_None ? "AsyncExecUnsafeLoopThread" : ThreadName;
	ExecObject->bBlockGC = bBlockGC;

	return ExecObject;
}
