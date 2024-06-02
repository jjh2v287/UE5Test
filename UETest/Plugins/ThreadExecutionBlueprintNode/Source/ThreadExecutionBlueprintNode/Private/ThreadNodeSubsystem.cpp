// Copyright YTSS 2022. All Rights Reserved.

#include "ThreadNodeSubsystem.h"





void UThreadNodeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
}

void UThreadNodeSubsystem::Deinitialize()
{
	// UE_LOG(LogTemp, Warning, TEXT("Thread Node Subsystem Deinitialize"))
}

void UThreadNodeSubsystem::CheckPointers()
{
	TSet<UThreadAsyncExecBase*> Valids;
	for (auto Node : Nodes)
	{
		if (IsValid(Node))
		{
			Valids.Add(Node);
		}
	}
	Nodes = Valids;
}

void UThreadNodeSubsystem::AddThreadObject(UThreadAsyncExecBase* NewNode)
{
	FScopeLock ScopeLock(&NodesMutex);
	Nodes.Add(NewNode);
}

void UThreadNodeSubsystem::RemoveThreadObject(UThreadAsyncExecBase* OldNode)
{
	FScopeLock ScopeLock(&NodesMutex);
	Nodes.Remove(OldNode);
}

TSet<UThreadAsyncExecBase*> UThreadNodeSubsystem::GetAllThreadExecNodes()
{
	FScopeLock ScopeLock(&NodesMutex);
	CheckPointers();
	TSet<UThreadAsyncExecBase*> Returns;
	for (auto Node : Nodes)
	{
		Returns.Add(Node);
	}
	return Returns;
}

TSet<UThreadAsyncExecOnce*> UThreadNodeSubsystem::GetAllThreadExecOnces()
{
	return GetAllThreadExecNodesTemplate<UThreadAsyncExecOnce>();
}

TSet<UThreadAsyncExecLoopUnsafely*> UThreadNodeSubsystem::GetAllThreadExecLoops()
{
	return GetAllThreadExecNodesTemplate<UThreadAsyncExecLoopUnsafely>();
}

TSet<UThreadAsyncExecTickBase*> UThreadNodeSubsystem::GetAllThreadExecTicks()
{
	return GetAllThreadExecNodesTemplate<UThreadAsyncExecTickBase>();
}

UDEPRECATED_Mutex* UThreadNodeSubsystem::CreateNewMutex()
{
	auto const NewMutex = NewObject<UDEPRECATED_Mutex>();
	// if (IsValid(NewMutex))
	// {
	// 	Mutexes.Add(NewMutex);
	// }
	return NewMutex;
}

TSet<UDEPRECATED_Mutex*> UThreadNodeSubsystem::GetAllMutexes()
{
	// return Mutexes;
	return TSet<UDEPRECATED_Mutex*>();
}


void UThreadNodeSubsystem::DestoryMutex(UDEPRECATED_Mutex* Mutex)
{
	// if (!IsValid(Mutex))return;
	// Mutexes.Remove(Mutex);
	// Mutex->MarkAsGarbage();
}
