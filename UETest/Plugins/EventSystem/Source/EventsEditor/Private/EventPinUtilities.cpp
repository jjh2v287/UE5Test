// Copyright 2019 - 2021, butterfly, Event System Plugin, All Rights Reserved.

#include "EventPinUtilities.h"
#include "EventsManager.h"
#include "SGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableSet.h"
#include "K2Node_FunctionTerminator.h"

static FName NAME_Categories = FName("Categories");

FString EventPinUtilities::ExtractTagFilterStringFromGraphPin(UEdGraphPin* InTagPin)
{
	FString FilterString;

	if (ensure(InTagPin))
	{
		const UEventsManager& TagManager = UEventsManager::Get();
		if (UScriptStruct* PinStructType = Cast<UScriptStruct>(InTagPin->PinType.PinSubCategoryObject.Get()))
		{
			FilterString = TagManager.GetCategoriesMetaFromField(PinStructType);
		}

		UEdGraphNode* OwningNode = InTagPin->GetOwningNode();

		if (FilterString.IsEmpty())
		{
			FilterString = OwningNode->GetPinMetaData(InTagPin->PinName, NAME_Categories);
		}

		if (FilterString.IsEmpty())
		{
			if (const UK2Node_CallFunction* CallFuncNode = Cast<UK2Node_CallFunction>(OwningNode))
			{
				if (const UFunction* TargetFunction = CallFuncNode->GetTargetFunction())
				{
					FilterString = TagManager.GetCategoriesMetaFromFunction(TargetFunction, InTagPin->PinName);
				}
			}
			else if (const UK2Node_VariableSet* VariableSetNode = Cast<UK2Node_VariableSet>(OwningNode))
			{
				FilterString = TagManager.GetCategoriesMetaFromField(VariableSetNode->GetPropertyForVariable());
			}
			else if (const UK2Node_FunctionTerminator* FuncTermNode = Cast<UK2Node_FunctionTerminator>(OwningNode))
			{
				if (const UFunction* SignatureFunction = FuncTermNode->FindSignatureFunction())
				{
					FilterString = TagManager.GetCategoriesMetaFromFunction(SignatureFunction, InTagPin->PinName);
				}
			}
		}
	}

	return FilterString;
}
