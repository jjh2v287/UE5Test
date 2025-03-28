// PlugInGrinderTask.h
#pragma once

#include "CoreMinimal.h"
#include "Core/HTNPrimitiveTask.h"
#include "PlugInGrinderTask.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API UPlugInGrinderTask : public UHTNPrimitiveTask
{
	GENERATED_BODY()
    
public:
	UPlugInGrinderTask();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
	virtual void ApplyEffects_Implementation(FCoffeeWorldState& WorldState) override;
	virtual void OnTaskStart_Implementation(AAIController* Controller) override;
};