// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/HTNMethod.h"
#include "SetupMachineMethod.generated.h"

UCLASS(Blueprintable)
class ZKGAME_API USetupMachineMethod : public UHTNMethod
{
	GENERATED_BODY()
    
public:
	USetupMachineMethod();
    
	virtual bool CheckPreconditions_Implementation(const FCoffeeWorldState& WorldState) override;
};