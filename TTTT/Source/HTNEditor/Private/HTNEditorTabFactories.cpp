// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#include "HTNEditorTabFactories.h"
#include "HTNEditorTabIds.h"
#include "HTN.h"
#include "HTNAppStyle.h"

#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "HTNEditorFactories"

FHTNBlackboardSummoner::FHTNBlackboardSummoner(TSharedPtr<FHTNEditor> InHTNEditor)
	: FWorkflowTabFactory(FHTNEditorTabIds::BlackboardID, InHTNEditor)
	, HTNEditor(InHTNEditor)
{
	TabLabel = LOCTEXT("BlackboardLabel", "Blackboard");
	TabIcon = FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "Kismet.Tabs.Components");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("BlackboardView", "Blackboard");
	ViewMenuTooltip = LOCTEXT("BlackboardView_ToolTip", "Show the blackboard view");
}

TSharedRef<SWidget> FHTNBlackboardSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return HTNEditor.Pin()->SpawnBlackboardViewWidget();
}

FText FHTNBlackboardSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("BlackboardTabTooltip", "The Blackboard view is for viewing and debugging blackboard key/value pairs.");
}


FHTNBlackboardEditorSummoner::FHTNBlackboardEditorSummoner(TSharedPtr<FHTNEditor> InHTNEditor)
	: FWorkflowTabFactory(FHTNEditorTabIds::BlackboardEditorID, InHTNEditor)
	, HTNEditor(InHTNEditor)
{
	TabLabel = LOCTEXT("BlackboardLabel", "Blackboard");
	TabIcon = FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "Kismet.Tabs.Components");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("BlackboardEditor", "Blackboard");
	ViewMenuTooltip = LOCTEXT("BlackboardEditor_ToolTip", "Show the blackboard editor");
}

TSharedRef<SWidget> FHTNBlackboardEditorSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return HTNEditor.Pin()->SpawnBlackboardEditorWidget();
}

FText FHTNBlackboardEditorSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("BlackboardEditorTabTooltip", "The Blackboard editor is for editing and debugging blackboard key/value pairs.");
}


FHTNBlackboardDetailsSummoner::FHTNBlackboardDetailsSummoner(TSharedPtr<FHTNEditor> InHTNEditor)
	: FWorkflowTabFactory(FHTNEditorTabIds::BlackboardDetailsID, InHTNEditor)
	, HTNEditor(InHTNEditor)
{
	TabLabel = LOCTEXT("BlackboardDetailsLabel", "Blackboard Details");
	TabIcon = FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "Kismet.Tabs.Components");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("BlackboardDetailsView", "Details");
	ViewMenuTooltip = LOCTEXT("BlackboardDetailsView_ToolTip", "Show the details view");
}

TSharedRef<SWidget> FHTNBlackboardDetailsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	check(HTNEditor.IsValid());
	return HTNEditor.Pin()->SpawnBlackboardDetailsWidget();
}

FText FHTNBlackboardDetailsSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("BlackboardDetailsTabTooltip", "The details tab is for editing blackboard entries.");
}


FHTNDetailsSummoner::FHTNDetailsSummoner(TSharedPtr<FHTNEditor> InHTNEditor)
	: FWorkflowTabFactory(FHTNEditorTabIds::GraphDetailsID, InHTNEditor)
	, HTNEditor(InHTNEditor)
{
	TabLabel = LOCTEXT("HTNDetailsLabel", "Details");
	TabIcon = FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "Kismet.Tabs.Components");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("HTNDetailsView", "Details");
	ViewMenuTooltip = LOCTEXT("HTNDetailsView_ToolTip", "Show the details view");
}

TSharedRef<SWidget> FHTNDetailsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	check(HTNEditor.IsValid());
	return HTNEditor.Pin()->SpawnDetailsWidget();
}

FText FHTNDetailsSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("HTNDetailsTabTooltip", "The HTN details tab allows editing of the properties of HTN nodes");
}


FHTNSearchSummoner::FHTNSearchSummoner(TSharedPtr<class FHTNEditor> InHTNEditor)
	: FWorkflowTabFactory(FHTNEditorTabIds::SearchID, InHTNEditor)
	, HTNEditor(InHTNEditor)
{
	TabLabel = LOCTEXT("BehaviorTreeSearchLabel", "Search");
	TabIcon = FSlateIcon(FHTNAppStyleGetAppStyleSetName(), "Kismet.Tabs.FindResults");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("HTNSearchView", "Search");
	ViewMenuTooltip = LOCTEXT("HTNSearchView_ToolTip", "Show the HTN search tab");
}

TSharedRef<SWidget> FHTNSearchSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	check(HTNEditor.IsValid());
	return HTNEditor.Pin()->SpawnSearchWidget();
}

FText FHTNSearchSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("HTNSearchTabTooltip", "The HTN search tab allows searching within HTN nodes");
}


FHTNGraphEditorSummoner::FHTNGraphEditorSummoner(TSharedPtr<FHTNEditor> InHTNEditor, FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback) :
	FDocumentTabFactoryForObjects<UEdGraph>(FHTNEditorTabIds::GraphDetailsID, InHTNEditor),
	HTNEditor(InHTNEditor),
	OnCreateGraphEditorWidget(CreateGraphEditorWidgetCallback)
{}

void FHTNGraphEditorSummoner::OnTabActivated(TSharedPtr<SDockTab> Tab) const
{
	check(HTNEditor.IsValid());
	const TSharedRef<SGraphEditor> GraphEditorWidget = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	HTNEditor.Pin()->OnGraphEditorFocused(GraphEditorWidget);
}

void FHTNGraphEditorSummoner::OnTabRefreshed(TSharedPtr<SDockTab> Tab) const
{
	const TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	GraphEditor->NotifyGraphChanged();
}

TAttribute<FText> FHTNGraphEditorSummoner::ConstructTabNameForObject(UEdGraph* DocumentID) const
{
	return TAttribute<FText>(FText::FromString(DocumentID->GetName()));
}

TSharedRef<SWidget> FHTNGraphEditorSummoner::CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	return OnCreateGraphEditorWidget.Execute(DocumentID);
}

const FSlateBrush* FHTNGraphEditorSummoner::GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	return FHTNAppStyle::GetBrush("NoBrush");
}

void FHTNGraphEditorSummoner::SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const
{
	check(HTNEditor.IsValid());
	check(HTNEditor.Pin()->GetCurrentHTN());

	UEdGraph* const Graph = FTabPayload_UObject::CastChecked<UEdGraph>(Payload);

#if UE_VERSION_OLDER_THAN(5, 6, 0)
	FVector2D ViewLocation;
#else
	FVector2f ViewLocation;
#endif
	float ZoomAmount;
	const TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);
	
	HTNEditor.Pin()->GetCurrentHTN()->LastEditedDocuments.Emplace(Graph, ViewLocation, ZoomAmount);
}

#undef LOCTEXT_NAMESPACE
