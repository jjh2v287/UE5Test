// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5EditorEditorModeToolkit.h"
#include "UE5EditorEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "UE5EditorEditorModeToolkit"

FUE5EditorEditorModeToolkit::FUE5EditorEditorModeToolkit()
{
}

void FUE5EditorEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FUE5EditorEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FUE5EditorEditorModeToolkit::GetToolkitFName() const
{
	return FName("UE5EditorEditorMode");
}

FText FUE5EditorEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "UE5EditorEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
