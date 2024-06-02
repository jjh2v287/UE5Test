// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.



#include "Async_CreateCoroutineAdvance.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "BlueprintProSubsystem.h"
#include "CoroutineHandle.h"

/** Declare General Log Category */
DEFINE_LOG_CATEGORY_STATIC(LogAsyncCreateCoroutineAdvance, Log, All);

//
UAsync_CreateCoroutineAdvance::UAsync_CreateCoroutineAdvance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


UAsync_CreateCoroutineAdvance* UAsync_CreateCoroutineAdvance::CreateCoroutineAdvance(const UObject* WorldContextObject, UObject* Object, const float Delay/*=.0f*/, const float Duration/*=1.0f*/)
{
	// Create this UAsync_CreateCoroutineAdvance Instance Node
	UAsync_CreateCoroutineAdvance* AsyncNode = NewObject<UAsync_CreateCoroutineAdvance>();
	AsyncNode->DelayTime = Delay;
	AsyncNode->DurationTime = Duration;
	AsyncNode->ReferObject = Object;

	UWorld* World = AsyncNode->GetGWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UBlueprintProSubsystem* BlueprintProSubsystem = GameInstance ? GameInstance->GetSubsystem<UBlueprintProSubsystem>() : nullptr;
	if (BlueprintProSubsystem)
	{
		UCoroutineHandle* NewCoroutineHandle = BlueprintProSubsystem->RequestCoroutine();
		NewCoroutineHandle->SetReferenceObject(Object, AsyncNode);
		AsyncNode->CoroutineHandle = NewCoroutineHandle;
	}

	//  Call to globally register this object with a game instance, it will not be destroyed until SetReadyToDestroy is called
	if (WorldContextObject)
	{
		AsyncNode->RegisterWithGameInstance((UObject*)WorldContextObject);
	}
	else
	{
		AsyncNode->RegisterWithGameInstance(World);
	}

	// return this node
	return AsyncNode;
}

void UAsync_CreateCoroutineAdvance::Activate()
{
	if (DelayTime < 0.0f)
	{
		FFrame::KismetExecutionMessage(TEXT("Create Coroutine(Latent)'s DelayTime cannot be less to 0."), ELogVerbosity::Warning);
		return;
	}
	if (DurationTime < 0.0f)
	{
		FFrame::KismetExecutionMessage(TEXT("Create Coroutine(Latent)'s DurationTime cannot be less to 0."), ELogVerbosity::Warning);
		return;
	}

	//bTickable = true;
	if (CoroutineHandle)
	{
		CoroutineHandle->EnableTick(true);
	}
	else
	{
		UE_LOG(LogAsyncCreateCoroutineAdvance, Error, TEXT("Function:%s, Invalid Coroutine Handle."), *FString(__FUNCTION__));
	}
}


// FTickableGameObject Interface
void UAsync_CreateCoroutineAdvance::Tick(float DeltaTime)
{
	if (!CoroutineHandle)
	{
		return;
	}
	else if (CoroutineHandle->IsCoroutinePaused())
	{
		return;
	}

	// Calculate elapsed time of tick
	ElapsedTime += DeltaTime;

	if (DelayTime + DurationTime <= ElapsedTime)
	{
		CoroutineHandle->SetProgress(1.0f);
		CoroutineHandle->EnableTick(false);

		//Call event connected to End Exce pin
		End.Broadcast(CoroutineHandle);

		SetReadyToDestroy();

		CoroutineHandle->Invalidate();
	}
	else if (DelayTime <= ElapsedTime)
	{
		if (!bCallStartEvent)
		{
			CoroutineHandle->SetProgress(0.0f);

			Start.Broadcast(CoroutineHandle);
			bCallStartEvent = true;
		}
		// Call event connected to Update Exce pin
		float Progress = FMath::Clamp((ElapsedTime - DelayTime) / DurationTime, 0.0f, 1.0f);
		CoroutineHandle->SetProgress(Progress);

		Update.Broadcast(CoroutineHandle);
	}
}

// FTickableGameObject Interface,whether open tick
bool UAsync_CreateCoroutineAdvance::IsTickable() const
{
	return CoroutineHandle ? CoroutineHandle->IsTickable() : false;
}

// FTickableGameObject Interface
bool UAsync_CreateCoroutineAdvance::IsTickableInEditor() const
{
	return false;
}
// FTickableGameObject Interface
TStatId UAsync_CreateCoroutineAdvance::GetStatId() const
{
	return Super::GetStatID();
}


UWorld* UAsync_CreateCoroutineAdvance::GetGWorld()
{
	UWorld* World =
#if WITH_EDITOR
		GIsEditor ? GWorld :
#endif // WITH_EDITOR
		(GEngine->GetWorldContexts().Num() > 0 ? GEngine->GetWorldContexts()[0].World() : nullptr);

	return World;
}