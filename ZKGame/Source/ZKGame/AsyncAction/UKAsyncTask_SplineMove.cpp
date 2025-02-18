// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAsyncTask_SplineMove.h"
#include "Engine/World.h"

UUKAsyncTask_SplineMove::UUKAsyncTask_SplineMove(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUKAsyncTask_SplineMove* UUKAsyncTask_SplineMove::SplineMove(const UObject* WorldContextObject, TSubclassOf<class UUKAsyncTask_SplineMove> Class, const FName SplineName, const int32 StartIndex, const int32 EndIndex)
{
	UUKAsyncTask_SplineMove* AsyncNode = NewObject<UUKAsyncTask_SplineMove>(WorldContextObject->GetWorld(), Class.Get());

	AsyncNode->SplineName = SplineName;
	AsyncNode->StartIndex = StartIndex;
	AsyncNode->EndIndex = EndIndex;
	
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

void UUKAsyncTask_SplineMove::FinishTask(const bool bSucceeded)
{
	if (bSucceeded)
	{
		OnMoveFinish.Broadcast();
		bTickable = false;
		Cancel();
	}
	else
	{
		OnMoveFail.Broadcast();
		bTickable = false;
		Cancel();
	}
}

void UUKAsyncTask_SplineMove::Cancel()
{
	OnMoveFinish.Clear();
	OnMoveFail.Clear();
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