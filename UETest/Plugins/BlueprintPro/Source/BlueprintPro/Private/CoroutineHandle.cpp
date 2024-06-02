// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.


#include "CoroutineHandle.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "BlueprintProSubsystem.h"


void UCoroutineHandle::PauseCoroutine()
{
	bPaused = true;
}

void UCoroutineHandle::UnPauseCoroutine()
{
	bPaused = false;
}

void UCoroutineHandle::Invalidate()
{
	Reset();

	UWorld* World =
#if WITH_EDITOR
		GIsEditor ? GWorld :
#endif // WITH_EDITOR
		(GEngine->GetWorldContexts().Num() > 0 ? GEngine->GetWorldContexts()[0].World() : nullptr);

	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UBlueprintProSubsystem* BlueprintProSubsystem = GameInstance ? GameInstance->GetSubsystem<UBlueprintProSubsystem>() : nullptr;
	if (BlueprintProSubsystem)
	{
		BlueprintProSubsystem->ReturnCoroutine(this);
	}
}


void UCoroutineHandle::SetProgress(float Value)
{
	Progress = FMath::Clamp(Value, 0.0f, 1.0f);
}

bool UCoroutineHandle::IsCoroutinePaused()
{
	return bPaused;
}

bool UCoroutineHandle::IsCoroutineExists()
{
	return AsyncCoroutineAdvance ? true : false;
}

void UCoroutineHandle::Reset()
{
	bTickable = false;
	bPaused = false;
	Progress = 0.0f;

	RefenceObject = nullptr;

	if (UBlueprintAsyncActionBase* AsyncNode = Cast<UBlueprintAsyncActionBase>(AsyncCoroutineAdvance))
	{
		AsyncNode->SetReadyToDestroy();
	}
	AsyncCoroutineAdvance = nullptr;
}
