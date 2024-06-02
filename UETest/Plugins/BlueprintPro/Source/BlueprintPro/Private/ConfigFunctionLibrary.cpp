// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#include "ConfigFunctionLibrary.h"
#include "DataTableUtils.h"
#include "Misc/DefaultValueHelper.h"
#include "DataTableUtils.h"
#include "Misc/Paths.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/FileManagerGeneric.h"
#include "HAL/FileManager.h"
//#include "Launch/Resources/Version.h"


/** Declare General Log Category */
DEFINE_LOG_CATEGORY_STATIC(LogConfigFunctionLibrary, Log, All);



//If bUseGeneratedConfigDir is true, return engine saves generated config directory files, otherwise return root configuration directory files
FString ConvertRelativePathToFull(const bool bUseGeneratedConfigDir, const FString& FileName)
{
	/** Based on FPluginManager::ConfigureEnabledPlugins(), */
	if (bUseGeneratedConfigDir)
	{
		return FString::Printf(TEXT("%s%s/%s"), *FPaths::GeneratedConfigDir(), ANSI_TO_TCHAR(FPlatformProperties::PlatformName()), *FileName);
	}
	else
	{
		// UE4 v4.25.2 FPaths::ProjectConfigDir() folder can be directly packaged.
		return FString::Printf(TEXT("%s/%s"), *FPaths::ProjectConfigDir(), *FileName);
	}
}


void UConfigFunctionLibrary::SetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, const int32& Value, bool& bSuccessful)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
	return;

}

void UConfigFunctionLibrary::Generic_SetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, FProperty* InProperty, void* InPropertyData, bool& bSuccessful)
{
	if (!GConfig)
	{
		UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("GConfig is NULL"));
		return;
	}
	else if (Section.IsEmpty() || Key.IsEmpty() || FileName.IsEmpty())
	{
		UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Cannot set invalid config value [Section:%s, Key:%s, FileName:%s]"), *Section, *Key, *FileName);
		return;
	}

	bSuccessful = true;
	FString ConfigValue;

	/** Based on FBlueprintEditorUtils::PropertyValueToString_Direct() */
	if (!InProperty->IsA(FStructProperty::StaticClass()))
	{
		//bSuccessful = InProperty->ExportText_Direct(ConfigValue, InPropertyData, InPropertyData, nullptr, PPF_None);
		bSuccessful = InProperty->ExportText_Direct(ConfigValue, InPropertyData, InPropertyData, nullptr, PPF_SerializedAsImportText);
		if (!bSuccessful)
		{
			return;
		}
	}
	else
	{
		if (const FStructProperty* StructProp = CastField<const FStructProperty>(InProperty))
		{
			/// Struct properties must be handled differently, Only support struct type as follow.
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				FVector PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();
			}
			else if (StructProp->Struct == TBaseStructure<FVector2D>::Get())
			{
				FVector2D PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();

			}
			else if (StructProp->Struct == TBaseStructure<FVector4>::Get())
			{
				FVector4 PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();
			}
			else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
			{
				FRotator PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();
			}
			else if (StructProp->Struct == TBaseStructure<FTransform>::Get())
			{
				FTransform PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();
			}
			else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
			{
				FLinearColor PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();
			}
			else if (StructProp->Struct == TBaseStructure<FColor>::Get())
			{
				FColor PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();
			}
			else if (StructProp->Struct == TBaseStructure<FDateTime>::Get())
			{
				FDateTime PropertyValue;
				InProperty->CopyCompleteValue(&PropertyValue, InPropertyData);
				ConfigValue = PropertyValue.ToString();
			}
			else
			{
				bSuccessful = false;
				UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("[%s] is unsupported property value type."), *InProperty->GetName());
			}
		}

	}

	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	GConfig->SetString(*Section, *Key, *ConfigValue, FullFileName);
	//Sometimes the config file wonn't save changes if you don't call this function after you've set all your config keys.
	GConfig->Flush(false, FullFileName);
}



void UConfigFunctionLibrary::GetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, bool& bSuccessful, int32& Value)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
	return;
}


void UConfigFunctionLibrary::Generic_GetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, bool& bSuccessful, FProperty* OutProperty, void* OutPropertyData)
{
	if (!GConfig)
	{
		UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("GConfig is NULL"));
		return;
	}

	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	// Config property value
	FString ConfigValue;
	if (!GConfig->GetString(*Section, *Key, ConfigValue, FullFileName))
	{
		UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Missing config value in [Section:%s, Key:%s, FileName:%s]"), *Section, *Key, *FullFileName);
		return;
	}

	bSuccessful = true;
	/** Based on FBlueprintEditorUtils::PropertyValueFromString_Direct()*/
	if (!OutProperty->IsA(FStructProperty::StaticClass()))
	{
		if (const FBoolProperty* BoolProp = CastField<const FBoolProperty>(OutProperty))
		{
			BoolProp->SetPropertyValue(OutPropertyData, ConfigValue.ToBool());
		}
		else if (const FIntProperty* IntProp = CastField<const FIntProperty>(OutProperty))
		{
			IntProp->SetPropertyValue(OutPropertyData, FCString::Atoi(*ConfigValue));
		}
		else if (const FInt64Property* Int64Prop = CastField<const FInt64Property>(OutProperty))
		{
			Int64Prop->SetPropertyValue(OutPropertyData, FCString::Atoi64(*ConfigValue));
		}
		else if (const FFloatProperty* FloatProp = CastField<const FFloatProperty>(OutProperty))
		{
			FloatProp->SetPropertyValue(OutPropertyData, FCString::Atod(*ConfigValue));
		}
		else if (const FDoubleProperty* DoubleProp = CastField<const FDoubleProperty>(OutProperty))
		{
			DoubleProp->SetPropertyValue(OutPropertyData, FCString::Atod(*ConfigValue));
		}
		else if (const FByteProperty* ByteProp = CastField<const FByteProperty>(OutProperty))
		{
			int32 IntValue = 0;
			if (const UEnum* Enum = ByteProp->Enum)
			{
				IntValue = Enum->GetValueByName(FName(*ConfigValue));
				bSuccessful = (INDEX_NONE != IntValue);

				// If the parse did not succeed, clear out the int to keep the enum value valid
				if (!bSuccessful)
				{
					IntValue = 0;
				}
			}
			else
			{
				bSuccessful = FDefaultValueHelper::ParseInt(ConfigValue, IntValue);
			}
			bSuccessful = bSuccessful && (IntValue <= 255) && (IntValue >= 0);
			ByteProp->SetPropertyValue(OutPropertyData, IntValue);
		}
		else if (const FEnumProperty* EnumProp = CastField<const FEnumProperty>(OutProperty))
		{
			int64 IntValue = EnumProp->GetEnum()->GetValueByName(FName(*ConfigValue));
			bSuccessful = (INDEX_NONE != IntValue);

			// If the parse did not succeed, clear out the int to keep the enum value valid
			if (!bSuccessful)
			{
				IntValue = 0;
			}
			bSuccessful = bSuccessful && (IntValue <= 255) && (IntValue >= 0);
			EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(OutPropertyData, IntValue);
		}
		else if (const FNameProperty* NameProp = CastField<const FNameProperty>(OutProperty))
		{
			NameProp->SetPropertyValue(OutPropertyData, FName(*ConfigValue));
		}
		else if (const FStrProperty* StringProp = CastField<const FStrProperty>(OutProperty))
		{
			StringProp->SetPropertyValue(OutPropertyData, ConfigValue);
		}
		else if (FTextProperty* TextProp = CastField<FTextProperty>(OutProperty))
		{
			FStringOutputDevice ImportError;

//#if  ( ENGINE_MAJOR_VERSION == 5 &&  1<= ENGINE_MINOR_VERSION )
			const TCHAR* EndOfParsedBuff = OutProperty->ImportText_Direct(*ConfigValue, OutPropertyData, nullptr, PPF_SerializedAsImportText, &ImportError);
//#else
//			const TCHAR* EndOfParsedBuff = OutProperty->ImportText(*ConfigValue, OutPropertyData, PPF_SerializedAsImportText, nullptr, &ImportError);
//#endif
			bSuccessful = EndOfParsedBuff && ImportError.IsEmpty();
		}
		else
		{
			bSuccessful = false;
			UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Unsupported config type: %s"), *OutProperty->GetName());
		}
	}
	else
	{
		if (const FStructProperty* StructProp = CastField<const FStructProperty>(OutProperty))
		{
			// Struct properties must be handled differently, Only support struct type as follow.
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				FVector PropertyValue;
				if (!PropertyValue.InitFromString(ConfigValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FVector"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else if (StructProp->Struct == TBaseStructure<FVector2D>::Get())
			{
				FVector2D PropertyValue;
				if (!PropertyValue.InitFromString(ConfigValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FVector2D"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else if (StructProp->Struct == TBaseStructure<FVector4>::Get())
			{
				FVector4 PropertyValue;
				if (!PropertyValue.InitFromString(ConfigValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FVector4"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
			{
				FRotator PropertyValue;
				if (!PropertyValue.InitFromString(ConfigValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FRotator"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else if (StructProp->Struct == TBaseStructure<FTransform>::Get())
			{
				FTransform PropertyValue;
				if (!PropertyValue.InitFromString(ConfigValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FTransform"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
			{
				FLinearColor PropertyValue;
				if (!PropertyValue.InitFromString(ConfigValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FLinearColor"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else if (StructProp->Struct == TBaseStructure<FColor>::Get())
			{
				FColor PropertyValue;
				if (!PropertyValue.InitFromString(ConfigValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FColor"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else if (StructProp->Struct == TBaseStructure<FDateTime>::Get())
			{
				FDateTime PropertyValue;
				if (!FDateTime::Parse(ConfigValue, PropertyValue))
				{
					bSuccessful = false;
					UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("Failed to parser [%s] as FDateTime"), *ConfigValue);
					return;
				}
				OutProperty->CopyCompleteValue(OutPropertyData, &PropertyValue);
			}
			else
			{
				bSuccessful = false;
				UE_LOG(LogConfigFunctionLibrary, Warning, TEXT("[%s] is unsupported property value type."), *OutProperty->GetName());
			}
		}
	}
}


void UConfigFunctionLibrary::SetStringArray(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, const TArray<FString>& Value)
{
	if (!GConfig)	return;
	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	GConfig->SetArray(*Section, *Key, Value, FullFileName);
	GConfig->Flush(false, FullFileName);
}

int32 UConfigFunctionLibrary::GetStringArray(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, TArray<FString>& Value)
{
	if (!GConfig)	return 0;
	// Calculates the name of a dest (generated) .ini file for a given base (ie Engine, Game, etc)
	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	return GConfig->GetArray(*Section, *Key, Value, FullFileName);
}

FString UConfigFunctionLibrary::GetPropertyName(const int32& Property)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
	return FString();
}

void UConfigFunctionLibrary::GetConfigFileNames(TArray<FString>& ConfigFileNames)
{
	if (!GConfig)	return;

	//  Get ini file of root config directory
	IFileManager::Get().FindFiles(ConfigFileNames, *FPaths::ProjectConfigDir(), TEXT(".ini"));
	for (FString& Name : ConfigFileNames)
	{
		// add file path
		Name = FPaths::ProjectConfigDir().Append(Name);
	}

	//  Get ini file of saved directory / Config
	TArray<FString> GeneratedConfigFileNames;
	GConfig->GetConfigFilenames(GeneratedConfigFileNames);

	ConfigFileNames.Append(GeneratedConfigFileNames);
}

bool UConfigFunctionLibrary::GetSectionNames(const bool bUseGeneratedConfigDir, const FString& FileName, TArray<FString>& SectionNames)
{
	if (!GConfig)	return false;
	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	return GConfig->GetSectionNames(FullFileName, SectionNames);
}


bool UConfigFunctionLibrary::DoesSectionExist(const FString& Section, const bool bUseGeneratedConfigDir, const FString& FileName)
{
	if (!GConfig)	return false;
	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	return GConfig->DoesSectionExist(*Section, FullFileName);
}


bool UConfigFunctionLibrary::RemoveKey(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName)
{
	if (!GConfig)	return false;
	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	return GConfig->RemoveKey(*Section, *Key, FullFileName);
}


bool UConfigFunctionLibrary::EmptySection(const FString& Section, const bool bUseGeneratedConfigDir, const FString& FileName)
{
	if (!GConfig)	return false;
	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	return GConfig->EmptySection(*Section, FullFileName);
}


bool UConfigFunctionLibrary::EmptySectionsMatchingString(const FString& SectionString, const bool bUseGeneratedConfigDir, const FString& FileName)
{
	if (!GConfig)	return false;
	FString FullFileName = ConvertRelativePathToFull(bUseGeneratedConfigDir, FileName);
	bool bSuccessful = GConfig->EmptySectionsMatchingString(*SectionString, FullFileName);
	//Sometimes the config file wonn't save changes if you don't call this function after you've set all your config keys.
	GConfig->Flush(false, FullFileName);
	return bSuccessful;
}


FString UConfigFunctionLibrary::GetDefaultSectionName(UObject* Object)
{
	if (!Object) { return FString(); }
	// Check object whether a class type
	FString ClassName = Object->GetClass()->GetName();
	if ((FCString::Stricmp(*ClassName, TEXT("Class")) == 0) || FCString::Stricmp(*ClassName, TEXT("Blueprint")) == 0)
	{
		return Object->GetPathName();
	}
	else
	{
		return Object->GetClass()->GetPathName();
	}
}
