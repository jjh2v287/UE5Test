// Copyright 2024, Cubeap Electronics, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTracerProModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
