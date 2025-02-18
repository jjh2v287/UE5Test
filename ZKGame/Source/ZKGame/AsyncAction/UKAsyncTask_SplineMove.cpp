// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAsyncTask_SplineMove.h"
#include "Engine/World.h"

UUKAsyncTask_SplineMove::UUKAsyncTask_SplineMove(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUKAsyncTask_SplineMove* UUKAsyncTask_SplineMove::SplineMove(const UObject* WorldContextObject, TSubclassOf<class UUKAsyncTask_SplineMove> Class)
{
	UUKAsyncTask_SplineMove* AsyncNode = NewObject<UUKAsyncTask_SplineMove>(WorldContextObject, Class);

	//  Call to globally register this object with a game instance, it will not be destroyed until SetReadyToDestroy is called
	if (WorldContextObject)
	{
		AsyncNode->RegisterWithGameInstance(WorldContextObject);
	}
	else
	{
		const UWorld* World = AsyncNode->GetWorld();
		AsyncNode->RegisterWithGameInstance(World);
	}

	// return this node
	return AsyncNode;
}

void UUKAsyncTask_SplineMove::Activate()
{
	Super::Activate();
	OnActivate();
}

void UUKAsyncTask_SplineMove::Cancel()
{
	Super::Cancel();
}

void UUKAsyncTask_SplineMove::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	OnTick(DeltaTime);
	// OnMoveFinish.Broadcast();
	// bTickable = false;
	// Cancel();

}