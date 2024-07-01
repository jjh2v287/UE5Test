// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "MultiImpactDataFactory.generated.h"

/**
 * 
 */
UCLASS()
class IMPACTSFXSYNTHEDITOR_API UMultiImpactDataFactory : public UFactory
{
	GENERATED_BODY()
	
public:
	UMultiImpactDataFactory();
	
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);
};
