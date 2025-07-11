// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct HTN_API FHTNDelegates
{
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlanExecutionStarted, const class UHTNComponent&, const TSharedPtr<struct FHTNPlan>&);
	
	static FOnPlanExecutionStarted OnPlanExecutionStarted;
};