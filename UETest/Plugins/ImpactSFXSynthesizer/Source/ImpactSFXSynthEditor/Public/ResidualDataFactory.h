// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ResidualDataFactory.generated.h"

class UResidualObj;
/**
 * 
 */
UCLASS()
class IMPACTSFXSYNTHEDITOR_API UResidualDataFactory : public UFactory
{
	GENERATED_BODY()
	
public:
	UResidualDataFactory();
	
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);

	TObjectPtr<UResidualObj> StagedObj;
};
