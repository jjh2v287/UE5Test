// Copyright YTSS 2022. All Rights Reserved.

#include "ThreadAsyncExecLibrary.h"

#include "ThreadAsyncExecLoopUnsafely.h"
#include "ThreadNodeSubsystem.h"
#include "HAL/ThreadManager.h"

int32 UThreadAsyncExecLibrary::GetCurrentThreadID()
{
	return FPlatformTLS::GetCurrentThreadId();
}

FName UThreadAsyncExecLibrary::GetThreadName()
{
	return FName(FThreadManager::Get().GetThreadName(GetCurrentThreadID()));
}

void UThreadAsyncExecLibrary::GameThreadExecOnce(const FSimpleDynamicDelegate& GameThreadExec)
{
	AsyncTask(ENamedThreads::GameThread, [GameThreadExec]() { GameThreadExec.ExecuteIfBound(); });
}

bool UThreadAsyncExecLibrary::SetThreadName(FName ThreadName)
{
	if (IsGameThread())
	{
		return false;
	}
	FPlatformProcess::SetThreadName(GetData(ThreadName.ToString()));
	return true;
}

bool UThreadAsyncExecLibrary::IsGameThread()
{
	return IsInGameThread();
}

void UThreadAsyncExecLibrary::ExecIsGameThread(bool& bIsInGameThread)
{
	bIsInGameThread = IsInGameThread();
}

void UThreadAsyncExecLibrary::ThreadWait(float Seconds)
{
	if (IsInGameThread())
		return;
	FPlatformProcess::Sleep(Seconds);
}
