// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/HTNPrimitiveTask.h"
#include "BrewTask.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UBrewTask : public UHTNPrimitiveTask
{
	GENERATED_BODY()
    
public:
	UBrewTask();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
	virtual void ApplyEffects_Implementation(FCoffeeWorldState& WorldState) override;
	virtual void OnTaskStart_Implementation(AAIController* Controller) override;
	virtual void OnTaskComplete_Implementation(AAIController* Controller, bool bSuccess) override;
};