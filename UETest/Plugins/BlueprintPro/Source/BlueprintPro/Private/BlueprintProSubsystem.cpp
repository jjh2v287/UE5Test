// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.


#include "BlueprintProSubsystem.h"
#include "CoroutineHandle.h"


UCoroutineHandle* UBlueprintProSubsystem::RequestCoroutine()
{
	if (CachedCoroutines.Num() > 0)
	{
		return CachedCoroutines.Pop(false);
	}
	UCoroutineHandle* NewCoroutine = NewObject<UCoroutineHandle>();
	AllCreatedCoroutines.Add(NewCoroutine);
	return NewCoroutine;
}

void UBlueprintProSubsystem::ReturnCoroutine(UCoroutineHandle* Coroutine)
{
	if (Coroutine && AllCreatedCoroutines.Contains(Coroutine))
	{
		Coroutine->Reset();
		if (CachedCoroutines.Contains(Coroutine) == false)
		{
			CachedCoroutines.Add(Coroutine);
		}
	}
}

void UBlueprintProSubsystem::ReturnAllCoroutines()
{
	CachedCoroutines = AllCreatedCoroutines;
	for (UCoroutineHandle* Coroutine : CachedCoroutines)
	{
		if (Coroutine)
		{
			Coroutine->Reset();
		}
	}

	int32 Removed = CachedCoroutines.RemoveAll([](UCoroutineHandle* Coroutine) { return Coroutine == nullptr; });
}

void UBlueprintProSubsystem::FreeAllCoroutines()
{
	CachedCoroutines.Reset();
	AllCreatedCoroutines.Reset();
}

void UBlueprintProSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
}

void UBlueprintProSubsystem::Deinitialize()
{
}
