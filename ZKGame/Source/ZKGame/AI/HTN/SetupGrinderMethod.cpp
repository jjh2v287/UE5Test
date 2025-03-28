// Fill out your copyright notice in the Description page of Project Settings.


#include "SetupGrinderMethod.h"


USetupGrinderMethod::USetupGrinderMethod()
{
	MethodName = TEXT("Method 3: 그라인더 설정");
	Priority = 10;
}

bool USetupGrinderMethod::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 항상 적용 가능
	return true;
}