// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.


#include "Async_SetTimerForNextTick.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "UObject/Stack.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"
#include "Engine/World.h"



UAsync_SetTimerForNextTick::UAsync_SetTimerForNextTick()
{
}


UAsync_SetTimerForNextTick::~UAsync_SetTimerForNextTick()
{
}


UAsync_SetTimerForNextTick* UAsync_SetTimerForNextTick::SetTimerForNextTick(const UObject* WorldContextObject, UObject* Object)
{
	if (!WorldContextObject)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject. Cannot execute Async SetTimer."), ELogVerbosity::Error);
		return nullptr;
	}
	UAsync_SetTimerForNextTick* AsyncNode = NewObject<UAsync_SetTimerForNextTick>();
	AsyncNode->WorldContextObject = WorldContextObject;
	AsyncNode->ReferObject = Object;


	FTimerDynamicDelegate Delegate;
	Delegate.BindUFunction(AsyncNode, FName("CompletedEvent"));

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World)
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		FTimerHandle Handle = TimerManager.K2_FindDynamicTimerHandle(Delegate);
		TimerManager.SetTimerForNextTick(Delegate);
	}

	//  Call to globally register this object with a game instance, it will not be destroyed until SetReadyToDestroy is called
	AsyncNode->RegisterWithGameInstance((UObject*)WorldContextObject);

	return AsyncNode;
}


void UAsync_SetTimerForNextTick::Activate()
{
	if (!WorldContextObject)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject. Cannot execute Set Timer."), ELogVerbosity::Error);
		return;
	}
	Then.Broadcast(ReferObject);
}


void UAsync_SetTimerForNextTick::CompletedEvent()
{
	OnEvent.Broadcast(ReferObject);

	SetReadyToDestroy();
}

