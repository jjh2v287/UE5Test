// Fill out your copyright notice in the Description page of Project Settings.


#include "ThreadAsyncExecTickUnsafely.h"

void UThreadAsyncExecTickUnsafely::Execution()
{
	// Go into tick loop
	TickEvent->Wait();
	while (HasExecMask(EThreadAsyncExecTag::DoExecution))
	{
		{
			// UE_LOG(LogTemp,Warning,TEXT("Async Tick GCScopeGuard"));
			if (bBlockGC)
			{
				FGCScopeGuard GCScopeGuard;
				if (OnTick.IsBound())
					OnTick.Broadcast(DeltaSeconds, this);
			}
			else
			{
				if (OnTick.IsBound())
					OnTick.Broadcast(DeltaSeconds, this);
			}
		}
		// UE_LOG(LogTemp,Warning,TEXT("Async Tick TickEvent->Wait()"));
		TickEvent->Wait();
	}
}

void UThreadAsyncExecTickUnsafely::BeginDestroy()
{
	Super::BeginDestroy();
	// Release FEvent resource
	if (TickEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(TickEvent);
	}
	TickEvent = nullptr;
}

void UThreadAsyncExecTickUnsafely::BeginDestroyButFutureNotReady()
{
	if (TickEvent)
	{
		TickEvent->Trigger();
	}
}

void UThreadAsyncExecTickUnsafely::CompletedExecInGameThread()
{
	if (OnCompleted.IsBound()) OnCompleted.Broadcast();
}

UThreadAsyncExecTickUnsafely* UThreadAsyncExecTickUnsafely::CreateThreadExecTickUnsafely
(
	bool TickEnabled,
	bool TickWhenPaused,
	bool bLongTask,
	FName ThreadName,
	bool bBlockGC
)
{
	auto ExecObject = NewObject<UThreadAsyncExecTickUnsafely>();

	ExecObject->bIsTickEnabled = TickEnabled;
	ExecObject->bIsTickableWhenPaused = TickWhenPaused;
	ExecObject->AsyncExecution = bLongTask ? EAsyncExecution::Thread : EAsyncExecution::TaskGraph;
	ExecObject->ThreadName = ThreadName == NAME_None ? "AsyncExecUnsafeTickThread" : ThreadName;
	ExecObject->bBlockGC = bBlockGC;

	ExecObject->TickEvent = FPlatformProcess::GetSynchEventFromPool(false);

	return ExecObject;
}

void UThreadAsyncExecTickUnsafely::Tick(float DeltaTime)
{
	DeltaSeconds = DeltaTime;
	TickEvent->Trigger();
}
