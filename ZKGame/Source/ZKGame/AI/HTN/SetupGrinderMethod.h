// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/HTNMethod.h"
#include "Core/HTNTypes.h"
#include "SetupGrinderMethod.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API USetupGrinderMethod : public UHTNMethod
{
	GENERATED_BODY()
    
public:
	USetupGrinderMethod();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
};