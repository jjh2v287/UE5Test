// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.


#include "Async_CreateCoroutine.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"


//
UAsync_CreateCoroutine::UAsync_CreateCoroutine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


UAsync_CreateCoroutine* UAsync_CreateCoroutine::CreateCoroutine(const UObject* WorldContextObject, UObject* Object, const float Delay/*=.0f*/, const float Duration/*=1.0f*/)
{
	// Create this UAsync_CreateCoroutine Instance Node
	UAsync_CreateCoroutine* AsyncNode = NewObject<UAsync_CreateCoroutine>();
	AsyncNode->Delay = Delay;
	AsyncNode->Duration = Duration;
	AsyncNode->ReferObject = Object;

	//  Call to globally register this object with a game instance, it will not be destroyed until SetReadyToDestroy is called
	if (WorldContextObject)
	{
		AsyncNode->RegisterWithGameInstance((UObject*)WorldContextObject);
	}
	else
	{
		UWorld* World = AsyncNode->GetGWorld();
		AsyncNode->RegisterWithGameInstance(World);
	}

	// return this node
	return AsyncNode;
}

void UAsync_CreateCoroutine::Activate()
{
	if (Delay < 0.0f)
	{
		FFrame::KismetExecutionMessage(TEXT("Create Coroutine(Latent)'s DelayTime cannot be less to 0."), ELogVerbosity::Warning);
		return;
	}
	if (Duration < 0.0f)
	{
		FFrame::KismetExecutionMessage(TEXT("Create Coroutine(Latent)'s DurationTime cannot be less to 0."), ELogVerbosity::Warning);
		return;
	}

	bTickable = true;
}


// FTickableGameObject Interface
void UAsync_CreateCoroutine::Tick(float DeltaTime)
{
	// Calculate elapsed time of tick
	ElapsedTime += DeltaTime;

	if (Delay + Duration <= ElapsedTime)
	{
		//Call event connected to End Exce pin
		End.Broadcast(ReferObject, 1.0);
		bTickable = false;
		SetReadyToDestroy();
	}
	else if (Delay <= ElapsedTime)
	{
		if (!bCallStartEvent)
		{
			Start.Broadcast(ReferObject, 0.0f);
			bCallStartEvent = true;
		}
		// Call event connected to Update Exce pin
		float Progress = FMath::Clamp((ElapsedTime - Delay) / Duration, 0.0f, 1.0f);
		Update.Broadcast(ReferObject, Progress);
	}
}

// FTickableGameObject Interface,whether open tick
bool UAsync_CreateCoroutine::IsTickable() const
{
	return bTickable;
}

// FTickableGameObject Interface
bool UAsync_CreateCoroutine::IsTickableInEditor() const
{
	return false;
}
// FTickableGameObject Interface
TStatId UAsync_CreateCoroutine::GetStatId() const
{
	return Super::GetStatID();
}


UWorld* UAsync_CreateCoroutine::GetGWorld()
{
	UWorld* World =
#if WITH_EDITOR
		GIsEditor ? GWorld :
#endif // WITH_EDITOR
		(GEngine->GetWorldContexts().Num() > 0 ? GEngine->GetWorldContexts()[0].World() : nullptr);

	return World;
}