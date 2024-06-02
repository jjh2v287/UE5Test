// Copyright 2023, Cubeap Electronics, All rights reserved.

#include "TickLoopTask.h"

UTickLoopTask* UTickLoopTask::DelayLoop(const UObject* WorldContextObject, const float Delay, const int32 StartIndex, const int32 EndIndex, const int32 InOneTick)
{

	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UTickLoopTask* Node = NewObject<UTickLoopTask>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->Current = StartIndex;
	if(Delay > 0.0f) Node->Delay = Delay;
	else Node->TimerSet = &UTickLoopTask::TimerTick;
	Node->End = EndIndex + 1;
	Node->OneTick = InOneTick < 0 ? 0 : InOneTick;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance()); 
	return Node;
}

void UTickLoopTask::ChangeDelay(const float NewDelay)
{
	Delay = NewDelay;
	if(Delay > 0.0f) TimerSet = &UTickLoopTask::TimerDelay;
	else TimerSet = &UTickLoopTask::TimerTick;
}

void UTickLoopTask::ChangeOneTick(const int32 NewTickCount)
{
	OneTick = NewTickCount;
}

void UTickLoopTask::Activate()
{
	Super::Activate();
	AsyncTask(ENamedThreads::GameThread, [this]() 
	{
		if(GetWorld()) (this->*(TimerSet))();
	});
}

void UTickLoopTask::Cancel()
{
	Super::Cancel();
	End = Current;
	OneTick = 0;
	if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	Completed.Broadcast(Current);
	SetReadyToDestroy();
}

FTimerDelegate UTickLoopTask::TD()
{
	return FTimerDelegate::CreateLambda([WeakThis = TWeakObjectPtr<UTickLoopTask>(this)]()
			{
				if(WeakThis.IsValid() && WeakThis->IsActive())
				{
					if(WeakThis->End - WeakThis->Current < WeakThis->OneTick) WeakThis->OneTick = WeakThis->End - WeakThis->Current;
					const int32 CurrentTick = WeakThis->OneTick;
					for(int32 i = 0; i < CurrentTick; i++) WeakThis->Body.Broadcast(WeakThis->Current++);
					if(WeakThis->Current < WeakThis->End) (WeakThis.Get()->*(WeakThis->TimerSet))();
					else WeakThis->Cancel();
				}
			});
}

void UTickLoopTask::TimerTick()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(TD());
}

void UTickLoopTask::TimerDelay()
{
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TD(),Delay,false,0.0f);
}

