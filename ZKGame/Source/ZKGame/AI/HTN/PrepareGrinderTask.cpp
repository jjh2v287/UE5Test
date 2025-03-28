// Fill out your copyright notice in the Description page of Project Settings.


#include "PrepareGrinderTask.h"

UPrepareGrinderTask::UPrepareGrinderTask()
{
    TaskName = TEXT("그라인더 준비");
    Cost = 3.0f;
}

bool UPrepareGrinderTask::CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState)
{
    // 원두가 있어야 함
    return WorldState.bHaveBeans;
}