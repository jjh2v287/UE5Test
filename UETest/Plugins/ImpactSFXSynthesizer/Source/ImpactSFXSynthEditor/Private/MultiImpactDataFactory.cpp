// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "MultiImpactDataFactory.h"

#include "MultiImpactData.h"

UMultiImpactDataFactory::UMultiImpactDataFactory()
{
	SupportedClass = UMultiImpactData::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMultiImpactDataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
                                                   UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMultiImpactData>(InParent, Class, Name, Flags, Context);
}
