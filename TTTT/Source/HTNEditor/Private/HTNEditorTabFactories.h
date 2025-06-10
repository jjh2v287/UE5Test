// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "Widgets/SWidget.h"
#include "GraphEditor.h"
#include "HTNEditor.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"

struct FHTNBlackboardSummoner : public FWorkflowTabFactory
{
public:
	FHTNBlackboardSummoner(TSharedPtr<class FHTNEditor> InHTNEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;
};

struct FHTNBlackboardEditorSummoner : public FWorkflowTabFactory
{
public:
	FHTNBlackboardEditorSummoner(TSharedPtr<class FHTNEditor> InHTNEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;
};

struct FHTNBlackboardDetailsSummoner : public FWorkflowTabFactory
{
public:
	FHTNBlackboardDetailsSummoner(TSharedPtr<class FHTNEditor> InHTNEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;
};

struct FHTNDetailsSummoner : public FWorkflowTabFactory
{
public:
	FHTNDetailsSummoner(TSharedPtr<class FHTNEditor> InHTNEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;
};

struct FHTNSearchSummoner : public FWorkflowTabFactory
{
public:
	FHTNSearchSummoner(TSharedPtr<class FHTNEditor> InHTNEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;
};

// Creates the widgets for the graph editor part of an HTNEditor
class FHTNGraphEditorSummoner : public FDocumentTabFactoryForObjects<UEdGraph>
{
public:
	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SGraphEditor>, FOnCreateGraphEditorWidget, UEdGraph*);
	
	FHTNGraphEditorSummoner(TSharedPtr<class FHTNEditor> InHTNEditor, FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback);
	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const override;
	virtual void OnTabRefreshed(TSharedPtr<SDockTab> Tab) const override;

protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UEdGraph* DocumentID) const override;
	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const override;
	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const override;
	virtual void SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const override;

protected:
	TWeakPtr<class FHTNEditor> HTNEditor;
	FOnCreateGraphEditorWidget OnCreateGraphEditorWidget;
};