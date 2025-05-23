// BlueprintPro Plugin - Copyright (c) 2023 XUSJTUER. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_GetUObjectPropertyByName.generated.h"


class UEdGraphPin;
class UK2Node_CallFunction;
class UEdGraph;

/**
 * K2Node for getting Variant property value of UObject by property name.
 */
UCLASS()
class BLUEPRINTPROEDITOR_API UK2Node_GetUObjectPropertyByName : public UK2Node
{
	GENERATED_UCLASS_BODY()
public:

	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual FText GetTooltipText() const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PostReconstructNode() override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetKeywords() const override;
	//virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End UK2Node Interface

	/** Get the then output pin */
	UEdGraphPin* GetThenPin() const { return Pins[1]; }
	/** Get the Failed output pin */
	UEdGraphPin* GetFailedPin() const { return Pins[2]; }
	/** Get the Data Table input pin */
	UEdGraphPin* GetObjectPin() const { return Pins[3]; }
	/** Get the spawn transform input pin */
	UEdGraphPin* GetPropertyNamePin() const { return Pins[4]; }
	/** Get the result output pin */
	UEdGraphPin* GetValuePin() const { return Pins[5]; }
	/** Get K2Node Call Function */
	UK2Node_CallFunction* GetK2NodeDstCallFunction(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

private:
	/**
	 * Takes the specified "MutatablePin" and sets its 'PinToolTip' field (according
	 * to the specified description)
	 *
	 * @param   MutatablePin	The pin you want to set tool-tip text on
	 * @param   PinDescription	A string describing the pin's purpose
	 */
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const;

	/** Propagates Value pin type to the between the input and output pins */
	void PropagateValuePinType();

	/** Tooltip text for this node. */
	FText NodeTooltip;
};
