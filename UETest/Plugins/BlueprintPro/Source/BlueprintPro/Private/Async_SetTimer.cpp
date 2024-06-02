// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.


#include "Async_SetTimer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "UObject/Stack.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"
#include "Engine/World.h"



//Declare General Log Category, source file .cpp
DEFINE_LOG_CATEGORY(LogAsyncSetTimer);


UAsync_SetTimer::UAsync_SetTimer()
{
	WorldContextObject = nullptr;

}


UAsync_SetTimer::~UAsync_SetTimer()
{

}


UAsync_SetTimer* UAsync_SetTimer::SetTimer(const UObject* WorldContextObject, float Time, bool bLooping, float InitialStartDelay /*= 0.f*/, float InitialStartDelayVariance /*= 0.f*/)
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

	UAsync_SetTimer* AsyncNode = NewObject<UAsync_SetTimer>();
	AsyncNode->WorldContextObject = WorldContextObject;

	FTimerDynamicDelegate Delegate;
	Delegate.BindUFunction(AsyncNode, FName("CompletedEvent"));
	AsyncNode->TimerHandle = AsyncNode->SetTimerDelegate(Delegate, Time, bLooping, InitialStartDelay, InitialStartDelayVariance);

	//  Call to globally register this object with a game instance, it will not be destroyed until SetReadyToDestroy is called
	AsyncNode->RegisterWithGameInstance((UObject*)WorldContextObject);
	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddUObject(AsyncNode, &UAsync_SetTimer::PreGarbageCollect);

	return AsyncNode;
}


/** Based on UKismetSystemLibrary::K2_SetTimerDelegate() */
FTimerHandle UAsync_SetTimer::SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay /*= 0.f*/, float InitialStartDelayVariance /*= 0.f*/)
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


void UAsync_SetTimer::PreGarbageCollect()
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

void UAsync_SetTimer::Activate()
{
	if (!WorldContextObject)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject. Cannot execute Set Timer."), ELogVerbosity::Error);
		return;
	}

	// call Then delegate binding event
	Then.Broadcast(TimerHandle);
}


void UAsync_SetTimer::CompletedEvent()
{
	OnEvent.Broadcast(TimerHandle);
}
