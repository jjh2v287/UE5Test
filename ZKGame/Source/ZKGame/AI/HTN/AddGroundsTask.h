// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/HTNPrimitiveTask.h"
#include "AddGroundsTask.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UAddGroundsTask : public UHTNPrimitiveTask
{
	GENERATED_BODY()
    
public:
	UAddGroundsTask();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
	virtual void ApplyEffects_Implementation(FCoffeeWorldState& WorldState) override;
	virtual void OnTaskStart_Implementation(AAIController* Controller) override;
};