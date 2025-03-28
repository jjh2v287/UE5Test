// Fill out your copyright notice in the Description page of Project Settings.


#include "GrindAndBrewMethod.h"

UGrindAndBrewMethod::UGrindAndBrewMethod()
{
	MethodName = TEXT("Method 1: 원두 그라인딩 후 추출");
	Priority = 10; // 높은 우선순위
}

bool UGrindAndBrewMethod::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
	// 원두가 있어야 함
	return WorldState.bHaveBeans;
}