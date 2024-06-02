// (c) 2021 Sergio Lena. All Rights Reserved.

#include "Action.h"
#include "ActionsManager.h"

void UAction::Initialize()
{
	if (!ensureAlwaysMsgf(!bExecuting, TEXT("UAction::Execute(): already executing this Action.")))
	{
		return;
	}

	bExecuting = true;
	bQueuedExecution = true;
	UActionsManager::RegisterAction(this);
}

void UAction::Abort()
{
	if (!ensureAlwaysMsgf(bExecuting, TEXT("UAction::Abort(): this Action is not being executed now.")))
	{
		return;
	}

	bExecuting = false;
	OnStopExecute(true);
	UActionsManager::DeregisterAction(this);
}

void UAction::Tick(float DeltaTime)
{
	if (bQueuedExecution)
	{
		bQueuedExecution = false;
		OnExecute();
		return;
	}

	if(bExecuting) 
	{
		OnUpdate();
	}
}

UWorld* UAction::GetWorld() const
{
	UWorld* GameWorld = nullptr;
	TIndirectArray<FWorldContext> Worlds = GEngine->GetWorldContexts();
	for (auto World : Worlds)
	{
		if (World.WorldType == EWorldType::Game || World.WorldType == EWorldType::PIE)
		{
			GameWorld = World.World();
			break;
		}
	}

	return GameWorld;
}

void UAction::Finish(const FName FinishMessage)
{
	if (!ensureAlwaysMsgf(bExecuting, TEXT("UAction::Finish(): this Action is not being executed now.")))
	{
		return;
	}

	bExecuting = false;
	OnStopExecute( false);
	UActionsManager::DeregisterAction(this);
}
