// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_Test.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "UETest/Components/UKHomingComponent.h"

UK2Node_Test::UK2Node_Test()
{
}

void UK2Node_Test::AllocateDefaultPins()
{
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    
    // 기본 핀 생성 (예: Execute 및 Then 핀)
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, FName("Execute"));
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, FName("Then"));

    // Add Target Object pin
    UEdGraphPin* ObjectPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UUKHomingComponent::StaticClass(), FName("UKHomingComponent"));
    
    // 델리게이트 핀 생성
    CreateDelegatePins();
}

void UK2Node_Test::CreateDelegatePins()
{
    // 델리게이트 속성을 가져옵니다 (예: 이 예제에서는 임의의 속성 이름 사용)
    UClass* MyClass = UUKEventTask::StaticClass();
    for (TFieldIterator<FMulticastDelegateProperty> PropIt(MyClass); PropIt; ++PropIt)
    {
        FMulticastDelegateProperty* DelegateProp = *PropIt;
        FString PinName = DelegateProp->GetName();
        CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, *PinName);
    }
}

FText UK2Node_Test::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(TEXT("HomingStart"));
}

FText UK2Node_Test::GetTooltipText() const
{
    return FText::FromString(TEXT("This node dynamically creates pins based on the number of multicast delegates."));
}

FLinearColor UK2Node_Test::GetNodeTitleColor() const
{
    return FLinearColor::Green;
}

void UK2Node_Test::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    Super::GetMenuActions(ActionRegistrar);

    // actions get registered under specific object-keys; the idea is that 
    // actions might have to be updated (or deleted) if their object-key is  
    // mutated (or removed)... here we use the node's class (so if the node 
    // type disappears, then the action should go with it)
    UClass* ActionKey = GetClass();
    // to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
    // check to make sure that the registrar is looking for actions of this type
    // (could be regenerating actions for a specific asset, and therefore the 
    // registrar would only accept actions corresponding to that asset)
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

FText UK2Node_Test::GetMenuCategory() const
{
    return FText::FromString(TEXT("UK Homing"));
}

void UK2Node_Test::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
    
    // 노드 확장 로직 구현 (필요한 경우)

    // Find the input pin for the UKHomingComponent
    UEdGraphPin* HomingComponentPin = FindPinChecked(TEXT("UKHomingComponent"));
    
    // Check if the pin is valid
    if (!HomingComponentPin || HomingComponentPin->LinkedTo.Num() == 0)
    {
        return;
    }

    // Get the connected pin
    UEdGraphPin* ConnectedPin = HomingComponentPin->LinkedTo[0];
    // Create a call function node for each delegate
    UClass* MyClass = UUKEventTask::StaticClass();
    for (TFieldIterator<FMulticastDelegateProperty> PropIt(MyClass); PropIt; ++PropIt)
    {
        FMulticastDelegateProperty* DelegateProp = *PropIt;
        FString PinName = DelegateProp->GetName();

        // // Create a call function node
        // UK2Node_CallFunction* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        // CallFunctionNode->FunctionReference.SetExternalMember(DelegateProp->GetFName(), UUKHomingComponent::StaticClass());
        // // CallFunctionNode->FunctionReference.SetExternalMember(DelegateProp->GetFName(), HomingComponentPin->PinType.PinSubCategoryObject.Get());
        // CallFunctionNode->AllocateDefaultPins();
        //
        // // Connect the pins
        // UEdGraphPin* ExecPin = CallFunctionNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
        // UEdGraphPin* ThenPin = FindPinChecked(PinName);
        // CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then), *ThenPin);
        //
        // // Connect the UKHomingComponent pin
        // UEdGraphPin* ObjectPin = CallFunctionNode->FindPinChecked(TEXT("UKHomingComponent"));
        // CompilerContext.MovePinLinksToIntermediate(*HomingComponentPin, *ObjectPin);
    }
}
