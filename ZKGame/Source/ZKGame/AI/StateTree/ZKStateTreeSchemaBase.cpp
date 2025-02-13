// Fill out your copyright notice in the Description page of Project Settings.

#include "ZKStateTreeSchemaBase.h"
#include "StateTreeConditionBase.h"
#include "StateTreeConsiderationBase.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeTaskBase.h"


UZKStateTreeSchemaBase::UZKStateTreeSchemaBase()
{
	ContextDataDescs.Append({
			// EvaluationContextOwner: {A474F4B3-A82F-45C2-9405-3535F711751B}
			FStateTreeExternalDataDesc(
					TEXT("ContextOwner"),
					UObject::StaticClass(), 
					FGuid(0xA474F4B3, 0xA82F45C2, 0x94053535, 0xF711751B))
			});
}

bool UZKStateTreeSchemaBase::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
	return
		// Common structs.
		InScriptStruct->IsChildOf(FStateTreeConditionCommonBase::StaticStruct())
		|| InScriptStruct->IsChildOf(FStateTreeConsiderationCommonBase::StaticStruct())
		|| InScriptStruct->IsChildOf(FStateTreeEvaluatorCommonBase::StaticStruct())
		|| InScriptStruct->IsChildOf(FStateTreeTaskCommonBase::StaticStruct())
		// GameplayCameras structs.
		;
}

bool UZKStateTreeSchemaBase::IsClassAllowed(const UClass* InClass) const
{
	return IsChildOfBlueprintBase(InClass);
}

bool UZKStateTreeSchemaBase::IsExternalItemAllowed(const UStruct& InStruct) const
{
	return true;
}

