// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#include "Async_SetTimerByObject.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "UObject/Stack.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"
#include "Engine/World.h"



UAsync_SetTimerByObject::UAsync_SetTimerByObject()
{
	WorldContextObject = nullptr;
}


UAsync_SetTimerByObject::~UAsync_SetTimerByObject()
{
}


UAsync_SetTimerByObject* UAsync_SetTimerByObject::SetTimerByObject(const UObject* WorldContextObject, UObject* Object, float Time, bool bLooping, float InitialStartDelay /*= 0.f*/, float InitialStartDelayVariance /*= 0.f*/)
{
	if (!WorldContextObject)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject. Cannot execute Async SetTimer."), ELogVerbosity::Error);
		return nullptr;
	}
	/** Based on UKismetSystemLibrary::K2_SetTimer() */
	InitialStartDelay += FMath::RandRange(-InitialStartDelayVariance, InitialStartDelayVariance);
	if (Time <= 0.f || ((Time + InitialStartDelay) - InitialStartDelayVariance) < 0.f)
	{
		FFrame::KismetExecutionMessage(TEXT("SetTimer passed a negative or zero time.  The associated timer may fail to fire!  If using InitialStartDelayVariance, be sure it is smaller than (Time + InitialStartDelay)."), ELogVerbosity::Warning);
		return nullptr;
	}

	UAsync_SetTimerByObject* AsyncNode = NewObject<UAsync_SetTimerByObject>();
	AsyncNode->WorldContextObject = WorldContextObject;
	AsyncNode->ReferObject = Object;

	FTimerDynamicDelegate Delegate;
	Delegate.BindUFunction(AsyncNode, FName("OnTimerEvent"));
	AsyncNode->TimerHandle = AsyncNode->SetTimerDelegate(Delegate, Time, bLooping, InitialStartDelay, InitialStartDelayVariance);

	//  Call to globally register this object with a game instance, it will not be destroyed until SetReadyToDestroy is called
	AsyncNode->RegisterWithGameInstance((UObject*)WorldContextObject);
	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddUObject(AsyncNode, &UAsync_SetTimerByObject::PreGarbageCollect);

	return AsyncNode;
}


/** Based on UKismetSystemLibrary::K2_SetTimerDelegate() */
FTimerHandle UAsync_SetTimerByObject::SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay /*= 0.f*/, float InitialStartDelayVariance /*= 0.f*/)
{
	FTimerHandle Handle;
	if (Delegate.IsBound())
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		if (World)
		{
			InitialStartDelay += FMath::RandRange(-InitialStartDelayVariance, InitialStartDelayVariance);
			if (Time <= 0.f || ((Time + InitialStartDelay) - InitialStartDelayVariance) < 0.f)
			{
				FFrame::KismetExecutionMessage(TEXT("SetTimer passed a negative or zero time.  The associated timer may fail to fire!  If using InitialStartDelayVariance, be sure it is smaller than (Time + InitialStartDelay)."), ELogVerbosity::Warning);
			}

			FTimerManager& TimerManager = World->GetTimerManager();
			Handle = TimerManager.K2_FindDynamicTimerHandle(Delegate);
			TimerManager.SetTimer(Handle, Delegate, Time, bLooping, (Time + InitialStartDelay));
		}
	}
	else
	{
		UE_LOG(LogBlueprintUserMessages, Warning, TEXT("SetTimer passed a bad function (%s) or object (%s)"), *Delegate.GetFunctionName().ToString(), *GetNameSafe(Delegate.GetUObject()));
	}

	return Handle;
}


void UAsync_SetTimerByObject::PreGarbageCollect()
{
	if (World)
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (!TimerManager.TimerExists(TimerHandle))
		{
			SetReadyToDestroy();
		}
	}
}

void UAsync_SetTimerByObject::Activate()
{
	if (!WorldContextObject)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject. Cannot execute Set Timer."), ELogVerbosity::Error);
		return;
	}

	// call Then delegate binding event
	Then.Broadcast(ReferObject, TimerHandle);
}


void UAsync_SetTimerByObject::OnTimerEvent()
{
	OnEvent.Broadcast(ReferObject, TimerHandle);
}
