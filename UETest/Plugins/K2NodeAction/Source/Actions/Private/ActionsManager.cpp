// (c) 2021 Sergio Lena. All Rights Reserved.
#include "ActionsManager.h"
#include "Action.h"


void UActionsManager::RegisterAction(UAction* Action)
{
	if (const auto Manager = Get())
	{
		if(Manager->DeleteBuffer.Contains(Action))
		{
			Manager->DeleteBuffer.Remove(Action);
		}

		Manager->RunningActions.Add(Action);
	}
}

void UActionsManager::DeregisterAction(UAction* Action)
{
	if (const auto Manager = Get())
	{
		if(Manager->RunningActions.Contains(Action))
		{
		 	Manager->DeleteBuffer.Add(Action, Manager->LifetimeAfterExecutionStopped);
		}
	}
}

void UActionsManager::Tick(const float DeltaTime)
{
	TArray<UAction*> PendingKill;

	for(auto & Action : DeleteBuffer)
	{
		if((Action.Value -= DeltaTime) <= 0.f)
		{
			PendingKill.AddUnique(Action.Key);
		}
	}

	for(const auto Action : PendingKill)
	{
		DeleteBuffer.Remove(Action);
		Action->ConditionalBeginDestroy();
	}
}

UActionsManager* UActionsManager::Get()
{
	const UWorld* GameWorld = nullptr;
	TIndirectArray<FWorldContext> Worlds = GEngine->GetWorldContexts();
	for (auto World : Worlds)
	{
		if (World.WorldType == EWorldType::Game || World.WorldType == EWorldType::PIE)
		{
			GameWorld = World.World();
			break;
		}
	}

	if (!ensureAlwaysMsgf(IsValid(GameWorld), TEXT("UActionsManager::StaticallyGetRunningActions(): game world not valid.")))
	{
		return nullptr;
	}

	UActionsManager* Manager = GameWorld->GetSubsystem<UActionsManager>();
	if (!ensureAlwaysMsgf(IsValid(Manager), TEXT("UActionsManager::StaticallyGetRunningActions(): manager not valid.")))
	{
		return nullptr;
	}

	return Manager;
}
