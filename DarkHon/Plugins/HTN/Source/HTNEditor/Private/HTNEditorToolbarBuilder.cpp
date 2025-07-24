// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNEditorToolbarBuilder.h"

#include "HTNAppStyle.h"
#include "HTNCommands.h"
#include "HTNDebugger.h"
#include "HTNEditor.h"
#include "SHTNRevisionMenu.h"

#include "AssetToolsModule.h"
#include "IAssetTypeActions.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ISourceControlRevision.h"
#include "ISourceControlState.h"
#include "Misc/PackageName.h"
#include "SourceControlHelpers.h"
#include "ToolMenuDelegates.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SSpacer.h"
#include "WorkflowOrientedApp/SModeWidget.h"

#define LOCTEXT_NAMESPACE "HTNEditorToolbarBuilder"

namespace
{
	class SHTNModeSeparator : public SBorder
	{
	public:
		SLATE_BEGIN_ARGS(SHTNModeSeparator) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArg)
		{
			SBorder::Construct(
				SBorder::FArguments()
				.BorderImage(FHTNAppStyle::GetBrush("BlueprintEditor.PipelineSeparator"))
				.Padding(0.0f));
		}

		// SWidget interface
		virtual FVector2D ComputeDesiredSize(float) const override
		{
			const float Height = 20.0f;
			const float Thickness = 16.0f;
			return FVector2D(Thickness, Height);
		}
		// End of SWidget interface
	};
}

void FHTNEditorToolbarBuilder::AddAssetToolbar(TSharedPtr<FExtender> Extender)
{
	check(HTNEditorWeakPtr.IsValid());
	const TSharedPtr<FHTNEditor> HTNEditor = HTNEditorWeakPtr.Pin();

	const TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension(
		TEXT("Asset"),
		EExtensionHook::After,
		HTNEditor->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FHTNEditorToolbarBuilder::FillAssetToolbar));
	HTNEditor->AddToolbarExtender(ToolbarExtender);
}

void FHTNEditorToolbarBuilder::AddModesToolbar(TSharedPtr<FExtender> Extender)
{
	check(HTNEditorWeakPtr.IsValid());
	const TSharedPtr<FHTNEditor> HTNEditor = HTNEditorWeakPtr.Pin();

	Extender->AddToolBarExtension(
		TEXT("Asset"),
		EExtensionHook::After,
		HTNEditor->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FHTNEditorToolbarBuilder::FillModesToolbar));
}

void FHTNEditorToolbarBuilder::AddDebuggerToolbar(TSharedPtr<FExtender> Extender)
{
	check(HTNEditorWeakPtr.IsValid());
	const TSharedPtr<FHTNEditor> HTNEditor = HTNEditorWeakPtr.Pin();

	const TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension(
		TEXT("Asset"),
		EExtensionHook::After, 
		HTNEditor->GetToolkitCommands(), 
		FToolBarExtensionDelegate::CreateSP(this, &FHTNEditorToolbarBuilder::FillDebuggerToolbar));
	HTNEditor->AddToolbarExtender(ToolbarExtender);
}

void FHTNEditorToolbarBuilder::FillAssetToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateSP(this, &FHTNEditorToolbarBuilder::MakeDiffMenu),
		LOCTEXT("Diff", "Diff"),
		LOCTEXT("BlueprintEditorDiffToolTip", "Diff against previous revisions"),
		FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "BlueprintDiff.ToolbarIcon"));
}

void FHTNEditorToolbarBuilder::FillModesToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(HTNEditorWeakPtr.IsValid());
	const TSharedPtr<FHTNEditor> HTNEditor = HTNEditorWeakPtr.Pin();

	const TAttribute<FName> GetActiveMode(HTNEditor.ToSharedRef(), &FHTNEditor::GetCurrentMode);
	const FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(HTNEditor.ToSharedRef(), &FHTNEditor::SetCurrentMode);

	// Left side padding
	HTNEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

	// HTN Mode
	HTNEditor->AddToolbarWidget(
		SNew(SModeWidget, FHTNEditor::GetLocalizedModeDescription(FHTNEditor::HTNMode), FHTNEditor::HTNMode)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(HTNEditor.Get(), &FHTNEditor::CanAccessHTNMode)
		.ToolTipText(LOCTEXT("HTNModeButtonTooltip", "Switch to HTN Mode"))
		.IconImage(FHTNAppStyle::GetBrush("BTEditor.SwitchToBehaviorTreeMode"))
#if ENGINE_MAJOR_VERSION <= 4
		.SmallIconImage(FHTNAppStyle::GetBrush("BTEditor.SwitchToBehaviorTreeMode.Small"))
#endif
	);

	HTNEditor->AddToolbarWidget(SNew(SHTNModeSeparator));

	// Blackboard mode
	HTNEditor->AddToolbarWidget(
		SNew(SModeWidget, FHTNEditor::GetLocalizedModeDescription(FHTNEditor::BlackboardMode), FHTNEditor::BlackboardMode)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(HTNEditor.Get(), &FHTNEditor::CanAccessBlackboardMode)
		.ToolTipText(LOCTEXT("BlackboardModeButtonTooltip", "Switch to Blackboard Mode"))
		.IconImage(FHTNAppStyle::GetBrush("BTEditor.SwitchToBlackboardMode"))
#if ENGINE_MAJOR_VERSION <= 4
		.SmallIconImage(FHTNAppStyle::GetBrush("BTEditor.SwitchToBlackboardMode.Small"))
#endif
	);

	// Right side padding
	HTNEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));
}

void FHTNEditorToolbarBuilder::FillDebuggerToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(HTNEditorWeakPtr.IsValid());
	const TSharedPtr<FHTNEditor> HTNEditor = HTNEditorWeakPtr.Pin();
	const TSharedPtr<FHTNDebugger> Debugger = HTNEditor->GetDebugger();
	if (!Debugger.IsValid() || !Debugger->IsDebuggerReady())
	{
		return;
	}

	const TSharedRef<SWidget> SelectionBox = SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectDebugActorTitle", "Debugged actor"))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SComboButton)
			.OnGetMenuContent(Debugger.ToSharedRef(), &FHTNDebugger::GetActorsMenu)
			.ButtonContent()
			[
				SNew(STextBlock)
				.ToolTipText(LOCTEXT("SelectDebugActor", "Pick actor to debug"))
				.Text(Debugger.ToSharedRef(), &FHTNDebugger::GetCurrentActorDescription)
			]
		];

	/*
	ToolbarBuilder.BeginSection("CachedState");
	{
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().BackOver);
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().BackInto);
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().ForwardInto);
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().ForwardOver);
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().StepOut);
	}
	ToolbarBuilder.EndSection();
	*/
	ToolbarBuilder.BeginSection(TEXT("World"));
	{
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().PausePlaySession);
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().ResumePlaySession);
		ToolbarBuilder.AddToolBarButton(FHTNDebuggerCommands::Get().StopPlaySession);
		ToolbarBuilder.AddSeparator();
		ToolbarBuilder.AddWidget(SelectionBox);
	}
	ToolbarBuilder.EndSection();
}

namespace
{
	// Delegate called to diff a specific revision with the current
	void OnDiffRevisionPicked(const FRevisionInfo& RevisionInfo, TWeakObjectPtr<UHTN> HTN)
	{
		if (!HTN.IsValid())
		{
			return;
		}

		const FString HTNFullName = HTN->GetFullName();
		FString ClassName, PackageName, ObjectName, SubObjectName;
		FPackageName::SplitFullObjectPath(HTNFullName, ClassName, PackageName, ObjectName, SubObjectName);
		const FString ObjectPathInPackage = SubObjectName.Len() > 0 ? ObjectName + TEXT(":") + SubObjectName : ObjectName;

		// Get the SCC state
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		const FString Filename = SourceControlHelpers::PackageFilename(PackageName);
		const FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);
		if (!SourceControlState.IsValid())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CouldNotFindSourceControlState", "Could not find source control state for file {0}"), 
				FText::FromString(Filename)));
			return;
		}

		for (int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); ++HistoryIndex)
		{
			TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex).ToSharedRef();
			if (Revision->GetRevision() != RevisionInfo.Revision)
			{
				continue;
			}

			// Get the revision of this package from source control
			FString PreviousTempPkgName;
			if (Revision->Get(PreviousTempPkgName))
			{
				if (UPackage* const PreviousTempPkg = LoadPackage(nullptr, *PreviousTempPkgName, LOAD_ForDiff | LOAD_DisableCompileOnLoad))
				{
					if (UHTN* const PreviousHTN = FindObject<UHTN>(PreviousTempPkg, *ObjectPathInPackage))
					{
						FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
						const int32 CheckInIdentifier = Revision->GetCheckInIdentifier();
						const FDateTime& Date = Revision->GetDate();
						const FRevisionInfo OldRevision { Revision->GetRevision(), CheckInIdentifier, Date };
						const FRevisionInfo CurrentRevision { TEXT(""), CheckInIdentifier, Date };
						AssetToolsModule.Get().DiffAssets(PreviousHTN, HTN.Get(), OldRevision, CurrentRevision);
					}
					else
					{
						FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UnableToFindAssetInPackage", "Unable to find the HTN asset in the package from the old revision."));
					}
				}
				else
				{
					FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UnableToLoadAssets", "Unable to load assets to diff. Content may no longer be supported?"));
				}
			}
			break;
		}
	}
}

TSharedRef<SWidget> FHTNEditorToolbarBuilder::MakeDiffMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	if (!ISourceControlModule::Get().IsEnabled() || !ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("SourceControlDisabled", "Source control is disabled"),
			FText(), FSlateIcon(), FUIAction());
		return MenuBuilder.MakeWidget();
	}

	const TSharedPtr<FHTNEditor> HTNEditor = HTNEditorWeakPtr.Pin();
	if (!HTNEditor.IsValid())
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("NoHTNEditor", "No HTN editor is running"),
			FText(), FSlateIcon(), FUIAction());
		return MenuBuilder.MakeWidget();
	}

	UHTN* const HTN = HTNEditor->GetCurrentHTN();
	if (!IsValid(HTN))
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("NoHTNAsset", "No HTN asset is being edited"),
			FText(), FSlateIcon(), FUIAction());
		return MenuBuilder.MakeWidget();
	}

	return SNew(SHTNRevisionMenu, HTN)
		.OnRevisionSelected_Static(&OnDiffRevisionPicked, TWeakObjectPtr<UHTN>(HTN));
}

#undef LOCTEXT_NAMESPACE