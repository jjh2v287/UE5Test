// Copyright 2023-2024, Le Binh Son, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorReimportHandler.h"
#include "Factories/Factory.h"

#include "ResidualObjFactory.generated.h"

class UResidualObj;

/**
 * 
 */
UCLASS()
class IMPACTSFXSYNTHEDITOR_API UResidualObjFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()
	
public:
	UResidualObjFactory();

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ End UFactory Interface
	
	bool ImportFromText(UResidualObj* ResidualObj, const TCHAR*& Buffer, const TCHAR* BufferEnd);

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) override;
	virtual EReimportResult::Type Reimport( UObject* Obj ) override;
	//~ End FReimportHandler Interface
	
	//~ Being UFactory Interface
	virtual void CleanUp() override;
	//~ End UFactory Interface

	//Must be reset after creating new obj
	TWeakObjectPtr<USoundWave> StagedSoundWave;
	
private:
	bool CreateFromWave(UResidualObj* NewAsset);
};
