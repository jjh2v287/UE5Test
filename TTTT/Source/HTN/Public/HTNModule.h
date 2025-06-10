// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IHTNModule : public IModuleInterface
{
	static FORCEINLINE IHTNModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IHTNModule>(TEXT("HTN"));
	}
};
