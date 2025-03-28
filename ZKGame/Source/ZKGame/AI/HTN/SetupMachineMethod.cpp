// Fill out your copyright notice in the Description page of Project Settings.


#include "SetupMachineMethod.h"

USetupMachineMethod::USetupMachineMethod()
{
	MethodName = TEXT("Method 4: 머신 설정");
	Priority = 10;
}

bool USetupMachineMethod::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 머신이 깨끗하고 필터가 있어야 함
	return WorldState.bIsMachineClean && WorldState.bHaveFilter;
}