// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNExtension_BlueprintBase.h"
#include "BlueprintNodeHelpers.h"

#define IS_BP_IMPLEMENTED(FunctionName) \
	(BlueprintNodeHelpers::HasBlueprintFunction(GET_FUNCTION_NAME_CHECKED(UHTNExtension_BlueprintBase, FunctionName), *this, *StaticClass()))

UHTNExtension_BlueprintBase::UHTNExtension_BlueprintBase() :
	bOnInitializeBPImplemented(IS_BP_IMPLEMENTED(OnInitialize)),
	bOnHTNStartedBPImplemented(IS_BP_IMPLEMENTED(OnHTNStarted)),
	bOnTickBPImplemented(IS_BP_IMPLEMENTED(OnTick)),
	bOnCleanupBPImplemented(IS_BP_IMPLEMENTED(OnCleanup))
{
	bNotifyTick = bOnTickBPImplemented;
}

#undef IS_BP_IMPLEMENTED

void UHTNExtension_BlueprintBase::Initialize()
{
	if (bOnInitializeBPImplemented)
	{
		OnInitialize();
	}
}

void UHTNExtension_BlueprintBase::HTNStarted()
{
	if (bOnHTNStartedBPImplemented)
	{
		OnHTNStarted();
	}
}

void UHTNExtension_BlueprintBase::Tick(float DeltaTime)
{
	if (bOnTickBPImplemented)
	{
		OnTick(DeltaTime);
	}
}

void UHTNExtension_BlueprintBase::Cleanup()
{
	if (bOnCleanupBPImplemented)
	{
		OnCleanup();
	}
}
