// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "ImpactModalObjFactory.h"

#include "Runtime/Launch/Resources/Version.h"
#include "ImpactSFXSynthEditor.h"
#include "ImpactModalObj.h"
#include "EditorFramework/AssetImportData.h"

#if ENGINE_MINOR_VERSION < 3
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#endif

static bool bImpactModalObjFactorySuppressImportOverwriteDialog = false;

UImpactModalObjFactory::UImpactModalObjFactory()
{
	bCreateNew = false;
	SupportedClass = UImpactModalObj::StaticClass();
	bEditorImport = true;
	bText = true;
	
	Formats.Add(TEXT("impobj;Impact Obj file"));
	
	bAutomatedReimport = true;
}

UObject* UImpactModalObjFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	// if already exists, remember the user settings
	UImpactModalObj* ExisingImpactModalObj = FindObject<UImpactModalObj>(InParent, *InName.ToString());

	bool bUseExistingSettings = bImpactModalObjFactorySuppressImportOverwriteDialog;

	if (ExisingImpactModalObj && !bUseExistingSettings && !GIsAutomationTesting)
	{
		DisplayOverwriteOptionsDialog(FText::Format(
			NSLOCTEXT("ImpactModalObjFactory", "ImportOverwriteWarning", "You are about to import '{0}' over an existing impact modal obj."),
			FText::FromName(InName)));

		switch (OverwriteYesOrNoToAllState)
		{
			case EAppReturnType::Yes:
			case EAppReturnType::YesAll:
			{
				// Overwrite existing settings
				bUseExistingSettings = false;
				break;
			}
			case EAppReturnType::No:
			case EAppReturnType::NoAll:
			{
				// Preserve existing settings
				bUseExistingSettings = true;
				break;
			}
			default:
			{
				GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
				return nullptr;
			}
		}
	}

	// Reset the flag back to false so subsequent imports are not suppressed unless the code explicitly suppresses it
	bImpactModalObjFactorySuppressImportOverwriteDialog = false;

	// Use pre-existing obj if it exists and we want to keep settings,
	// otherwise create new obj and import raw data.
	UImpactModalObj* ImpactModalObj = (bUseExistingSettings && ExisingImpactModalObj) ? ExisingImpactModalObj : NewObject<UImpactModalObj>(InParent, InName, Flags);
	if(ImportFromText(ImpactModalObj, Buffer, BufferEnd))
	{
		// Store the current file path and timestamp for re-import purposes
		ImpactModalObj->AssetImportData->Update(CurrentFilename);
		
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, ImpactModalObj);
		return ImpactModalObj;
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
	return nullptr;
}

bool UImpactModalObjFactory::ImportFromText(UImpactModalObj* ImpactModalObj,  const TCHAR*& Buffer, const TCHAR* BufferEnd)
{
	TSharedPtr<FJsonObject> JsonParsed;
	TStringView<TCHAR> JsonString = TStringView<TCHAR>(Buffer, (BufferEnd - Buffer)); 
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::CreateFromView(JsonString);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		const bool bSuccess = JsonParsed->TryGetNumberField(TEXT("Version"), ImpactModalObj->Version);
		if(!bSuccess || ImpactModalObj->Version <= 0)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Impact Modal Obj has invalid Version = %d!"), ImpactModalObj->Version);
			return false;
		}

		if(!JsonParsed->HasField(TEXT("ModalData")))
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Impact Modal Obj has no data!"));
			return false;
		}
		
		TArray<TSharedPtr<FJsonValue, ESPMode::ThreadSafe>> JArrayData = JsonParsed->GetArrayField(TEXT("ModalData"));
		ImpactModalObj->Params.Empty(JArrayData.Num());
		for(const TSharedPtr<FJsonValue> JEntry : JArrayData)
			ImpactModalObj->Params.Emplace(JEntry->AsNumber());

		if(ImpactModalObj->Params.Num() % 3 != 0)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Impact Modal Obj has invalid length!"));
			return false;	
		}
		
		ImpactModalObj->NumModals = ImpactModalObj->Params.Num() / 3;
		return true;
	}

	return false;
}


bool UImpactModalObjFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	if (UImpactModalObj* ImpactModalObj = Cast<UImpactModalObj>(Obj))
		return true;
	
	return false;
}

void UImpactModalObjFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UImpactModalObj* ImpactModalObj = Cast<UImpactModalObj>(Obj);
	if (ImpactModalObj && ensure(NewReimportPaths.Num() == 1))
	{
		ImpactModalObj->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UImpactModalObjFactory::Reimport(UObject* Obj)
{
	// Only handle valid object
	if (!Obj || !Obj->IsA(UImpactModalObj::StaticClass()))
		return EReimportResult::Failed;	

	UImpactModalObj* ImpactModalObj = Cast<UImpactModalObj>(Obj);
	check(ImpactModalObj);

	const FString Filename = ImpactModalObj->AssetImportData->GetFirstFilename();
	const FString FileExtension = FPaths::GetExtension(Filename);
	bool bIsSupportedExtension = FileExtension.Equals(FString(TEXT("resdobj")), ESearchCase::IgnoreCase);
	bIsSupportedExtension = bIsSupportedExtension || FileExtension.Equals(FString(TEXT("impobj")), ESearchCase::IgnoreCase);
	if (!bIsSupportedExtension)
		return EReimportResult::Failed;

	// If there is no file path provided, can't reimport from source
	if (!Filename.Len())
		return EReimportResult::Failed;

	UE_LOG(LogImpactSFXSynthEditor, Log, TEXT("Performing reimport of [%s]"), *Filename);

	// Ensure that the file provided by the path exists
	if (IFileManager::Get().FileSize(*Filename) == INDEX_NONE)
	{
		UE_LOG(LogImpactSFXSynthEditor, Warning, TEXT("-- cannot reimport: source file cannot be found."));
		return EReimportResult::Failed;
	}

	// Always suppress dialogs on re-import
	bImpactModalObjFactorySuppressImportOverwriteDialog = true;

	bool OutCanceled = false;
	if (!ImportObject(ImpactModalObj->GetClass(), ImpactModalObj->GetOuter(), *ImpactModalObj->GetName(), RF_Public | RF_Standalone, Filename, nullptr, OutCanceled))
	{
		if (OutCanceled)
		{
			UE_LOG(LogImpactSFXSynthEditor, Warning, TEXT("-- import canceled"));
			return EReimportResult::Cancelled;
		}

		UE_LOG(LogImpactSFXSynthEditor, Warning, TEXT("-- import failed"));
		return EReimportResult::Failed;
	}

	UE_LOG(LogImpactSFXSynthEditor, Log, TEXT("-- imported successfully"));

	ImpactModalObj->AssetImportData->Update(Filename);
	ImpactModalObj->MarkPackageDirty();

	return EReimportResult::Succeeded;
}

void UImpactModalObjFactory::CleanUp()
{
	Super::CleanUp();
}

