// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5EditorEditorMode.h"
#include "UE5EditorEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "UE5EditorEditorModeCommands.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/UE5EditorSimpleTool.h"
#include "Tools/UE5EditorInteractiveTool.h"

// step 2: register a ToolBuilder in FUE5EditorEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "UE5EditorEditorMode"

const FEditorModeID UUE5EditorEditorMode::EM_UE5EditorEditorModeId = TEXT("EM_UE5EditorEditorMode");

FString UUE5EditorEditorMode::SimpleToolName = TEXT("UE5Editor_ActorInfoTool");
FString UUE5EditorEditorMode::InteractiveToolName = TEXT("UE5Editor_MeasureDistanceTool");


UUE5EditorEditorMode::UUE5EditorEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UUE5EditorEditorMode::EM_UE5EditorEditorModeId,
		LOCTEXT("ModeName", "UE5Editor"),
		FSlateIcon(),
		true);
}


UUE5EditorEditorMode::~UUE5EditorEditorMode()
{
}


void UUE5EditorEditorMode::ActorSelectionChangeNotify()
{
}

void UUE5EditorEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FUE5EditorEditorModeCommands& SampleToolCommands = FUE5EditorEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UUE5EditorSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UUE5EditorInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UUE5EditorEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FUE5EditorEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UUE5EditorEditorMode::GetModeCommands() const
{
	return FUE5EditorEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
