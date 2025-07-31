// Copyright 2020-2025 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphEditor.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

// An entry in the tree of search results displayed by the SHTNSearch widget.
// The search results are arranged in a tree so results of decorators/services are under results of their parent nodes.
class FHTNSearchResult
{
public: 
	FHTNSearchResult(class UEdGraphNode* InNode = nullptr, TSharedPtr<FHTNSearchResult> InParent = nullptr, TOptional<FString> MatchedString = {});

	TSharedRef<SWidget>	CreateIcon() const;
	FText GetText() const;
	FText GetHighlightText() const;
	UEdGraphNode* GetNodeToJumpToOnClick() const;

	TWeakObjectPtr<class UEdGraphNode> GraphNode;
	TOptional<FString> MatchedString;

	TWeakPtr<FHTNSearchResult> Parent;
	TArray<TSharedPtr<FHTNSearchResult>> Children;
};

struct FHTNSearchContext
{
public:
	FHTNSearchContext(const FString& SearchString);

	TSharedPtr<FHTNSearchResult> SearchNode(class UEdGraphNode* Node) const;
	TOptional<FString> Matches(const class UEdGraphNode* Node) const;
	TOptional<FString> Matches(const FString& String) const;

private:
	TArray<FString> Queries;
};

// TODO rename the file to SHTNSearch.h to match others.
// Widget for the "Search" tab of the HTN editor, allows for searching node titles, types, etc. through the the HTN graph.
class SHTNSearch : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHTNSearch){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class FHTNEditor> InHTNEditor);

	// Focuses this widget's search box
	void FocusForUse();

private:
	using STreeViewType = STreeView<TSharedPtr<FHTNSearchResult>>;

	// Search box functions
	void OnSearchTextChanged(const FText& Text);
	void OnSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType);

	void UpdateSearch(const FText& SearchString);
	void SetNodeHighlighting(TArrayView<const TSharedPtr<FHTNSearchResult>> Results, bool bHighlight);
	 
	// Tree view functions
	TSharedRef<class ITableRow> OnGenerateRow(TSharedPtr<FHTNSearchResult> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedPtr<FHTNSearchResult> Item, TArray<TSharedPtr<FHTNSearchResult>>& OutChildren);
	void OnTreeSelectionChanged(TSharedPtr<FHTNSearchResult> Item, ESelectInfo::Type SelectInfo);

	class UEdGraph* GetGraph() const;
	void ZoomToFitNodes(const SGraphEditor& GraphEditor, TArrayView<UEdGraphNode* const> Nodes) const;

	TWeakPtr<class FHTNEditor> HTNEditorPtr;
	TSharedPtr<STreeViewType> TreeView;
	TSharedPtr<class SSearchBox> SearchTextField;

	// The top-level items shown by the tree view
	TArray<TSharedPtr<FHTNSearchResult>> TopLevelResults;
};
