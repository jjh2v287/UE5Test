// (c) 2021 Sergio Lena. All Rights Reserved.

#include "K2Node_RunAction.h"
#include "AIController.h"
#include "Action.h"
#include "ActionsStatics.h"
#include "BlueprintCompilationManager.h"
#include "BlueprintDelegateNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "KismetCompiler.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_EnumLiteral.h"

#define LOCTEXT_NAMESPACE "K2Node_RunAction"



struct FK2Node_RunActionHelper
{
	static const FName ControllerPinName;
};

const FName FK2Node_RunActionHelper::ControllerPinName(TEXT("AIController"));

void UK2Node_RunAction::AllocateDefaultPins()
{
	// Add execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	UEdGraphPin* OutputPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	OutputPin->SafeSetHidden(true);
	
	// If required add the world context pin
	if (UseWorldContext())
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), "World");
	}

	// Add blueprint pin
	UEdGraphPin* ClassPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, GetClassPinBaseClass(), "Class");
	
	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, GetClassPinBaseClass(), UEdGraphSchema_K2::PN_ReturnValue);
	
	if (UseOuter())
	{
		UEdGraphPin* OuterPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), "Outer");
	}

}

FText UK2Node_RunAction::GetNodeTitle(ENodeTitleType::Type Title) const
{
	UEdGraphPin* ClassPin = GetClassPin();
	if (!ClassPin || !IsValid(ClassPin->DefaultObject) || ClassPin->LinkedTo.Num())
	{
		return LOCTEXT("TitleUnknown", "Run Action");
	}

	if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Action"), FText::FromString(ClassPin->DefaultObject->GetName().Replace(*FString("_C"),*FString(""))));
		CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("Run Action", "Execute {Action}"), Args), this);
	}
	return CachedNodeTitle;
}

void UK2Node_RunAction::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{

	return Super::GetPinHoverText(Pin, HoverTextOut);
}

void UK2Node_RunAction::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static FName RunActionFunctionName = GET_FUNCTION_NAME_CHECKED(UActionsStatics, RunAction);
	static FName FunctionParam_ActionClass = TEXT("Action");
	static FName FunctionParam_Instigator = TEXT("Instigator");

	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ActionClassPin = GetClassPin();
	UEdGraphPin* ThenPin = GetThenPin();
	UEdGraphPin* ResultPin = GetResultPin();

	UClass* SpawnClass = ActionClassPin ? Cast<UClass>(ActionClassPin->DefaultObject) : nullptr;
	if (!ActionClassPin->LinkedTo.Num() && !SpawnClass)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("RunActionExpandingWithoutClass", "Spawn node @@ must have a class specified.").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	// Here we adapt/bind our pins to the static function pins that we are calling.
	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(RunActionFunctionName, StaticClass());
	CallCreateNode->AllocateDefaultPins();

	//allocate nodes for created widget.
	UEdGraphPin* CallCreate_ExecPin = CallCreateNode->GetExecPin();
	UEdGraphPin* CallCreate_ActionClass = CallCreateNode->FindPinChecked(FunctionParam_ActionClass);
	UEdGraphPin* CallCreate_Result = CallCreateNode->GetReturnValuePin();

	if (ActionClassPin->LinkedTo.Num())
	{
		CompilerContext.MovePinLinksToIntermediate(*ActionClassPin, *CallCreate_ActionClass);
	}
	else
	{
		CallCreate_ActionClass->DefaultObject = SpawnClass;
	}

	CallCreate_Result->PinType = ResultPin->PinType;

	//CompilerContext.MovePinLinksToIntermediate(*InstigatorPin, *CallCreate_Instigator);
	CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CallCreate_Result);
	CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallCreate_ExecPin);

	// Now we make the assignment pins for all exposed class variables.
	UEdGraphPin* LastThen = GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, this, CallCreate_Result, GetClassToSpawn());
	
	CompilerContext.MovePinLinksToIntermediate(*ThenPin, *LastThen);

	// Finally, break stuff that was connected.
	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_RunAction::GenerateAssignmentNodes(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UK2Node_CallFunction* CallBeginSpawnNode, UEdGraphNode* SpawnNode, UEdGraphPin* CallBeginResult, const UClass* ForClass )
{
	static const FName ObjectParamName(TEXT("Object"));
	static const FName ValueParamName(TEXT("Value"));
	static const FName PropertyNameParamName(TEXT("PropertyName"));
	static const FName DelegateNameParamName(TEXT("OutputDelegate"));
	static const FName EventNameParamName(TEXT("Delegate"));
	
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	UEdGraphPin* LastThen = CallBeginSpawnNode->GetThenPin();

	// Create 'set var by name' nodes and hook them up
	for (int32 PinIdx = 0; PinIdx < SpawnNode->Pins.Num(); PinIdx++)
	{
		// Only create 'set param by name' node if this pin is linked to something
		UEdGraphPin* OrgPin = SpawnNode->Pins[PinIdx];
		const bool bHasDefaultValue = !OrgPin->DefaultValue.IsEmpty() || !OrgPin->DefaultTextValue.IsEmpty() || OrgPin->DefaultObject;
		if (NULL == CallBeginSpawnNode->FindPin(OrgPin->PinName) &&
			(OrgPin->LinkedTo.Num() > 0 || bHasDefaultValue))
		{
			if( OrgPin->LinkedTo.Num() == 0 )
			{
				FProperty* Property = FindFProperty<FProperty>(ForClass, OrgPin->PinName);
				// NULL property indicates that this pin was part of the original node, not the 
				// class we're assigning to:
				if( !Property )
				{
					continue;
				}

				// We don't want to generate an assignment node unless the default value 
				// differs from the value in the CDO:
				FString DefaultValueAsString;
					
				if (FBlueprintCompilationManager::GetDefaultValue(ForClass, Property, DefaultValueAsString))
				{
					if (Schema->DoesDefaultValueMatch(*OrgPin, DefaultValueAsString) && !Property->IsA(FMulticastDelegateProperty::StaticClass()))
					{
						continue;
					}
				}
				else if(ForClass->ClassDefaultObject && !Property->IsA(FMulticastDelegateProperty::StaticClass()))
				{
					FBlueprintEditorUtils::PropertyValueToString(Property, (uint8*)ForClass->ClassDefaultObject, DefaultValueAsString);

					if (DefaultValueAsString == OrgPin->GetDefaultAsString())
					{
						continue;
					}
				}
			}
			FProperty* Property = FindFProperty<FProperty>(ForClass, OrgPin->PinName);
			if(!Property->IsA(FMulticastDelegateProperty::StaticClass()))
			{
				UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(OrgPin->PinType);
				if (SetByNameFunction)
				{
					UK2Node_CallFunction* SetVarNode = nullptr;
					if (OrgPin->PinType.IsArray())
					{
						SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(SpawnNode, SourceGraph);
					}
					else
					{
						SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
					}
					SetVarNode->SetFromFunction(SetByNameFunction);
					SetVarNode->AllocateDefaultPins();

					// Connect this node into the exec chain
					Schema->TryCreateConnection(LastThen, SetVarNode->GetExecPin());
					LastThen = SetVarNode->GetThenPin();

					// Connect the new actor to the 'object' pin
					UEdGraphPin* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
					CallBeginResult->MakeLinkTo(ObjectPin);

					// Fill in literal for 'property name' pin - name of pin is property name
					UEdGraphPin* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
					PropertyNamePin->DefaultValue = OrgPin->PinName.ToString();

					UEdGraphPin* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
					if (OrgPin->LinkedTo.Num() == 0 &&
						OrgPin->DefaultValue != FString() &&
						OrgPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte &&
						OrgPin->PinType.PinSubCategoryObject.IsValid() &&
						OrgPin->PinType.PinSubCategoryObject->IsA<UEnum>())
					{
						// Pin is an enum, we need to alias the enum value to an int:
						UK2Node_EnumLiteral* EnumLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_EnumLiteral>(SpawnNode, SourceGraph);
						EnumLiteralNode->Enum = CastChecked<UEnum>(OrgPin->PinType.PinSubCategoryObject.Get());
						EnumLiteralNode->AllocateDefaultPins();
						EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ValuePin);

						UEdGraphPin* InPin = EnumLiteralNode->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
						check( InPin );
						InPin->DefaultValue = OrgPin->DefaultValue;
					}
					else
					{
						// For non-array struct pins that are not linked, transfer the pin type so that the node will expand an auto-ref that will assign the value by-ref.
						if (OrgPin->PinType.IsArray() == false && OrgPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && OrgPin->LinkedTo.Num() == 0)
						{
							ValuePin->PinType.PinCategory = OrgPin->PinType.PinCategory;
							ValuePin->PinType.PinSubCategory = OrgPin->PinType.PinSubCategory;
							ValuePin->PinType.PinSubCategoryObject = OrgPin->PinType.PinSubCategoryObject;
							CompilerContext.MovePinLinksToIntermediate(*OrgPin, *ValuePin);
						}
						else
						{
							CompilerContext.MovePinLinksToIntermediate(*OrgPin, *ValuePin);
							SetVarNode->PinConnectionListChanged(ValuePin);
						}

					}
				}
			}
			else
			{
				if (UFunction* TargetFunction = CastField<FMulticastDelegateProperty>(Property)->SignatureFunction)
				{
					//First, create an event node matching the delegate signature
					// UK2Node_CustomEvent* DelegateEvent = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(this, OrgPin, SourceGraph);
					UK2Node_CustomEvent* DelegateEvent = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, SourceGraph);
					DelegateEvent->EventReference.SetFromField<UFunction>(TargetFunction, false);
					DelegateEvent->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *Property->GetName(), *CompilerContext.GetGuid(this));
					DelegateEvent->AllocateDefaultPins();
					
					UEdGraphPin* EventOutputPin = Schema->FindExecutionPin(*DelegateEvent, EGPD_Output);
					check(EventOutputPin != NULL);
					CompilerContext.MovePinLinksToIntermediate(*OrgPin, *EventOutputPin);
					
					UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(this, SourceGraph);
					AddDelegateNode->SetFromProperty(Property, false, Property->GetOwnerClass());
					AddDelegateNode->AllocateDefaultPins();
					
					Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), CallBeginResult);
					Schema->TryCreateConnection(LastThen, AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
					LastThen = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);
					
					UEdGraphPin* DelegateOutputPin = DelegateEvent->FindPinChecked(DelegateNameParamName);
					check(DelegateOutputPin != NULL);
					
					UEdGraphPin* EventInputPin = AddDelegateNode->FindPinChecked(EventNameParamName);
					check(EventInputPin != NULL);
					
					Schema->TryCreateConnection(EventInputPin, DelegateOutputPin);
				
				}
				
			}
		}
	}

	return LastThen;
}
bool UK2Node_RunAction::CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema)
{
	check(CENode && Function && Schema);

	bool bResult = true;
	for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		const FProperty* Param = *PropIt;
		if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
		{
			FEdGraphPinType PinType;
			bResult &= Schema->ConvertPropertyToPinType(Param, /*out*/ PinType);
			bResult &= (nullptr != CENode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output));
		}
	}
	return bResult;
}

FSlateIcon UK2Node_RunAction::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.54f, 0.16f, 0.88f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Event_16x");
	return Icon;
}

bool UK2Node_RunAction::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || (FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph && Blueprint->GeneratedClass->GetDefaultObject()->ImplementsGetWorld()));
}

FText UK2Node_RunAction::GetTooltipText() const
{
	return LOCTEXT("RunActionTooltip", "Attempts to run an Action.");
}

FText UK2Node_RunAction::GetMenuCategory() const
{
	return LOCTEXT("RunActionMenuCategory", "Action");
}

void UK2Node_RunAction::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins)
{
	check(InClass);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	const UObject* const ClassDefaultObject = InClass->GetDefaultObject(false);

	for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		const bool bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
		const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if(	!Property->HasAnyPropertyFlags(CPF_Parm) && 
			bIsSettableExternally &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
			!bIsDelegate &&
			(nullptr == FindPin(Property->GetFName()) ) &&
			FBlueprintEditorUtils::PropertyStillExists(Property))
		{
			if (UEdGraphPin* Pin = CreatePin(EGPD_Input, NAME_None, Property->GetFName()))
			{
				K2Schema->ConvertPropertyToPinType(Property, /*out*/ Pin->PinType);
				if (OutClassPins)
				{
					OutClassPins->Add(Pin);
				}

				if (ClassDefaultObject && K2Schema->PinDefaultValueIsEditable(*Pin))
				{
					FString DefaultValueAsString;
					const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString, this);
					check(bDefaultValueSet);
					K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
				}

				// Copy tooltip from the property.
				K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
			}
		}
		else if(bIsDelegate)
		{
			if(UEdGraphPin* DelegatePin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, Property->GetFName()))
			{
				K2Schema->ConstructBasicPinTooltip(*DelegatePin, Property->GetToolTipText(), DelegatePin->PinToolTip);
			}
		}
	}

	// Change class of output pin
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = InClass->GetAuthoritativeClass();
}

UClass* UK2Node_RunAction::GetClassPinBaseClass() const
{
	return UAction::StaticClass();
}

bool UK2Node_RunAction::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	const FName PinName = Pin->PinName;
	return Super::IsSpawnVarPin(Pin);
}



#undef LOCTEXT_NAMESPACE