// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/HTNCompoundTask.h"
#include "PrepareGrinderTask.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UPrepareGrinderTask : public UHTNCompoundTask
{
	GENERATED_BODY()
    
public:
	UPrepareGrinderTask();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
};