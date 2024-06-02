// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_TestThreadExecOnce.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "GraphEditorSettings.h"

void UK2Node_TestThreadExecOnce::AllocateDefaultPins()
{
	CreatePin(EGPD_Input,UEdGraphSchema_K2::PC_Exec,UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output,UEdGraphSchema_K2::PC_Exec,UEdGraphSchema_K2::PN_Then);
}

FSlateIcon UK2Node_TestThreadExecOnce::GetIconAndTint(FLinearColor& OutColor) const
{
	static const FSlateIcon Icon=FSlateIcon("EditorStyle","GraphEditor.Macro");
	return Icon;
}

FLinearColor UK2Node_TestThreadExecOnce::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
}

void UK2Node_TestThreadExecOnce::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
}

// void UK2Node_TestThreadExecOnce::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
// {
// 	// Super::GetMenuActions(ActionRegistrar);
// 	if(auto ActionKey=GetClass())
// 	{
// 		auto NodeSpawner=UBlueprintNodeSpawner::Create(ActionKey);
// 		check(NodeSpawner);
// 		ActionRegistrar.AddBlueprintAction(NodeSpawner);
// 	}
// }
