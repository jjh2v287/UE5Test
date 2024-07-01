// Copyright 2023-2024, Le Binh Son, All rights reserved.


#include "ResidualObjFactory.h"

#include "Runtime/Launch/Resources/Version.h"
#include "EditorDialogLibrary.h"
#include "ImpactSFXSynthEditor.h"
#include "ResidualAnalyzer.h"
#include "ResidualObj.h"
#include "EditorFramework/AssetImportData.h"
#include "ImpactSFXSynth/Public/Utils.h"
#include "DSP/Dsp.h"
#include "DSP/BufferVectorOperations.h"

#if ENGINE_MINOR_VERSION < 3
#include "Misc/FeedbackContext.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#endif

static bool bResiduallObjFactorySuppressImportOverwriteDialog = false;

UResidualObjFactory::UResidualObjFactory()
{
	bCreateNew = false;
	SupportedClass = UResidualObj::StaticClass();
	bEditorImport = true;
	bText = true;
	
	Formats.Add(TEXT("resdobj;Residual Obj file"));
	
	bAutomatedReimport = true;
}

UObject* UResidualObjFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	// if already exists, remember the user settings
	UResidualObj* ExistingResidualObj = FindObject<UResidualObj>(InParent, *InName.ToString());

	bool bUseExistingSettings = bResiduallObjFactorySuppressImportOverwriteDialog;

	if (ExistingResidualObj && !bUseExistingSettings && !GIsAutomationTesting)
	{
		DisplayOverwriteOptionsDialog(FText::Format(
			NSLOCTEXT("ResidualObjFactory", "ImportOverwriteWarning", "You are about to import '{0}' over an existing residual obj."),
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
	bResiduallObjFactorySuppressImportOverwriteDialog = false;

	// Use pre-existing obj if it exists and we want to keep settings,
	// otherwise create new obj and import raw data.
	UResidualObj* ResidualObj = (bUseExistingSettings && ExistingResidualObj) ? ExistingResidualObj : NewObject<UResidualObj>(InParent, InName, Flags);
	if(ImportFromText(ResidualObj, Buffer, BufferEnd))
	{
		// Store the current file path and timestamp for re-import purposes
		ResidualObj->AssetImportData->Update(CurrentFilename);
		
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, ResidualObj);
		return ResidualObj;
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
	return nullptr;
}

UObject* UResidualObjFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	bool Success = false;
	UResidualObj* NewAsset = NewObject<UResidualObj>(InParent, Name, Flags);

	if (StagedSoundWave.IsValid())
	{
		Warn->ShowBuildProgressWindow();
		Success = CreateFromWave(NewAsset);
		StagedSoundWave.Reset();
		Warn->CloseBuildProgressWindow();
	}

	if(Success)
	{
		return NewAsset;
	}
	
	const FText Title = FText(NSLOCTEXT("ResidualObjFactory", "CreateNewFailedTitle", "Error"));
	const FText Message = FText(NSLOCTEXT("ResidualObjFactory", "CreateNewFailedMessage", "Failed to create a new residual object from the selected audio. Please check the output log for more details."));
	
#if ENGINE_MINOR_VERSION < 3
	UEditorDialogLibrary::ShowMessage(Title, Message, EAppMsgType::Ok, EAppReturnType::No);
#else
	UEditorDialogLibrary::ShowMessage(Title, Message, EAppMsgType::Ok, EAppReturnType::No, EAppMsgCategory::Error);
#endif
	return nullptr;
}

bool UResidualObjFactory::ImportFromText(UResidualObj* ResidualObj,  const TCHAR*& Buffer, const TCHAR* BufferEnd)
{
	TSharedPtr<FJsonObject> JsonParsed;
	TStringView<TCHAR> JsonString = TStringView<TCHAR>(Buffer, (BufferEnd - Buffer)); 
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::CreateFromView(JsonString);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		int32 Version = 0;
		bool bSuccess = JsonParsed->TryGetNumberField(TEXT("Version"), Version);
		if(!bSuccess || Version <= 0)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid Version = %d!"), Version);
			return false;
		}
		
		int32 NumFFT = 0;
		bSuccess = JsonParsed->TryGetNumberField(TEXT("NumFFT"), NumFFT);
		if(!bSuccess || NumFFT <= 0 || !LBSImpactSFXSynth::IsPowerOf2(NumFFT))
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid NumFFT = %d!"), NumFFT);
			return false;
		}

		int32 HopSize = 0;
		bSuccess = JsonParsed->TryGetNumberField(TEXT("HopSize"), HopSize);
		if(!bSuccess || HopSize <= 0)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid HopSize = %d!"), HopSize);
			return false;
		}
		
		int32 NumErb = 0;
		bSuccess = JsonParsed->TryGetNumberField(TEXT("NumErb"), NumErb);
		if(!bSuccess || NumErb < 4) 
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid NumErb = %d!"), NumErb);
			return false;
		}

		int32 NumFrame = 0;
		bSuccess = JsonParsed->TryGetNumberField(TEXT("NumFrame"), NumFrame);
		if(!bSuccess || NumFrame < UResidualObj::NumMinErbFrames)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid NumFrame = %d!"), NumFrame);
			return false;
		}

		float SamplingRate = 0;
		bSuccess = JsonParsed->TryGetNumberField(TEXT("Fs"), SamplingRate);
		if(!bSuccess || SamplingRate < 4000)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid Sampling Rate = %f!"), SamplingRate);
			return false;
		}

		float ErbMin = -1;
		bSuccess = JsonParsed->TryGetNumberField(TEXT("ErbMin"), ErbMin);
		if(!bSuccess || ErbMin < 0)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid ErbMin = %f!"), ErbMin);
			return false;
		}

		float ErbMax = 0;
		bSuccess = JsonParsed->TryGetNumberField(TEXT("ErbMax"), ErbMax);
		if(!bSuccess || ErbMax <= 0)
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has invalid ErbMax = %f!"), ErbMax);
			return false;
		}

		if(!JsonParsed->HasField(TEXT("Data")))
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has no data!"));
			return false;
		}
		
		TArray<TSharedPtr<FJsonValue, ESPMode::ThreadSafe>> JArrayData = JsonParsed->GetArrayField(TEXT("Data"));
		const int32 ExpectDataLength = NumErb * NumFrame;
		if(ExpectDataLength != JArrayData.Num())
		{
			UE_LOG(LogImpactSFXSynthEditor, Error, TEXT("Imported Residual Obj has different data length (%d) compared to expected length (%d)!"), JArrayData.Num(), ExpectDataLength);
			return false;
		}

		ResidualObj->SetProperties(Version, NumFFT, HopSize, NumErb, NumFrame, SamplingRate, ErbMax, ErbMin);
		ResidualObj->Data.SetNumUninitialized(JArrayData.Num());
		int Index = 0;
		for(const TSharedPtr<FJsonValue> JEntry : JArrayData)
		{
			ResidualObj->Data[Index] = JEntry->AsNumber();
			Index++;
		}
		
		return true;
	}

	return false;
}


bool UResidualObjFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	if (UResidualObj* ResidualObj = Cast<UResidualObj>(Obj))
		return true;
	
	return false;
}

void UResidualObjFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UResidualObj* ResidualObj = Cast<UResidualObj>(Obj);
	if (ResidualObj && ensure(NewReimportPaths.Num() == 1))
	{
		ResidualObj->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UResidualObjFactory::Reimport(UObject* Obj)
{
	// Only handle valid object
	if (!Obj || !Obj->IsA(UResidualObj::StaticClass()))
		return EReimportResult::Failed;	

	UResidualObj* ResidualObj = Cast<UResidualObj>(Obj);
	check(ResidualObj);

	const FString Filename = ResidualObj->AssetImportData->GetFirstFilename();
	const FString FileExtension = FPaths::GetExtension(Filename);
	const bool bIsSupportedExtension = FileExtension.Equals(FString(TEXT("resdobj")), ESearchCase::IgnoreCase);
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
	bResiduallObjFactorySuppressImportOverwriteDialog = true;

	bool OutCanceled = false;
	if (!ImportObject(ResidualObj->GetClass(), ResidualObj->GetOuter(), *ResidualObj->GetName(), RF_Public | RF_Standalone, Filename, nullptr, OutCanceled))
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

	ResidualObj->AssetImportData->Update(Filename);
	ResidualObj->MarkPackageDirty();

	return EReimportResult::Succeeded;
}

bool UResidualObjFactory::CreateFromWave(UResidualObj* NewAsset)
{
	USoundWave* Wave = StagedSoundWave.Get();
	LBSImpactSFXSynth::FResidualAnalyzer Analyzer = LBSImpactSFXSynth::FResidualAnalyzer(Wave);
	return Analyzer.StartAnalyzing(NewAsset);
}

void UResidualObjFactory::CleanUp()
{
	Super::CleanUp();
}

