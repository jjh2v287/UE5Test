// Copyright YTSS 2022. All Rights Reserved.

#include "SyncExecOnce.h"

void UDEPRECATED_USyncExecOnce::Activate()
{
	AsyncTask(ENamedThreads::GameThread, [&] { Execution(); });
}

UDEPRECATED_USyncExecOnce* UDEPRECATED_USyncExecOnce::CreateSyncExecOnce()
{
	return NewObject<UDEPRECATED_USyncExecOnce>();
}

void UDEPRECATED_USyncExecOnce::Execution()
{
	{
		FGCScopeGuard GCScopeGuard;
		if(OnExecution.IsBound())
			OnExecution.Broadcast();
	}
	SetReadyToDestroy();
}
