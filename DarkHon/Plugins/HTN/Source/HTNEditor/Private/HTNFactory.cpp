// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNFactory.h"
#include "HTN.h"

UHTNFactory::UHTNFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UHTN::StaticClass();
}

UObject* UHTNFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UHTN>(InParent, Class, Name, Flags);
}