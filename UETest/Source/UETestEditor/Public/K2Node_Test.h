// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_Test.generated.h"

class FBlueprintActionDatabaseRegistrar;

/**
 * 
 */
UCLASS()
class UETESTEDITOR_API UK2Node_Test : public UK2Node
{
	GENERATED_BODY()
	public:
        UK2Node_Test();
    
        virtual void AllocateDefaultPins() override;
        virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
        virtual FText GetTooltipText() const override;
        virtual FLinearColor GetNodeTitleColor() const override;
		virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
		virtual FText GetMenuCategory() const override;
        virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    
    protected:
        void CreateDelegatePins();
};
