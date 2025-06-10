// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "Tasks/HTNTask_BlackboardBase.h"

void UHTNTask_BlackboardBase::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* const BBAsset = GetBlackboardAsset())
	{
		BlackboardKey.ResolveSelectedKey(*BBAsset);
	}
	else
	{
		UE_LOG(LogHTN, Warning, TEXT("Can't initialize task: %s, make sure that the HTN specifies a Blackboard asset!"), *GetShortDescription());
	}
}
