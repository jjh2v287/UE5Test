// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "ResidualDataFactory.h"

#include "ResidualData.h"

UResidualDataFactory::UResidualDataFactory()
{
	SupportedClass = UResidualData::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UResidualDataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
                                                UObject* Context, FFeedbackContext* Warn)
{
	UResidualData* NewData = NewObject<UResidualData>(InParent, Class, Name, Flags, Context);
	NewData->SetResidualObj(StagedObj);
	StagedObj = nullptr;
	return NewData;
}
