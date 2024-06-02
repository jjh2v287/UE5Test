// (c) 2021 Sergio Lena. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "K2Node_CallFunction.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_CustomEvent.h"

#include "K2Node_RunAction.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONSED_API UK2Node_RunAction : public UK2Node_ConstructObjectFromClass
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	UEdGraphPin* GenerateAssignmentNodes(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UK2Node_CallFunction* CallBeginSpawnNode, UEdGraphNode* SpawnNode,
	                                     UEdGraphPin* CallBeginResult,
	                                     const UClass* ForClass);
	bool CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema);
	bool CreateDelegateForNewFunction(UEdGraphPin* DelegateInputPin, FName FunctionName, UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext);
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual void CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins = nullptr);
	
	//~ End UEdGraphNode Interface.

	FName GetDelegateTargetEntryPointName(FName DelegatePropertyName) const
	{
		const FString TargetName = GetName() + TEXT("_") + DelegatePropertyName.ToString() + TEXT("_EP");
		return FName(*TargetName);
	}
	
	//~ Begin UK2Node Interface
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	//~ End UK2Node Interface

	//~ Begin UK2Node_ConstructObjectFromClass Interface
	virtual UClass* GetClassPinBaseClass() const override;
	virtual bool IsSpawnVarPin(UEdGraphPin* Pin) const override;

};
