// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConfigFunctionLibrary.generated.h"

/**
 *
 */
UCLASS()
class BLUEPRINTPRO_API UConfigFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:// Set & Get value

	/**
		* Save value to ini config file
		*
		* @param Section	Section of the ini file to load from.
		* @param Key		The key in the section of the ini file to load.
		* @param bUseGeneratedConfigDir		If true, use engine saves generated config directory files, otherwise use root configuration directory files.
		* @param FileName	File name to use to find the section, default value: DefaultGame.ini and stored in ProjectDir()/Config/DefaultGame.ini
		* @param Value		Save value of the .ini file.
		* @param bSuccessful		True on success, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Set Value", CustomStructureParam = "Value", AutoCreateRefTerm = "Value", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "FileName,bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static void SetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, const int32& Value, bool& bSuccessful);
	static void Generic_SetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, FProperty* InProperty, void* InPropertyData, bool& bSuccessful);

	DECLARE_FUNCTION(execSetConfigPorpertyByName)
	{
		P_GET_PROPERTY(FStrProperty, Section);
		P_GET_PROPERTY(FStrProperty, Key);
		P_GET_UBOOL(bUseGeneratedConfigDir);
		P_GET_PROPERTY(FStrProperty, FileName);

		Stack.StepCompiledIn<FStructProperty>(NULL);
		void* ValueAddr = Stack.MostRecentPropertyAddress;
		FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
		if (!ValueProp)
		{
			return;
		}
		P_GET_UBOOL_REF(bSuccessful);

		P_FINISH;

		P_NATIVE_BEGIN;
		Generic_SetConfigPorpertyByName(Section, Key, bUseGeneratedConfigDir, FileName, ValueProp, ValueAddr, bSuccessful);
		P_NATIVE_END;
	}


	/**
	* Load value from ini config file
	*
	* @param Section	Section of the ini file to load from.
	* @param Key		The key in the section of the ini file to load.
	* @param bUseGeneratedConfigDir		If true, use engine saves generated config directory files, otherwise use root configuration directory files.
	* @param FileName	File name to use to find the section, default value: DefaultGame.ini and stored in ProjectDir()/Config/DefaultGame.ini
	* @param bSuccessful		True on success, false otherwise.
	* @param Value		Value of the .ini file.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Get Value", CustomStructureParam = "Value", AutoCreateRefTerm = "Value", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "FileName,bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static void GetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, bool& bSuccessful, int32& Value);
	static void Generic_GetConfigPorpertyByName(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, bool& bSuccessful, FProperty* OutProperty, void* OutPropertyData);

	DECLARE_FUNCTION(execGetConfigPorpertyByName)
	{
		P_GET_PROPERTY(FStrProperty, Section);
		P_GET_PROPERTY(FStrProperty, Key);
		P_GET_UBOOL(bUseGeneratedConfigDir);
		P_GET_PROPERTY(FStrProperty, FileName);
		P_GET_UBOOL_REF(bSuccessful);

		Stack.StepCompiledIn<FStructProperty>(NULL);
		void* ValueAddr = Stack.MostRecentPropertyAddress;
		FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
		if (!ValueProp)
		{
			return;
		}

		P_FINISH;

		P_NATIVE_BEGIN;
		Generic_GetConfigPorpertyByName(Section, Key, bUseGeneratedConfigDir, FileName, bSuccessful, ValueProp, ValueAddr);
		P_NATIVE_END;
	}


	/**
	* Save string array to ini config file
	*
	* @param Section	Section of the ini file to load from.
	* @param Key		The key in the section of the ini file to load.
	* @param bUseGeneratedConfigDir		If true, use engine saves generated config directory files, otherwise use root configuration directory files.
	* @param FileName	File name to use to find the section, default value: DefaultGame.ini and stored in ProjectDir()/Config/DefaultGame.ini
	* @param Value		Save the string array to .ini file.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set String Array", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "FileName,bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static void SetStringArray(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, const TArray<FString>& Value);

	/**
	* Load string array from ini config file
	*
	* @param Section	Section of the ini file to load from.
	* @param Key		The key in the section of the ini file to load.
	* @param bUseGeneratedConfigDir		If true, use engine saves generated config directory files, otherwise use root configuration directory files.
	* @param FileName	File name to use to find the section, default value: DefaultGame.ini and stored in ProjectDir()/Config/DefaultGame.ini
	* @param Value		String array to load into
	*
	* @return The length of out string array
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get String Array", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "FileName,bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static int32 GetStringArray(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName, TArray<FString>& Value);

public:	// GetProperty Name

	/** Returns a human readable string that was assigned to this field at creation. */
	UFUNCTION(BlueprintPure, CustomThunk, meta = (DisplayName = "Get Property Name", CustomStructureParam = "Property"), Category = "BlueprintPro|Config")
		static FString GetPropertyName(const int32& Property);

	DECLARE_FUNCTION(execGetPropertyName)
	{
		Stack.StepCompiledIn<FStructProperty>(NULL);
		FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
		if (!ValueProp)
		{
			return;
		}
		P_FINISH;

		P_NATIVE_BEGIN;
		*(FString*)RESULT_PARAM = ValueProp->GetAuthoredName();
		P_NATIVE_END;
	}

public: //Config section & Key

	/**
	 * Retrieve a list of all of the config files stored in the cache
	 *
	 * @param ConfigFileNames Out array to receive the list of file names
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Config File Names"), Category = "BlueprintPro|Config")
		static void GetConfigFileNames(TArray<FString>& ConfigFileNames);

	/**
	 * Retrieve the names for all sections contained in the file specified by file name
	 *
	 * @param bUseGeneratedConfigDir	If true, use engine saves generated config directory files, otherwise use root configuration directory files.
	 * @param FileName		The file to retrieve section names from
	 * @param SectionNames	It will receive the list of section names
	 *
	 * @return True if the file specified was successfully found;
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Section Names", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static bool GetSectionNames(const bool bUseGeneratedConfigDir, const FString& FileName, TArray<FString>& SectionNames);

	/** Utility function for looking up section from config file. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Does Section Exist", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static bool DoesSectionExist(const FString& Section, const bool bUseGeneratedConfigDir, const FString& FileName);

	/** Utility function to remove key of section by name.*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Key", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static bool RemoveKey(const FString& Section, const FString& Key, const bool bUseGeneratedConfigDir, const FString& FileName);

	/** Utility function to remove section of config file. */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Section", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static bool EmptySection(const FString& Section, const bool bUseGeneratedConfigDir, const FString& FileName);

	/** Utility function to empty all sections which's section name contains section string. */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Empty Sections Matching String", FileName = "DefaultProject.ini", bUseGeneratedConfigDir = false, AdvancedDisplay = "bUseGeneratedConfigDir"), Category = "BlueprintPro|Config")
		static bool EmptySectionsMatchingString(const FString& SectionString, const bool bUseGeneratedConfigDir, const FString& FileName);

	/** Utility function to get object default section name. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Object Default Section", DefaultToSelf = "Object", AdvancedDisplay = "Object"), Category = "BlueprintPro|Config")
		static FString GetDefaultSectionName(UObject* Object);

};
