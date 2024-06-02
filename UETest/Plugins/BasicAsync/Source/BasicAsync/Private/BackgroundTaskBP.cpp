// Copyright 2023, Cubeap Electronics, All rights reserved.

#include "BackgroundTaskBP.h"

UBackgroundTaskBP* UBackgroundTaskBP::BackgroundTaskLoop(const UObject* WorldContextObject, bool bAsyncComplete, int32 Count)
{
	UWorld* WorldContext = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if(!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}
	
	UBackgroundTaskBP* Node = NewObject<UBackgroundTaskBP>();
	Node->WorldContextObject = WorldContextObject->GetWorld();
	Node->Count = Count > 0 ? Count -1 : Count;
	if(bAsyncComplete) Node->Thread = ENamedThreads::AnyThread;
	Node->RegisterWithGameInstance(WorldContext->GetGameInstance());
	return Node;
}

UBackgroundTaskBP* UBackgroundTaskBP::BackgroundTask(const UObject* WorldContextObject, bool bAsyncComplete)
{
	return BackgroundTaskLoop(WorldContextObject, bAsyncComplete, 0);
}

UBackgroundTaskBP* UBackgroundTaskBP::BackgroundTaskWhile(const UObject* WorldContextObject, bool bAsyncComplete)
{
	return BackgroundTaskLoop(WorldContextObject, bAsyncComplete, -1);
}

void UBackgroundTaskBP::Activate()
{
	RunTask();
}

AsyncBackgroundTaskLoop::AsyncBackgroundTaskLoop(TWeakObjectPtr<UBackgroundTaskBP> Task)
{
	NodeObject = Task;
}

void AsyncBackgroundTaskLoop::DoWork()
{
	if(CheckValid()) NodeObject->Body.Broadcast(NodeObject->Count);
}

bool AsyncBackgroundTaskLoop::CheckValid() const
{
	return NodeObject.IsValid() && NodeObject->IsActive();
}

AsyncBackgroundTaskLoop::~AsyncBackgroundTaskLoop()
{
	if(CheckValid()) NodeObject->Restart();
}

void UBackgroundTaskBP::Restart()
{
	if(IsValid(this) && GetWorld())
	{
		if(Count !=0)
		{
			if(Count > 0) Count--;
			RunTask();
		}
		else (this->*(Finalize))();
	}
	else (this->*(Finalize))();
}

void UBackgroundTaskBP::Stop()
{
	bStop = true;
	SetReadyToDestroy();
}

void UBackgroundTaskBP::RunTask()
{
	if(FPlatformProcess::SupportsMultithreading()) (new FAutoDeleteAsyncTask<AsyncBackgroundTaskLoop>(TWeakObjectPtr<UBackgroundTaskBP>(this)))->StartBackgroundTask();
	else (new FAutoDeleteAsyncTask<AsyncBackgroundTaskLoop>(TWeakObjectPtr<UBackgroundTaskBP>(this)))->StartSynchronousTask();
}

void UBackgroundTaskBP::GameThread()
{
	AsyncTask(Thread, [SelfPtr = TWeakObjectPtr<UBackgroundTaskBP>(this)]() 
	{
		if(SelfPtr.IsValid() && SelfPtr->IsActive())
		{
			SelfPtr->Completed.Broadcast(SelfPtr->Count);
			SelfPtr->Stop();
		}
	});
}

void UBackgroundTaskBP::Cancel()
{
	Count = 0;
	Super::Cancel();
}

void UBackgroundTaskBP::FullCancel()
{
	Finalize = &UBackgroundTaskBP::Stop;
	Cancel();
}

void UBackgroundTaskBP::Wait()
{
	while(!bStop) FPlatformProcess::Sleep(0.0f);
}

void UBackgroundTaskBP::StopAndWait(const TArray<UBackgroundTaskBP*> Tasks)
{
	AsyncTask(ENamedThreads::GameThread, [Tasks]() 
	{	
		for(auto& T : Tasks)
		{
			if(T->IsActive())
			{
				if(!T->bStop) T->FullCancel();
				T->Wait();
			}
		}
	});
}


