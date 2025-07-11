// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "HTNFactory.generated.h"

/// A factory for Hierarchical Task Network assets
UCLASS()
class HTNEDITOR_API UHTNFactory : public UFactory
{
	GENERATED_BODY()
	
public:

	UHTNFactory();
	
	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End UFactory Interface
};
