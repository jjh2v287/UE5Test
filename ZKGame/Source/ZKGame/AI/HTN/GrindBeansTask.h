﻿// GrindBeansTask.h
#pragma once

#include "CoreMinimal.h"
#include "Core/HTNPrimitiveTask.h"
#include "GrindBeansTask.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UGrindBeansTask : public UHTNPrimitiveTask
{
	GENERATED_BODY()
    
public:
	UGrindBeansTask();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
	virtual void ApplyEffects_Implementation(FCoffeeWorldState& WorldState) override;
	virtual void OnTaskStart_Implementation(AAIController* Controller) override;
};