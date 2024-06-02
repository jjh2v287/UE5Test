// Copyright YTSS 2022. All Rights Reserved.

#include "ThreadAsyncExecTick.h"

#include "Async/TaskGraphInterfaces.h"

void UThreadAsyncExecTickBase::Activate()
{
	// FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &UThreadAsyncExecTick::OnWorldPreActorTick);
	// FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UThreadAsyncExecTick::OnWorldPostActorTick);
	Super::Activate();
}

void UThreadAsyncExecTickBase::Execution()
{
	// Go into tick loop
	TickEvent->Wait();
	while (HasExecMask(EThreadAsyncExecTag::DoExecution))
	{
		SyncTickEvent->Reset();
		bIsTicking = true;

		// UE_LOG(LogTemp,Warning,TEXT("Async Tick GCScopeGuard"));

		if (Behavior.bDynamicExecPerTick)
		{
			double Duration = 0;
			do
			{
				double Start = FPlatformTime::Seconds();
				DoExecutionDelegate();
				double End = FPlatformTime::Seconds();

				if (Behavior.bPredictNextUsingPreExec)
				{
					if ((End - Start) * (Behavior.PredictedTimeCostFactor + 1) + Duration > DeltaSeconds)
					{
						break;
					}
				}

				Duration += End - Start;
			}
			while (Duration < DeltaSeconds && bSyncTicking);
		}
		else
		{
			for (int i = 1; i <= Behavior.StaticExecTimes; i++)
			{
				DoExecutionDelegate();
			}
		}
		bIsTicking = false;
		DoTickEndDelegate();

		// UE_LOG(LogTemp,Warning,TEXT("Async Tick TickEvent->Wait()"));
		SyncTickEvent->Trigger();
		TickEvent->Wait();
	}
}

void UThreadAsyncExecTickBase::BeginDestroy()
{
	Super::BeginDestroy();
	// Release FEvent resource
	if (TickEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(TickEvent);
		TickEvent = nullptr;
	}
	if (SyncTickEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(SyncTickEvent);
		SyncTickEvent = nullptr;
	}
}

void UThreadAsyncExecTickBase::BeginDestroyButFutureNotReady()
{
	if (TickEvent)
	{
		TickEvent->Trigger();
	}
	if (SyncTickEvent)
	{
		SyncTickEvent->Trigger();
	}
}

// void UThreadAsyncExecTickBase::CompletedExecInGameThread()
// {
// 	if (OnCompleted.IsBound()) OnCompleted.Broadcast();
// }

// void UThreadAsyncExecTick::OnWorldPreActorTick(UWorld* World, ELevelTick LevelTick, float DeltaTime)
// {
// 	if (IsValid(this) && TickEvent)
// 	{
// 		DeltaSeconds = DeltaTime;
// 		TickEvent->Trigger();
// 	}
// }
//
// void UThreadAsyncExecTick::OnWorldPostActorTick(UWorld* World, ELevelTick LevelTick, float DeltaTime)
// {
// 	SyncTickEvent->Wait();
// }

void UThreadAsyncExecTickBase::BeginExec(float DeltaTime, ELevelTick TickType, EThreadTickTiming CurrentTickTiming)
{
	if (IsValid(this) && TickEvent)
	{
		DeltaSeconds = DeltaTime;
		bSyncTicking = true;
		TickEvent->Trigger();
	}
}

void UThreadAsyncExecTickBase::EndExec(float DeltaTime, ELevelTick TickType, EThreadTickTiming CurrentTickTiming)
{
	bSyncTicking = false;
	SyncTickEvent->Wait();
}

void UThreadAsyncExecTick::DoExecutionDelegate()
{
	if (OnExecution.IsBound())
		OnExecution.Broadcast(DeltaSeconds, this);
}

void UThreadAsyncExecTick::CompletedExecInGameThread()
{
	if (OnCompleted.IsBound()) OnCompleted.Broadcast();
}

void UThreadAsyncExecTick::DoTickEndDelegate()
{
	if (OnTickEnd.IsBound())
	{
		OnTickEnd.Broadcast();
	}
}

UThreadAsyncExecTick* UThreadAsyncExecTick::CreateThreadExecTick
(
	bool bTickEnabled, bool bTickWhenPaused,
	bool bLongTask, FName ThreadName,
	FThreadExecTimingPair TimingPair,
	FThreadTickExecBehavior Behavior
)
{
	auto NewTick = NewObject<UThreadAsyncExecTick>();

	NewTick->bIsTickEnabled = bTickEnabled;
	NewTick->bIsTickableWhenPaused = bTickWhenPaused;

	NewTick->AsyncExecution = bLongTask ? EAsyncExecution::Thread : EAsyncExecution::TaskGraph;
	NewTick->ThreadName = ThreadName == NAME_None ? "AsyncExecTickThread" : ThreadName;
	NewTick->TimingPair = TimingPair;

	NewTick->Behavior = Behavior;

	NewTick->TickEvent = FPlatformProcess::GetSynchEventFromPool(false);
	NewTick->SyncTickEvent = FPlatformProcess::GetSynchEventFromPool(true);
	NewTick->SyncTickEvent->Trigger();

	return NewTick;
}

void UThreadAsyncExecTickForLoop::DoExecutionDelegate()
{
	if (Index <= LastIndex)
	{
		if (OnExecution.IsBound())
			OnExecution.Broadcast(Index, DeltaSeconds, this);
		Index++;
	}
	else
	{
		BreakNextTick();
	}
}

void UThreadAsyncExecTickForLoop::CompletedExecInGameThread()
{
	if (OnCompleted.IsBound()) OnCompleted.Broadcast();
}

void UThreadAsyncExecTickForLoop::DoTickEndDelegate()
{
	if (OnTickEnd.IsBound())
	{
		OnTickEnd.Broadcast();
	}
}

UThreadAsyncExecTickForLoop* UThreadAsyncExecTickForLoop::CreateThreadExecTickForLoop
(
	int32 FirstIndex, int32 LastIndex,
	bool bTickEnabled, bool bTickWhenPaused,
	bool bLongTask, FName ThreadName,
	FThreadExecTimingPair TimingPair,
	FThreadTickExecBehavior Behavior
)
{
	auto NewTick = NewObject<UThreadAsyncExecTickForLoop>();

	NewTick->FirstIndex = FirstIndex;
	NewTick->LastIndex = LastIndex;
	NewTick->Index = FirstIndex;

	NewTick->bIsTickEnabled = bTickEnabled;
	NewTick->bIsTickableWhenPaused = bTickWhenPaused;

	NewTick->AsyncExecution = bLongTask ? EAsyncExecution::Thread : EAsyncExecution::TaskGraph;
	NewTick->ThreadName = ThreadName == NAME_None ? "AsyncExecTickForLoopThread" : ThreadName;
	NewTick->TimingPair = TimingPair;

	NewTick->Behavior = Behavior;

	NewTick->TickEvent = FPlatformProcess::GetSynchEventFromPool(false);
	NewTick->SyncTickEvent = FPlatformProcess::GetSynchEventFromPool(true);
	NewTick->SyncTickEvent->Trigger();

	return NewTick;
}

// void UThreadAsyncExecTick::Tick(float DeltaTime)
// {
// 	// DeltaSeconds = DeltaTime;
// 	// TickEvent->Trigger();
// 	// SyncTickEvent->Wait();
// }
