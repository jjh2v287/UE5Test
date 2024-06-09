// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#include "ESBPLibrary.h"
#include "EdGraphSchema_K2.h"
#include "JsonObjectConverter.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/GameInstance.h"

extern ENGINE_API FString GetPathPostfix(const UObject* ForObject);

UESBPLibrary::UESBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

FString UESBPLibrary::GetParameterType(const FEdGraphPinType& Type)
{
	FString SyncStatusJsonString;
	const bool bJsonStringOk = FJsonObjectConverter::UStructToJsonObjectString(Type, SyncStatusJsonString);
	return SyncStatusJsonString;
}

FString UESBPLibrary::GetCppName(FFieldVariant Field, bool bUInterface /*= false*/, bool bForceParameterNameModification /*= false*/)
{
	check(Field);
	const UClass* AsClass = Field.Get<UClass>();
	const UScriptStruct* AsScriptStruct = Field.Get<UScriptStruct>();
	if (AsClass || AsScriptStruct)
	{
		if (AsClass && AsClass->HasAnyClassFlags(CLASS_Interface))
		{
			ensure(AsClass->IsChildOf<UInterface>());
			return FString::Printf(TEXT("%s%s")
				, bUInterface ? TEXT("U") : TEXT("I")
				, *AsClass->GetName());
		}
		const UStruct* AsStruct = Field.Get<UStruct>();
		check(AsStruct);
		if (AsStruct->IsNative())
		{
			return FString::Printf(TEXT("%s%s"), AsStruct->GetPrefixCPP(), *AsStruct->GetName());
		}
		else
		{
			return ::UnicodeToCPPIdentifier(*AsStruct->GetName(), false, AsStruct->GetPrefixCPP()) + GetPathPostfix(AsStruct);
		}
	}
	else if (const FProperty* AsProperty = Field.Get<FProperty>())
	{
		const UStruct* Owner = AsProperty->GetOwnerStruct();
		const bool bModifyName = ensure(Owner)
			&& (Cast<UBlueprintGeneratedClass>(Owner) || !Owner->IsNative() || bForceParameterNameModification);
		if (bModifyName)
		{
			FString VarPrefix;

			const bool bIsUberGraphVariable = Owner->IsA<UBlueprintGeneratedClass>() && AsProperty->HasAllPropertyFlags(CPF_Transient | CPF_DuplicateTransient);
			const bool bIsParameter = AsProperty->HasAnyPropertyFlags(CPF_Parm);
			const bool bFunctionLocalVariable = Owner->IsA<UFunction>();
			if (bIsUberGraphVariable)
			{
				int32 InheritenceLevel = GetInheritenceLevel(Owner);
				VarPrefix = FString::Printf(TEXT("b%dl__"), InheritenceLevel);
			}
			else if (bIsParameter)
			{
				VarPrefix = TEXT("bpp__");
			}
			else if (bFunctionLocalVariable)
			{
				VarPrefix = TEXT("bpfv__");
			}
			else
			{
				VarPrefix = TEXT("bpv__");
			}
			return ::UnicodeToCPPIdentifier(AsProperty->GetName(), AsProperty->HasAnyPropertyFlags(CPF_Deprecated), *VarPrefix);
		}
		return AsProperty->GetNameCPP();
	}

	if (Field.IsA<UUserDefinedEnum>())
	{
		return ::UnicodeToCPPIdentifier(Field.GetName(), false, TEXT("E__"));
	}

	if (!Field.IsNative())
	{
		return ::UnicodeToCPPIdentifier(Field.GetName(), false, TEXT("bpf__"));
	}
	return Field.GetName();
}

int32 UESBPLibrary::GetInheritenceLevel(const UStruct* Struct)
{
	const UStruct* StructIt = Struct ? Struct->GetSuperStruct() : nullptr;
	int32 InheritenceLevel = 0;
	while ((StructIt != nullptr) && !StructIt->IsNative())
	{
		++InheritenceLevel;
		StructIt = StructIt->GetSuperStruct();
	}
	return InheritenceLevel;
}

bool UESBPLibrary::GetPinTypeFromStr(const FString& PinTypeStr, FEdGraphPinType& PinType)
{
	if (FJsonObjectConverter::JsonObjectStringToUStruct(PinTypeStr, &PinType, 0, 0))
	{
		return true;
	}
	return false;
}

