// Fill out your copyright notice in the Description page of Project Settings.


#include "UsePreGroundMethod.h"

UUsePreGroundMethod::UUsePreGroundMethod()
{
	MethodName = TEXT("Method 2: 분쇄된 원두 사용");
	Priority = 5; // 낮은 우선순위
}

bool UUsePreGroundMethod::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 분쇄된 원두가 있어야 함
	return WorldState.bHaveGroundCoffee;
}