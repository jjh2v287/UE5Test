// Copyright Kong Studios, Inc. All Rights Reserved.

#include "UKAsyncTask_Base.h"
#include "Engine/World.h"

UUKAsyncTask_Base::UUKAsyncTask_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UUKAsyncTask_Base::Activate()
{
	bTickable = true;
	Super::Activate();
}

void UUKAsyncTask_Base::Cancel()
{
	Super::Cancel();

	bTickable = false;
}

// FTickableGameObject Interface
void UUKAsyncTask_Base::Tick(float DeltaTime)
{
	
}

TStatId UUKAsyncTask_Base::GetStatId() const
{
	return GetStatID();
}

bool UUKAsyncTask_Base::IsTickable() const
{
	return bTickable;
}

bool UUKAsyncTask_Base::IsTickableInEditor() const
{
	return false;
}
// ~FTickableGameObject Interface