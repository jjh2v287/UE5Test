// Copyright 2021 Marcin Swiderski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Stats/Stats.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTickOptToolkit, Verbose, All);
DECLARE_CYCLE_STAT_EXTERN(TEXT("TickOptToolkit Time"), STAT_TickOptToolkit, STATGROUP_Game, );

extern const FName NAME_NiagaraComponent;

class FTickOptToolkit : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
