// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "AssetTypeActions_HTN.h"

#include "HTN.h"
#include "HTNColors.h"
#include "HTNEditorModule.h"
#include "SHTNDiff.h"

#include "AIModule.h"
#include "IAssetTools.h"
#include "Misc/EngineVersionComparison.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_HTN"

FText FAssetTypeActions_HTN::GetName() const
{
	return LOCTEXT("HTN", "Hierarchical Task Network");
}

FColor FAssetTypeActions_HTN::GetTypeColor() const
{
	return HTNColors::Asset.ToFColor(/*bSRGB=*/true);
}

UClass* FAssetTypeActions_HTN::GetSupportedClass() const
{
	return UHTN::StaticClass();
}

uint32 FAssetTypeActions_HTN::GetCategories()
{
	// GetAIAssetCategoryBit was deprecated in 5.3.
	// We should migrate from FAssetTypeActions to the newer AssetDefinition system.
	// That will only be possible cleanly once we drop support for UE5.1 (so when 5.4 is out) because UAssetDefinition was added in 5.2.
#if UE_VERSION_OLDER_THAN(5, 3, 0)
	return IAIModule::Get().GetAIAssetCategoryBit();
#else
	return IAssetTools::Get().FindAdvancedAssetCategory(TEXT("AI"));
#endif
}

void FAssetTypeActions_HTN::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	IHTNEditorModule& EditorModule = IHTNEditorModule::Get();
	
	for (UObject* const Obj : InObjects)
	{
		if (UHTN* const HTN = Cast<UHTN>(Obj))
		{
			EditorModule.CreateHTNEditor(Mode, EditWithinLevelEditor, HTN);
		}
	}
}

void FAssetTypeActions_HTN::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const
{
	UHTN* const OldHTN = Cast<UHTN>(OldAsset);
	UHTN* const NewHTN = Cast<UHTN>(NewAsset);
	check(NewHTN || OldHTN);

	// Sometimes we're comparing different revisions of one single asset 
	// (other times we're comparing two completely separate assets altogether)
	// if we're diffing one asset against itself, put its name in the window's title.
	const bool bIsSingleAsset = !OldHTN || !NewHTN || OldHTN->GetName() == NewHTN->GetName();
	const FText WindowTitle = bIsSingleAsset ? 
		FText::Format(LOCTEXT("HTNDiff", "{0} - HTN Diff"), FText::FromString(NewHTN->GetName())) : 
		LOCTEXT("NamelessHTNDiff", "HTN Diff");

	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(FVector2D(1000, 800));

	Window->SetContent(SNew(SHTNDiff)
		.HTNOld(OldHTN)
		.HTNNew(NewHTN)
		.OldRevision(OldRevision)
		.NewRevision(NewRevision)
		.ShowAssetNames(!bIsSingleAsset)
		.OpenInDefaultDiffTool(this, &FAssetTypeActions_HTN::OpenInDefaultDiffTool));

	// Make this window a child of the modal window if we've been spawned while one is active.
	if (const TSharedPtr<SWindow> ActiveModal = FSlateApplication::Get().GetActiveModalWindow())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window, ActiveModal.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}
}

void FAssetTypeActions_HTN::OpenInDefaultDiffTool(UHTN* OldHTN, UHTN* NewHTN) const
{
	const FString OldTextFilename = DumpAssetToTempFile(OldHTN);
	const FString NewTextFilename = DumpAssetToTempFile(NewHTN);

	// Get diff program to use
	const FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateDiffProcess(DiffCommand, OldTextFilename, NewTextFilename);
}

#undef LOCTEXT_NAMESPACE
