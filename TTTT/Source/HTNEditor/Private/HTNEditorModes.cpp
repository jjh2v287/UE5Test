// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNEditorModes.h"
#include "HTNEditorTabIds.h"
#include "HTNEditor.h"
#include "HTNEditorTabFactories.h"
#include "HTNEditorToolbarBuilder.h"
#include "Runtime/Launch/Resources/Version.h"

/////////////////////////////////////////////////////
// FHTNEditorApplicationMode

#define LOCTEXT_NAMESPACE "HTNApplicationMode"

FHTNEditorApplicationMode::FHTNEditorApplicationMode(TSharedPtr<class FHTNEditor> InHTNEditor)
	: FApplicationMode(FHTNEditor::HTNMode, FHTNEditor::GetLocalizedModeDescription),
	HTNEditor(InHTNEditor)
{
	HTNEditorTabFactories.RegisterFactory(MakeShared<FHTNDetailsSummoner>(InHTNEditor));
	HTNEditorTabFactories.RegisterFactory(MakeShared<FHTNSearchSummoner>(InHTNEditor));
	HTNEditorTabFactories.RegisterFactory(MakeShared<FHTNBlackboardSummoner>(InHTNEditor));

#if ENGINE_MAJOR_VERSION >= 5
	TabLayout = FTabManager::NewLayout(TEXT("Standalone_HTN_Layout_v2"))
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab(FHTNEditorTabIds::GraphEditorID, ETabState::ClosedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.3f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(FHTNEditorTabIds::GraphDetailsID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(FHTNEditorTabIds::BlackboardID, ETabState::OpenedTab)
					->AddTab(FHTNEditorTabIds::SearchID, ETabState::ClosedTab)
				)
			)
		);
#else
	TabLayout = FTabManager::NewLayout(TEXT("Standalone_HTN_Layout_v1"))
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(InHTNEditor->GetToolbarTabId(), ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.7f)
					->AddTab(FHTNEditorTabIds::GraphEditorID, ETabState::ClosedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.3f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->AddTab(FHTNEditorTabIds::GraphDetailsID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.4f)
						->AddTab(FHTNEditorTabIds::BlackboardID, ETabState::OpenedTab)
						->AddTab(FHTNEditorTabIds::SearchID, ETabState::ClosedTab)
					)
				)
			)
		);
#endif

	InHTNEditor->GetToolbarBuilder()->AddAssetToolbar(ToolbarExtender);
	InHTNEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);
	InHTNEditor->GetToolbarBuilder()->AddDebuggerToolbar(ToolbarExtender);
}

void FHTNEditorApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(HTNEditor.IsValid());
	const TSharedPtr<FHTNEditor> HTNEditorPtr = HTNEditor.Pin();

	HTNEditorPtr->RegisterToolbarTabSpawner(InTabManager.ToSharedRef());
	HTNEditorPtr->PushTabFactories(HTNEditorTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FHTNEditorApplicationMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();

	check(HTNEditor.IsValid());
	HTNEditor.Pin()->SaveEditedObjectState();
}

void FHTNEditorApplicationMode::PostActivateMode()
{
	// Reopen any documents that were open when the asset was last saved
	check(HTNEditor.IsValid());
	HTNEditor.Pin()->RestoreHTN();

	FApplicationMode::PostActivateMode();
}

#undef  LOCTEXT_NAMESPACE

/////////////////////////////////////////////////////
// FHTNBlackboardEditorApplicationMode

#define LOCTEXT_NAMESPACE "BlackboardApplicationMode"

FHTNBlackboardEditorApplicationMode::FHTNBlackboardEditorApplicationMode(TSharedPtr<class FHTNEditor> InHTNEditor)
	: FApplicationMode(FHTNEditor::BlackboardMode, FHTNEditor::GetLocalizedModeDescription),
	HTNEditor(InHTNEditor)
{
	BlackboardTabFactories.RegisterFactory(MakeShared<FHTNBlackboardEditorSummoner>(InHTNEditor));
	BlackboardTabFactories.RegisterFactory(MakeShared<FHTNBlackboardDetailsSummoner>(InHTNEditor));

#if ENGINE_MAJOR_VERSION >= 5
	TabLayout = FTabManager::NewLayout(TEXT("Standalone_HTNBlackboardEditor_Layout_v2"))
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(FHTNEditorTabIds::BlackboardEditorID, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(FHTNEditorTabIds::BlackboardDetailsID, ETabState::OpenedTab)
			)
		);
#else
	TabLayout = FTabManager::NewLayout(TEXT("Standalone_HTNBlackboardEditor_Layout_v1"))
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(InHTNEditor->GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(FHTNEditorTabIds::BlackboardEditorID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(FHTNEditorTabIds::BlackboardDetailsID, ETabState::OpenedTab)
				)
			)
		);
#endif

	InHTNEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);
}

void FHTNBlackboardEditorApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(HTNEditor.IsValid());
	const TSharedPtr<FHTNEditor> HTNEditorPtr = HTNEditor.Pin();

	HTNEditorPtr->RegisterToolbarTabSpawner(InTabManager.ToSharedRef());
	HTNEditorPtr->PushTabFactories(BlackboardTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

#undef LOCTEXT_NAMESPACE
