// Copyright YTSS 2022. All Rights Reserved.

#include "ThreadAsyncExecBase.h"

#include "ThreadExecDeveloperSettings.h"
#include "ThreadNodeSubsystem.h"
#include "Engine/Engine.h"

void UThreadAsyncExecBase::Activate()
{
	if (auto ThreadNodeSubsystem = GEngine->GetEngineSubsystem<UThreadNodeSubsystem>())
	{
		ThreadNodeSubsystem->AddThreadObject(this);
	}
	Future = Async
	(
		AsyncExecution,
		[&]
		{
			FPlatformProcess::SetThreadName(GetThreadName().ToString().GetCharArray().GetData());
			Execution();
		},
		[&]
		{
			Completed();
		}
	);
}

void UThreadAsyncExecBase::BeginDestroy()
{
	// UE_LOG(LogTemp, Warning, TEXT("%s Node BeginDestroy"), *GetName())
	Super::BeginDestroy();

	if (!Future.IsReady())
	{
		ClearExecMask(EThreadAsyncExecTag::DoExecution);
		SetExecMask(EThreadAsyncExecTag::WithoutCompletion);

		BeginDestroyButFutureNotReady();

		// if (auto Settings = GetDefault<UThreadExecDeveloperSettings>())
		// {
		// 	if(Settings->PendingThreadTaskDoneWhenEnding>0)
		// 	{
		// 		Future.WaitFor(FTimespan::FromMilliseconds(Settings->PendingThreadTaskDoneWhenEnding));
		// 	}
		// 	else
		// 	{
		// 		Future.Wait();
		// 	}
		// }
		// else
		// {
		// 	Future.WaitFor(FTimespan::FromSeconds(1));
		// }
	}
	if (auto ThreadNodeSubsystem = GEngine->GetEngineSubsystem<UThreadNodeSubsystem>())
	{
		ThreadNodeSubsystem->RemoveThreadObject(this);
	}
}

void UThreadAsyncExecBase::Completed()
{
	if (HasExecMask(EThreadAsyncExecTag::WithoutCompletion)) return;
	AsyncTask(ENamedThreads::GameThread, [&]
	{
		if (HasExecMask(EThreadAsyncExecTag::WithoutCompletion)) return;

		CompletedExecInGameThread();

		SetReadyToDestroy();
		MarkAsGarbage();
	});
}

void UThreadAsyncExecBase::BeginDestroyButFutureNotReady()
{
}

void UThreadAsyncExecBase::CompletedExecInGameThread()
{
}
